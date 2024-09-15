#pragma once
#include "pico.h"
#include "pico/scanvideo.h"

#define GVGA_COLOR(r,g,b) PICO_SCANVIDEO_PIXEL_FROM_RGB5(r, g, b)
typedef uint16_t GVgaColor;

enum {
    GVGA_TOP = 0,
    GVGA_BOTTOM = 1,
    GVGA_LEFT = 2,
    GVGA_RIGHT = 3,
};

#if PICO_SCANVIDEO_MAX_SCANLINE_BUFFER_WORDS < 700
#error "This library requires PICO_SCANVIDEO_MAX_SCANLINE_BUFFER_WORDS to be at least 320"
#endif

typedef enum {
    GVGA_BLACK = GVGA_COLOR(0, 0, 0),
    GVGA_BLUE = GVGA_COLOR(0, 0, 0x1f),
    GVGA_GREEN = GVGA_COLOR(0, 0x1f, 0),
    GVGA_CYAN = GVGA_COLOR(0, 0x1f, 0x1f),
    GVGA_RED = GVGA_COLOR(0x1f, 0, 0),
    GVGA_MAGENTA = GVGA_COLOR(0x1f, 0, 0x1f),
    GVGA_VIOLET = GVGA_COLOR(0x1f, 0, 0x1f),
    GVGA_PURPLE = GVGA_COLOR(0x1f, 0, 0x1f),
    GVGA_YELLOW = GVGA_COLOR(0x1f, 0x1f, 0),
    GVGA_WHITE = GVGA_COLOR(0x1f, 0x1f, 0x1f),
} GDviColor;

typedef struct GVga {
    void *hardware;
    uint16_t height;
    uint16_t width;
    uint16_t bits;
    uint16_t colors;
    void *context;
    uint8_t *bitplanes[3];
    uint8_t paletteLogic[4]; // note: only 3 are used
    GVgaColor palette[256];
    uint32_t fatBits;
    uint16_t multiplier;
    uint16_t headerRows;
    uint8_t borderColors[4];
    int32_t (*scanline_render)(uint32_t *buf, size_t buf_length, int width, int scanline);
} GVga;

extern GVga *gvga_init(uint16_t width, uint16_t height, uint8_t bits, void *context);
extern void gvga_start(GVga *gvga);
extern void gvga_setPalette(GVga* gvga, GVgaColor *palette);
extern void gvga_setBorderColors(GVga *gvga, GVgaColor top, GVgaColor bottom, GVgaColor left, GVgaColor right);

extern void gvga_destroy(GVga *gvga);
extern void gvga_stop(GVga *gvga);
