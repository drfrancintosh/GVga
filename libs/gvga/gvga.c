#include "gvga.h"
#include <stdlib.h>
#include <string.h>

#include "pico/scanvideo.h"
#include "pico/scanvideo/composable_scanline.h"
#include "pico/multicore.h"

static struct GVga _gvga;
static GVga *gvga = &_gvga;

#define VGA_MODE vga_mode_320x240_60
#define FRAME_HEIGHT 240
#define FRAME_WIDTH 320

static uint16_t colorbuf[256 * 8];
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
static int32_t striped_scanline(uint32_t *buf, size_t buf_length, int width, uint32_t color16) {
    _bufptr = 0;
    uint32_t colorBase = color16 / 80;
    uint16_t rgb[3] = {0, 0, 0};
    uint16_t color = COLOR(rgb[0], rgb[1], rgb[2]);
    push0(buf, COMPOSABLE_RAW_RUN);
    push1(buf, color);
    push0(buf, 320-3);
    push1(buf, color);
    push0(buf, color);
    push1(buf, color);
    push0(buf, color);
    push1(buf, color);
    push0(buf, color);
    push1(buf, color);
    push0(buf, color);
    push1(buf, color);
    for(uint32_t i = 2; i < 10; i+=2) {
        push0(buf, color);
        push1(buf, color);
    }
    for(uint32_t j=1; j <= 0x1f; j++) {
        rgb[colorBase] = j;
        color = COLOR(rgb[0], rgb[1], rgb[2]);
        for(uint32_t i = 0; i < 10; i+=2) {
            push0(buf, color);
            push1(buf, color);
        }
    }
    push0(buf, COMPOSABLE_RAW_1P);
    push1(buf, 0);
    // note we must end with a black pixel
    push32(buf, COMPOSABLE_EOL_ALIGN);
    return _bufptr;
}

static int32_t _scanline_render_1bpp(uint32_t *buf, size_t buf_length, int width, int scanline) {
    _bufptr = 0;
    uint8_t *row = &gvga->bitplanes[0][scanline * width / gvga->pixelsPerByte];
    for(int pixel=0; pixel < width; pixel+=8) {
        uint8_t byte = row[pixel / 8];
        uint16_t *colors = &colorbuf[byte * 8];
        if (pixel == 0) {
            push0(buf, COMPOSABLE_RAW_RUN);
            push1(buf, colors[0]);
            push0(buf, width-3);
            push1(buf, colors[1]);
            push0(buf, colors[2]);
            push1(buf, colors[3]);
            push0(buf, colors[4]);
            push1(buf, colors[5]);
            push0(buf, colors[6]);
            push1(buf, colors[7]);
        } else {
            push0(buf, colors[0]);
            push1(buf, colors[1]);
            push0(buf, colors[2]);
            push1(buf, colors[3]);
            push0(buf, colors[4]);
            push1(buf, colors[5]);
            push0(buf, colors[6]);
            push1(buf, colors[7]);
        }
    }
    push0(buf, COMPOSABLE_RAW_1P);
    push1(buf, 0);
    // note we must end with a black pixel
    push32(buf, COMPOSABLE_EOL_ALIGN);
    return _bufptr;
}

static int32_t _scanline_render_2bpp(uint32_t *buf, size_t buf_length, int width, int scanline) {
    _bufptr = 0;
    uint8_t *row = &gvga->bitplanes[0][scanline * width / gvga->pixelsPerByte];
    uint8_t byte = *row++;
    uint16_t *colors = &colorbuf[byte * gvga->colors];
    push0(buf, COMPOSABLE_RAW_RUN);
    push1(buf, *colors++);
    push0(buf, width-3);
    push1(buf, *colors++);
    push0(buf, *colors++);
    push1(buf, *colors++);
    for(int pixel=gvga->pixelsPerByte; pixel < width; pixel += gvga->pixelsPerByte) {
        byte = *row++;
        colors = &colorbuf[byte * gvga->colors];
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
    uint8_t *row = &gvga->bitplanes[0][scanline * width / gvga->pixelsPerByte];
    uint8_t byte = *row++;
    uint16_t *colors = &colorbuf[byte * gvga->colors];
    push0(buf, COMPOSABLE_RAW_RUN);
    push1(buf, colors[0]);
    push0(buf, width-3);
    push1(buf, colors[1]);
    push0(buf, colors[2]);
    push1(buf, colors[3]);
    for(int pixel=4; pixel < width; pixel+=4) {
        byte = *row++;
        colors = &colorbuf[byte * gvga->colors];
        push0(buf, colors[0]);
        push1(buf, colors[1]);
        push1(buf, colors[2]);
        push1(buf, colors[3]);
    }
    push0(buf, COMPOSABLE_RAW_1P);
    push1(buf, 0);
    // note we must end with a black pixel
    push32(buf, COMPOSABLE_EOL_ALIGN);
    return _bufptr;
}

static int32_t _scanline_render_8bpp(uint32_t *buf, size_t buf_length, int width, int scanline) {
    _bufptr = 0;
    uint8_t *byte = &gvga->bitplanes[0][scanline * width];
    push0(buf, COMPOSABLE_RAW_RUN);
    push1(buf, gvga->palette[*byte++]);
    push0(buf, width-3);
    push1(buf, gvga->palette[*byte++]);
    for(int pixel=2; pixel < width; pixel+=2) {
        push0(buf, gvga->palette[*byte++]);
        push1(buf, gvga->palette[*byte++]);
    }
    push0(buf, COMPOSABLE_RAW_1P);
    push1(buf, 0);
    // note we must end with a black pixel
    push32(buf, COMPOSABLE_EOL_ALIGN);
    return _bufptr;
}

static void __time_critical_func(render_loop)() {
    while (true) {
        struct scanvideo_scanline_buffer *dest = scanvideo_begin_scanline_generation(true);
        uint32_t *buf = dest->data;
        size_t buf_length = dest->data_max;
        // uint16_t frameno = scanvideo_frame_number(dest->scanline_id);
        uint16_t scanline = scanvideo_scanline_number(dest->scanline_id);
        // dest->data_used = striped_scanline(buf, buf_length, VGA_MODE.width, scanline);
        dest->data_used = gvga->scanline_render(buf, buf_length, VGA_MODE.width, scanline);
        dest->status = SCANLINE_OK;
        scanvideo_end_scanline_generation(dest);
    }
}

static void core1_func() {
    scanvideo_setup(&VGA_MODE);
    scanvideo_timing_enable(true);
    render_loop();
}

static GVgaColor _palette2[2] = {GVGA_BLACK, GVGA_WHITE};
static GVgaColor _palette4[4] = {GVGA_BLACK, GVGA_RED, GVGA_GREEN, GVGA_BLUE};
static GVgaColor _palette16[16] = {GVGA_WHITE, GVGA_RED, GVGA_GREEN, GVGA_BLUE, GVGA_YELLOW, GVGA_CYAN, GVGA_VIOLET, GVGA_WHITE, GVGA_COLOR(15, 15, 15), GVGA_COLOR(15, 15, 0), GVGA_COLOR(15, 0, 15), GVGA_COLOR(15, 0, 0), GVGA_COLOR(0, 15, 15), GVGA_COLOR(0, 15, 0), GVGA_COLOR(0, 0, 15), GVGA_COLOR(7, 7, 7)};
static GVgaColor _palette256[256];

GVga *gvga_init(uint16_t width, uint16_t height, uint8_t bits, void *context) {
    gvga->height = height;
    gvga->width = width;
    gvga->bits = bits;
    gvga->colors = 1 << bits;
    gvga->pixelsPerByte = 8 / bits;
    gvga->context = context;

    if (width == 320) {
        // gvga->bitwise = fatbitwise_init();
        // gvga->fatBits = 1;
    }
    gvga->multiplier = (FRAME_HEIGHT + 1) / height;
    gvga->headerRows = (FRAME_HEIGHT - height * gvga->multiplier) / 2;

    gvga->bitplanes[0] = calloc(width * height / gvga->pixelsPerByte, 1);
    if (bits == 1) {
        gvga_setPalette(gvga, _palette2);
        gvga->scanline_render = _scanline_render_1bpp;
    } else if (bits == 2) {
        gvga_setPalette(gvga, _palette4);
        gvga->scanline_render = _scanline_render_2bpp;
    } else if (bits == 4) {
        gvga_setPalette(gvga, _palette16);
        gvga->scanline_render = _scanline_render_4bpp;
    } else if (bits == 8) {
        // set default 256 color palette
        for(uint i=0; i<256; i++) {
            gvga->palette[i] = GVGA_COLOR(i >> 3, i >> 3, i >> 3);
        }
        memcpy(_palette256, _palette16, 16 * sizeof(GVgaColor));
        gvga_setPalette(gvga, _palette256);
        gvga->scanline_render = _scanline_render_8bpp;
    }

    return gvga;
}

static void _palette_1bpp(GVga *gvga) {
    for(uint i=0; i<256; i++) {
        for(uint j=0; j<8; j++) {
            uint bit = 1 << (7-j);
            uint index = (i & bit) ? 1 : 0;
            colorbuf[i * 8 + j] = gvga->palette[index];
        }
    }
}

static void _palette_2bpp(GVga *gvga) {
    for(uint i=0; i<256; i++) {
        uint chew0 = (i & 0xc0) >> 6;
        uint chew1 = (i & 0x30) >> 4;
        uint chew2 = (i & 0x0c) >> 2;
        uint chew3 = (i & 0x03);
        colorbuf[i * 4 + 0] = gvga->palette[chew0];
        colorbuf[i * 4 + 1] = gvga->palette[chew1];
        colorbuf[i * 4 + 2] = gvga->palette[chew2];
        colorbuf[i * 4 + 3] = gvga->palette[chew3];
    }
}

static void _palette_4bpp(GVga *gvga) {
    for(uint i=0; i<256; i++) {
        uint nybble0 = (i & 0xf0) >> 4;
        uint nybble1 = i & 0x0f;
        colorbuf[i * 2 + 0] = gvga->palette[nybble0];
        colorbuf[i * 2 + 1] = gvga->palette[nybble1];
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

 void gvga_setPalette(GVga* gvga, GVgaColor *palette) {
    memcpy(gvga->palette, palette, gvga->colors * sizeof(GVgaColor));
    gvga_setBorderColors(gvga, palette[0], palette[0], palette[0], palette[0]);
    if (gvga->bits == 1) {
        _palette_1bpp(gvga);
    } else if (gvga->bits == 2) {
        _palette_2bpp(gvga);
    } else if (gvga->bits == 4) {
        _palette_4bpp(gvga);
    }
 }

 void gvga_destroy(GVga *gvga) {

 }

 void gvga_stop(GVga *gvga) {

 }
