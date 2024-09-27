#include "gvga.h"
#include "_gvga.h"
#include "c16_pico.h"

#define FRAME_WIDTH 640
#define FRAME_HEIGHT 480

static struct GVga _gvga;
static GVga *gvga = &_gvga;
static mutex_t _gvga_mutex;

static scanvideo_mode_t _gvga_mode_640x480_60 =
{
    .default_timing = &vga_timing_640x480_60_default,
    .pio_program = &video_24mhz_composable,
    .width = 640,
    .height = 480,
    .xscale = 1,
    .yscale = 1,
};

static uint _gvga_bufptr;
static bool _gvga_oddFrame = false;

#include "_gvga_palette.c"
#include "_gvga_scanlines.c"

static void core1_func() {
    scanvideo_setup(gvga->vga_mode);
    scanvideo_timing_enable(true);
    render_loop();
}

GVga *gvga_init(uint16_t width, uint16_t height, int bits, bool doubleBuffer, bool interlaced, void *context) {
    gvga->bits = bits; // 0 == text mode
        gvga->mode = GVGA_MODE_BITMAP;
    if (bits < 0) {
        gvga->bits = 1;
        gvga->mode = GVGA_MODE_TEXT;
    }
    gvga->context = context;
    gvga->height = height;
    gvga->width = width;
    gvga->colors = 1 << gvga->bits;
    gvga->pixelsPerByte = 8 / gvga->bits;
    gvga->rowBytes = width / gvga->pixelsPerByte;
    gvga->rows = height / 8;
    gvga->cols = width / 8;
    // scale vga width
    gvga->vga_mode = &_gvga_mode_640x480_60;
    if (gvga->width == 320) {
        _gvga_mode_640x480_60.width = 320;
        _gvga_mode_640x480_60.xscale = 2;
    } else if (gvga->width == 640) {
        _gvga_mode_640x480_60.width = 640;
        _gvga_mode_640x480_60.xscale = 1;
    } else {
        return NULL;
    }

    // scale vga height
    gvga->multiplier = (FRAME_HEIGHT + 1) / height;
    uint vga_height = FRAME_HEIGHT / gvga->multiplier;
    gvga->headerRows = (vga_height - height) / 2;
    _gvga_mode_640x480_60.height = vga_height;
    _gvga_mode_640x480_60.yscale = gvga->multiplier;

    switch(gvga->bits) {
        case 1:
        case 2:
        case 4:
        case 8:
            break;
        default:
            return NULL;
    }
    uint pixels_per_byte = 8 / gvga->bits;
    uint32_t frame_size = width * height / pixels_per_byte * (doubleBuffer ? 2 : 1);
    if (gvga->mode & GVGA_MODE_TEXT) {
        frame_size /= 8;
    }
    if (frame_size > 200000) return NULL;
    gvga->showFrame = calloc(frame_size, 1);
    if (gvga->showFrame == NULL) return gvga_destroy(gvga);

    gvga->drawFrame = gvga->showFrame;
    if (doubleBuffer) {
        gvga->drawFrame = calloc(frame_size, 1);
        if (gvga->drawFrame == NULL) return gvga_destroy(gvga);
    }
    gvga->scanning_mutex = &_gvga_mutex;
    mutex_init(gvga->scanning_mutex);

    gvga->palette = calloc(gvga->colors, sizeof(GVgaColor));
    if (gvga->palette == NULL) return gvga_destroy(gvga);

    switch(gvga->bits) {
        case 1:
            gvga_setPalette(gvga, _gvga_palette16, 0, 2);
            gvga->scanline_render = _scanline_render_1bpp;
            break;
        case 2:
            gvga_setPalette(gvga, _gvga_palette16, 0, 4);
            gvga->scanline_render = interlaced ? _scanline_render_2bpp_interlaced : _scanline_render_2bpp;
            break;
        case 4:
            gvga_setPalette(gvga, _gvga_palette16, 0, 16);
            gvga->scanline_render = interlaced ? _scanline_render_4bpp_interlaced : _scanline_render_4bpp;
            break;
        case 8:
            // set default 256 color palette
            for(uint i=0; i<256; i++) {
                uint red = ((i & 0xe0) >> 5) + 1; // 3 bits
                uint green = ((i & 0x1c) >> 2) + 1; // 3 bits
                uint blue = (i & 0x03) + 1; // 2 bits
                gvga->palette[i] = GVGA_COLOR(red * 4 - 1, green * 4 - 1, blue * 8 - 1);
            }
            // copy the 16-bit default palette to the 256-bit palette
            gvga_setPalette(gvga, _gvga_palette16, 0, 16);
            gvga->scanline_render = interlaced ? _scanline_render_8bpp_interlaced : _scanline_render_8bpp;
            break;
    }

    return gvga;
}

void gvga_sync(GVga *gvga) {
    // wait for the mutex to be acquired by core 1
    while (mutex_try_enter(gvga->scanning_mutex, NULL)) {
        // the other core hasn't started displaying the scanlines
        // release it immediately so core 1 can acquire it
        mutex_exit(gvga->scanning_mutex); 
    }
    // core 1 has acquired the mutex
    // wait for it to release it
    while(!mutex_try_enter(gvga->scanning_mutex, NULL)) {
        // the other core is still displaying the scanlines
        // wait for it to finish
    }
    // core 1 has released the mutex and we now have it
    // release it so that core 1 is free to display the frame buffer
    mutex_exit(gvga->scanning_mutex); 
}

void gvga_swap(GVga *gvga, bool copy) {
    if (copy && (gvga->drawFrame != gvga->showFrame)) {
        memcpy(gvga->showFrame, gvga->drawFrame, gvga->width * gvga->height / (8/gvga->bits));
    }
    uint8_t *temp = gvga->drawFrame;
    gvga->drawFrame = gvga->showFrame;
    gvga->showFrame = temp;
}


 void gvga_start(GVga *gvga) {
    multicore_launch_core1(core1_func);
 }

void gvga_setBorderColors(GVga *gvga, GVgaColor top, GVgaColor bottom, GVgaColor left, GVgaColor right) {
    gvga->borderColors[GVGA_TOP] = top;
    gvga->borderColors[GVGA_BOTTOM] = bottom;
    gvga->borderColors[GVGA_LEFT] = left;
    gvga->borderColors[GVGA_RIGHT] = right;
}

 void gvga_setPalette(GVga* gvga, GVgaColor *palette, uint start, uint count) {
    for(int i=0; i<count; i++) {
        gvga->palette[start + i] = palette[i];
    }
    gvga_setBorderColors(gvga, gvga->palette[0], gvga->palette[0], gvga->palette[0], gvga->palette[0]);
    switch(gvga->bits) {
        case 1:
            _gvga_palette_1bpp(gvga);
            break;
        case 2:
            _gvga_palette_2bpp(gvga);
            break;
        case 4: 
            _gvga_palette_4bpp(gvga);
            break;
        case 8: 
            break;
    }   
 }

 GVga *gvga_destroy(GVga *gvga) {
    if (gvga->showFrame != NULL) free(gvga->showFrame);
    if (gvga->drawFrame != NULL && gvga->showFrame != gvga->drawFrame) free(gvga->drawFrame);
    if (gvga->palette != NULL) free(gvga->palette);
    return NULL;
 }

 void gvga_stop(GVga *gvga) {

 }

void gvga_text(GVga *gvga, int row, int col, char *text, uint16_t color) {
	char *offset = (char *) gvga->drawFrame + row * gvga->rowBytes + col;
    while(*text) {
        *offset++ = *text++;
    }
}

void gvga_char(GVga *gvga, int row, int col, unsigned char c, uint16_t color) {
	unsigned char *offset = gvga->drawFrame + row * gvga->rowBytes + col;
    *offset = c;
}
