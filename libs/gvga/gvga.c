#include "gvga.h"
#include <stdlib.h>
#include <string.h>

#include "pico/scanvideo.h"
#include "pico/scanvideo/composable_scanline.h"
#include "pico/multicore.h"

static struct GVga _gvga;
static GVga *gvga = &_gvga;

#define VGA_MODE vga_mode_320x240_60
#define _8_PIXELS_PER_BYTE 8
#define _4_PIXELS_PER_BYTE 4
#define _2_PIXELS_PER_BYTE 2
#define _1_PIXELS_PER_BYTE 1

static uint16_t _paletteBuf[256 * 8];
static uint _bufptr;

inline void push0(uint32_t *buf, uint16_t value) {
    buf[_bufptr] = value;
}

inline void push1(uint32_t *buf, uint16_t value) {
    buf[_bufptr++] |= (value << 16);
}

inline void push32(uint32_t *buf, uint16_t value) {
    buf[_bufptr] |= value << 16;
}

#define COLOR(r,g,b) PICO_SCANVIDEO_PIXEL_FROM_RGB5(r, g, b)
static int32_t _scanline_render_1bpp(uint32_t *buf, size_t buf_length, int width, int scanline) {
    _bufptr = 0;
    uint8_t *row = &gvga->showFrame[scanline * width / _8_PIXELS_PER_BYTE];
    uint16_t *colors = &_paletteBuf[(*row++) * _8_PIXELS_PER_BYTE];
    push0(buf, COMPOSABLE_RAW_RUN);
    push1(buf, *colors++);
    push0(buf, width-3);
    push1(buf, *colors++);
    push0(buf, *colors++);
    push1(buf, *colors++);
    push0(buf, *colors++);
    push1(buf, *colors++);
    push0(buf, *colors++);
    push1(buf, *colors++);
    for(int pixel=_8_PIXELS_PER_BYTE; pixel < width; pixel+=_8_PIXELS_PER_BYTE) {
        colors = &_paletteBuf[(*row++) * _8_PIXELS_PER_BYTE];
        push0(buf, *colors++);
        push1(buf, *colors++);
        push0(buf, *colors++);
        push1(buf, *colors++);
        push0(buf, *colors++);
        push1(buf, *colors++);
        push0(buf, *colors++);
        push1(buf, *colors++);
    }
    push0(buf, COMPOSABLE_RAW_1P);
    push1(buf, 0);
    // note we must end with a black pixel
    push32(buf, COMPOSABLE_EOL_ALIGN);
    return _bufptr;
}

static int32_t _scanline_render_2bpp(uint32_t *buf, size_t buf_length, int width, int scanline) {
    _bufptr = 0;
    uint8_t *row = &gvga->showFrame[scanline * width / _4_PIXELS_PER_BYTE];
    uint16_t *colors = &_paletteBuf[(*row++) * _4_PIXELS_PER_BYTE];
    push0(buf, COMPOSABLE_RAW_RUN);
    push1(buf, *colors++);
    push0(buf, width-3);
    push1(buf, *colors++);
    push0(buf, *colors++);
    push1(buf, *colors++);
    for(int pixel=_4_PIXELS_PER_BYTE; pixel < width; pixel += _4_PIXELS_PER_BYTE) {
        colors = &_paletteBuf[(*row++) * _4_PIXELS_PER_BYTE];
        push0(buf, *colors++);
        push1(buf, *colors++);
        push0(buf, *colors++);
        push1(buf, *colors++);
    }
    push0(buf, COMPOSABLE_RAW_1P);
    push1(buf, 0);
    // note we must end with a black pixel
    push32(buf, COMPOSABLE_EOL_ALIGN);
    return _bufptr;
}

static int32_t _scanline_render_4bpp(uint32_t *buf, size_t buf_length, int width, int scanline) {
    _bufptr = 0;
    uint8_t *row = &gvga->showFrame[scanline * width / _2_PIXELS_PER_BYTE];
    uint8_t byte = *row++;
    uint16_t *colors = &_paletteBuf[byte * _2_PIXELS_PER_BYTE];
    push0(buf, COMPOSABLE_RAW_RUN);
    push1(buf, *colors++);
    push0(buf, width-3);
    push1(buf, *colors++);
    for(int pixel=_2_PIXELS_PER_BYTE; pixel < width; pixel+= _2_PIXELS_PER_BYTE) {
        byte = *row++;
        colors = &_paletteBuf[byte * _2_PIXELS_PER_BYTE];
        push0(buf, *colors++);
        push1(buf, *colors++);
    }
    push0(buf, COMPOSABLE_RAW_1P);
    push1(buf, 0);
    // note we must end with a black pixel
    push32(buf, COMPOSABLE_EOL_ALIGN);
    return _bufptr;
}

static int32_t _scanline_render_8bpp(uint32_t *buf, size_t buf_length, int width, int scanline) {
    _bufptr = 0;
    uint8_t *row = &gvga->showFrame[scanline * width];
    GVgaColor *palette = gvga->palette;
    push0(buf, COMPOSABLE_RAW_RUN);
    push1(buf, palette[*row++]);
    push0(buf, width-3);
    push1(buf, palette[*row++]);
    for(int pixel=2; pixel < width; pixel+=2) {
        push0(buf, palette[*row++]);
        push1(buf, palette[*row++]);
    }
    push0(buf, COMPOSABLE_RAW_1P);
    push1(buf, 0);
    // note we must end with a black pixel
    push32(buf, COMPOSABLE_EOL_ALIGN);
    return _bufptr;
}

static void __time_critical_func(render_loop)() {
    bool isBlocked = false;
    while (true) {
        struct scanvideo_scanline_buffer *dest = scanvideo_begin_scanline_generation(true);
        uint32_t *buf = dest->data;
        size_t buf_length = dest->data_max;
        uint16_t scanline = scanvideo_scanline_number(dest->scanline_id);
        if (scanline == 0 && !isBlocked) {
            mutex_enter_blocking(&gvga->scanning_mutex);
            isBlocked = true;
        }
        // dest->data_used = striped_scanline(buf, buf_length, VGA_MODE.width, scanline);
        dest->data_used = gvga->scanline_render(buf, buf_length, VGA_MODE.width, scanline);
        dest->status = SCANLINE_OK;
        scanvideo_end_scanline_generation(dest);
        if ((scanline >= VGA_MODE.height - 1) && isBlocked) {
            mutex_exit(&gvga->scanning_mutex);
            isBlocked = false;
        }
    }
}

static void core1_func() {
    scanvideo_setup(&VGA_MODE);
    scanvideo_timing_enable(true);
    render_loop();
}

static GVgaColor _palette16[16] = {
    GVGA_BLACK, GVGA_RED, // 1-bit colors
    GVGA_GREEN, GVGA_BLUE, // additional colors for 2-bits
    GVGA_YELLOW, GVGA_CYAN, GVGA_VIOLET, GVGA_WHITE, // additional colors for 3-bits
    GVGA_COLOR(15, 15, 15), GVGA_COLOR(15, 0, 0), GVGA_COLOR(0, 15, 0), GVGA_COLOR(0, 0, 15), // additional colors for 4-bits (half-bright)
    GVGA_COLOR(15, 15, 0), GVGA_COLOR(0, 15, 15), GVGA_COLOR(15, 15, 0), GVGA_COLOR(7, 7, 7)};// additional colors for 4-bits (half-bright)

GVga *gvga_init(uint16_t width, uint16_t height, int bits, void *context) {
    bool doubleBuffer = bits < 0;
    gvga->context = context;
    gvga->height = height;
    gvga->width = width;
    gvga->bits = abs(bits);
    gvga->colors = 1 << gvga->bits;

    switch(gvga->bits) {
        case 1:
        case 2:
        case 4:
        case 8:
            break;
        default:
            return NULL;
    }
    gvga->showFrame = calloc(width * height / (8/gvga->bits), 1);
    if (gvga->showFrame == NULL) return NULL;

    gvga->drawFrame = gvga->showFrame;
    if (doubleBuffer) {
        gvga->drawFrame = calloc(width * height /  (8/gvga->bits), 1);
        if (gvga->drawFrame == NULL) return NULL;
    }
    mutex_init(&gvga->scanning_mutex);

    gvga->multiplier = (VGA_MODE.height + 1) / height;
    gvga->headerRows = (VGA_MODE.height - height * gvga->multiplier) / 2;

    gvga->palette = calloc(gvga->colors, sizeof(GVgaColor));
    if (gvga->palette == NULL) return NULL;

    switch(gvga->bits) {
        case 1:
            gvga_setPalette(gvga, _palette16, 0, 2);
            gvga->scanline_render = _scanline_render_1bpp;
            break;
        case 2:
            gvga_setPalette(gvga, _palette16, 0, 4);
            gvga->scanline_render = _scanline_render_2bpp;
            break;
        case 4:
            gvga_setPalette(gvga, _palette16, 0, 16);
            gvga->scanline_render = _scanline_render_4bpp;
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
            gvga_setPalette(gvga, _palette16, 0, 16);
            // gvga_setPalette(gvga, gvga->palette); // note it's copying the palette onto itself, but also setting the _paletteBuf
            gvga->scanline_render = _scanline_render_8bpp;
            break;
    }

    return gvga;
}

void gvga_sync(GVga *gvga) {
    // wait for the mutex to be acquired by core 1
    while (mutex_try_enter(&gvga->scanning_mutex, NULL)) {
        // the other core hasn't started displaying the scanlines
        // release it immediately so core 1 can acquire it
        mutex_exit(&gvga->scanning_mutex); 
    }
    // core 1 has acquired the mutex
    // wait for it to release it
    while(!mutex_try_enter(&gvga->scanning_mutex, NULL)) {
        // the other core is still displaying the scanlines
        // wait for it to finish
    }
    // core 1 has released the mutex and we now have it
    // release it so that core 1 is free to display the frame buffer
    mutex_exit(&gvga->scanning_mutex); 
}

void gvga_swap(GVga *gvga, bool copy) {
    if (copy && (gvga->drawFrame != gvga->showFrame)) {
        memcpy(gvga->showFrame, gvga->drawFrame, gvga->width * gvga->height / (8/gvga->bits));
    }
    uint8_t *temp = gvga->drawFrame;
    gvga->drawFrame = gvga->showFrame;
    gvga->showFrame = temp;
}

static void _palette_1bpp(GVga *gvga) {
    for(uint i=0; i<256; i++) {
        for(uint j=0; j<8; j++) {
            uint bit = 1 << (7-j);
            uint index = (i & bit) ? 1 : 0;
            _paletteBuf[i * 8 + j] = gvga->palette[index];
        }
    }
}

static void _palette_2bpp(GVga *gvga) {
    for(uint i=0; i<256; i++) {
        uint chew0 = (i & 0xc0) >> 6;
        uint chew1 = (i & 0x30) >> 4;
        uint chew2 = (i & 0x0c) >> 2;
        uint chew3 = (i & 0x03);
        _paletteBuf[i * 4 + 0] = gvga->palette[chew0];
        _paletteBuf[i * 4 + 1] = gvga->palette[chew1];
        _paletteBuf[i * 4 + 2] = gvga->palette[chew2];
        _paletteBuf[i * 4 + 3] = gvga->palette[chew3];
    }
}

static void _palette_4bpp(GVga *gvga) {
    for(uint i=0; i<256; i++) {
        uint nybble0 = (i & 0xf0) >> 4;
        uint nybble1 = i & 0x0f;
        _paletteBuf[i * _2_PIXELS_PER_BYTE + 0] = gvga->palette[nybble0];
        _paletteBuf[i * _2_PIXELS_PER_BYTE + 1] = gvga->palette[nybble1];
    }
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
            _palette_1bpp(gvga);
            break;
        case 2:
            _palette_2bpp(gvga);
            break;
        case 4: 
            _palette_4bpp(gvga);
            break;
        case 8: 
            break;
    }   
 }

 void gvga_destroy(GVga *gvga) {

 }

 void gvga_stop(GVga *gvga) {

 }
