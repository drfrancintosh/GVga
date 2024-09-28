#pragma once
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "gvga_font.h"

#define GVGA_COLOR(r,g,b) (((b)<<11u)|((g)<<6)|((r)<<0))
typedef uint16_t GVgaColor;

enum {
    GVGA_TOP = 0,
    GVGA_BOTTOM = 1,
    GVGA_LEFT = 2,
    GVGA_RIGHT = 3,
};

#if PICO_SCANVIDEO_MAX_SCANLINE_BUFFER_WORDS < 320
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

typedef enum {
    GVGA_MODE_BITMAP = 0x01,
    GVGA_MODE_TEXT= 0x02,
    GVGA_MODE_INTERLACED = 0x04,
} GVgaMode;

typedef struct GVga {
    void *vga_mode;
    uint16_t height;
    uint16_t width;
    uint16_t bits;
    uint16_t colors;
    uint16_t rowBytes;
    uint16_t pixelsPerByte;
    uint16_t multiplier;
    uint16_t headerRows;
    uint16_t mode;
    uint16_t rows;
    uint16_t cols;
    GVgaColor borderColors[4];
    GVgaColor *palette;
    uint8_t *drawFrame;
    uint8_t *showFrame;
    int32_t (*scanlineRender)(struct GVga *gvga, uint32_t *buf, size_t bufLength, int width, int scanline);
    void *scanningMutex;
    GVgaFont *font;
    void *userData;

} GVga;

extern GVga *gvga_init(uint16_t width, uint16_t height, int bits, bool doubleBuffer, bool interlaced, void *userData);
extern void gvga_start(GVga *gvga);
extern void gvga_setPalette(GVga* gvga, GVgaColor *palette, unsigned intstart, unsigned intcount);
extern void gvga_setBorderColors(GVga *gvga, GVgaColor top, GVgaColor bottom, GVgaColor left, GVgaColor right);

extern GVga *gvga_destroy(GVga *gvga);
extern void gvga_stop(GVga *gvga);
extern void gvga_sync(GVga *gvga);
extern void gvga_swap(GVga *gvga, bool copy);

extern void gvga_text(GVga *gvga, int row, int col, char *text, uint16_t color);
extern void gvga_char(GVga *gvga, int row, int col, unsigned char c, uint16_t color);
