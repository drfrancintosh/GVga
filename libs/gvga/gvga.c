#include "gvga.h"
#include "_gvga.h"
#include "gvga_font.h"
#include "_gvga_font_data.h"
#include "gmem.h"

#define FRAME_WIDTH 640
#define FRAME_HEIGHT 480

static struct GVga _gvga;
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

static void __time_critical_func(render_loop)(GVga *gvga) {
    bool isBlocked = false;
    bool isInterlaced = gvga->mode & GVGA_MODE_INTERLACED;
    while (true) {
        struct scanvideo_scanline_buffer *dest = scanvideo_begin_scanline_generation(true);
        uint32_t *buf = dest->data;
        size_t buf_length = dest->data_max;
        uint16_t framenumber = scanvideo_frame_number(dest->scanline_id);
        uint16_t scanline = scanvideo_scanline_number(dest->scanline_id);
        bool isEvenScanline = !(scanline & 1);
        _gvga_oddFrame = framenumber & 1;
        if (scanline == 0 && !isBlocked) {
            mutex_enter_blocking(gvga->scanningMutex);
            isBlocked = true;
        }
        if (scanline < gvga->headerRows) {
            dest->data_used = _scanline_render_blank_line(gvga, buf, buf_length, gvga->width, scanline, gvga->borderColors[GVGA_TOP]);
        } else if (scanline < gvga->height + gvga->headerRows) {
            if (isInterlaced && (_gvga_oddFrame ^ isEvenScanline)) {
                dest->data_used = _scanline_render_blank_line(gvga, buf, buf_length, gvga->width, scanline, gvga->palette[0]);
            } else {
                dest->data_used = gvga->scanlineRender(gvga, buf, buf_length, gvga->width, scanline - gvga->headerRows);
            }
        } else {
            dest->data_used = _scanline_render_blank_line(gvga, buf, buf_length, gvga->width, scanline, gvga->borderColors[GVGA_BOTTOM]);
        }
        dest->status = SCANLINE_OK;
        scanvideo_end_scanline_generation(dest);
        if ((scanline >= gvga->height - 1) && isBlocked) {
            mutex_exit(gvga->scanningMutex);
            isBlocked = false;
        }
    }
}

static void core1_func() {
    scanvideo_setup(_gvga.vga_mode);
    scanvideo_timing_enable(true);
    render_loop(&_gvga);
}

static void _errorScreen(GVga *gvga, char *msg) {
    if (gvga) {
        gvga_destroy(gvga);
    }
    char line1[80];
    char line2[80];
    char line3[80];
    uint32_t mem = gvga->width * gvga->height / (8 / gvga->bits) * (gvga->mode & GVGA_MODE_DOUBLE_BUFFERED ? 2 : 1);
    sprintf(line1, msg);
    sprintf(line2, "w=%d h=%d b=%d dblbuf=%b", gvga->width, gvga->height, gvga->bits, (gvga->mode & GVGA_MODE_DOUBLE_BUFFERED)!=0);
    sprintf(line3, "Mem: %d,%03d", mem / 1000, mem % 1000);

    int row = 0;
    int col = 0;
    uint16_t pen = 1;
	gvga = gvga_init(320, 120, -1, false, false, NULL);
	gvga_start(gvga);
    gvga_text(gvga, row++, col, line1, pen);
    gvga_text(gvga, row++, col, line2, pen);
    gvga_text(gvga, row++, col, line3, pen);
    while(1);
}

GVga *gvga_init(uint16_t width, uint16_t height, int bits, bool doubleBuffer, bool interlaced, void *userData) {
    GVga *gvga = &_gvga;
    gvga->bits = bits; // 0 == text mode
        gvga->mode = GVGA_MODE_BITMAP;
    if (bits < 0) {
        gvga->bits = 1;
        gvga->mode = GVGA_MODE_TEXT;
    }
    gvga->mode |= interlaced ? GVGA_MODE_INTERLACED : 0;
    gvga->mode |= doubleBuffer ? GVGA_MODE_DOUBLE_BUFFERED : 0;
    gvga->userData = userData;
    gvga->height = height;
    gvga->width = width;
    gvga->colors = 1 << gvga->bits;
    gvga->pixelsPerByte = 8 / gvga->bits;
    gvga->rowBytes = width / gvga->pixelsPerByte;
    gvga->frameBytes = gvga->rowBytes * height;
    gvga->rows = height / 8;
    gvga->cols = width / 8;

    if (gvga->height > FRAME_HEIGHT) {
        _errorScreen(gvga, "Invalid height");
    }
    // scale vga width
    gvga->vga_mode = &_gvga_mode_640x480_60;
    if (gvga->width == 320) {
        _gvga_mode_640x480_60.width = 320;
        _gvga_mode_640x480_60.xscale = 2;
    } else if (gvga->width == 640) {
        _gvga_mode_640x480_60.width = 640;
        _gvga_mode_640x480_60.xscale = 1;
    } else {
        _errorScreen(gvga, "Invalid width");
    }

    gvga->font = gvga_font_init(8, 8, 0, 255, _gvga_font_data);

    // scale vga height
    gvga->multiplier = (FRAME_HEIGHT + 1) / height;
    uint vgaHeight = FRAME_HEIGHT / gvga->multiplier;
    gvga->headerRows = (vgaHeight - height) / 2;
    _gvga_mode_640x480_60.height = vgaHeight;
    _gvga_mode_640x480_60.yscale = gvga->multiplier;

    switch(gvga->bits) {
        case 1:
        case 2:
        case 4:
        case 8:
            break;
        default:
            _errorScreen(gvga, "Invalid bits");
    }
    uint pixelsPerByte = 8 / gvga->bits;
    uint32_t frameBytes = width * height / pixelsPerByte;
    if (gvga->mode & GVGA_MODE_TEXT) {
        frameBytes /= 8;
    }
    gvga->showFrame = gcalloc(frameBytes, 1);
    if (gvga->showFrame == NULL) _errorScreen(gvga, "Out of memory - showFrame");

    gvga->drawFrame = gvga->showFrame;
    if (doubleBuffer) {
        gvga->drawFrame = gcalloc(frameBytes, 1);
        if (gvga->drawFrame == NULL) _errorScreen(gvga, "Out of memory - drawFrame");
    }
    gvga->scanningMutex = &_gvga_mutex;
    mutex_init(gvga->scanningMutex);

    gvga->palette = gcalloc(gvga->colors, sizeof(GVgaColor));
    if (gvga->palette == NULL) _errorScreen(gvga, "Out of memory - palette");

    switch(gvga->bits) {
        case 1:
            gvga_setPalette(gvga, _gvga_palette16, 0, 2);
            gvga->scanlineRender = _scanline_render_1bpp;
            break;
        case 2:
            gvga_setPalette(gvga, _gvga_palette16, 0, 4);
            gvga->scanlineRender = _scanline_render_2bpp;
            break;
        case 4:
            gvga_setPalette(gvga, _gvga_palette16, 0, 16);
            gvga->scanlineRender = _scanline_render_4bpp;
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
            gvga->scanlineRender = _scanline_render_8bpp;
            break;
    }

    return gvga;
}

void gvga_sync(GVga *gvga) {
    // wait for the mutex to be acquired by core 1
    while (mutex_try_enter(gvga->scanningMutex, NULL)) {
        // the other core hasn't started displaying the scanlines
        // release it immediately so core 1 can acquire it
        mutex_exit(gvga->scanningMutex); 
    }
    // core 1 has acquired the mutex
    // wait for it to release it
    while(!mutex_try_enter(gvga->scanningMutex, NULL)) {
        // the other core is still displaying the scanlines
        // wait for it to finish
    }
    // core 1 has released the mutex and we now have it
    // release it so that core 1 is free to display the frame buffer
    mutex_exit(gvga->scanningMutex); 
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
