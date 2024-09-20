#include "gfx.h"
#include <stdlib.h>
#include <string.h>
#include "c16.h"

extern unsigned char fontData[];
typedef unsigned int uint;

#define _1_PIXELS_PER_BYTE 1
#define _2_PIXELS_PER_BYTE 2
#define _4_PIXELS_PER_BYTE 4
#define _8_PIXELS_PER_BYTE 8

// minimal graphics library for GVga
void gfx_set8bpp(GVga *gvga, int x, int y, int color) {
    uint row = y * gvga->width / _1_PIXELS_PER_BYTE;
    uint byte = row + x;
    gvga->drawFrame[byte] = color;
}

void gfx_set4bpp(GVga *gvga, int x, int y, int color) {
    uint row = y * gvga->width / _2_PIXELS_PER_BYTE;
    uint byte = row + x / _2_PIXELS_PER_BYTE;
    uint mask = 0;
    if (x % 2 == 0) {
        mask = 0x0f;
        color = color << 4;
    } else {
        mask = 0xf0;
    }
    gvga->drawFrame[byte] = (gvga->drawFrame[byte] & mask) | color;
}

void gfx_set2bpp(GVga *gvga, int x, int y, int color) {
    uint row = y * gvga->width / _4_PIXELS_PER_BYTE;
    uint byte = row + x / _4_PIXELS_PER_BYTE;
    uint mask = 0;
    switch (x % 4) {
        case 0:
            mask = 0xc0;
            color = color << 6;
            break;
        case 1:
            mask = 0x30;
            color = color << 4;
            break;
        case 2:
            mask = 0x0c;
            color = color << 2;
            break;
        case 3:
            mask = 0x03;
            break;
    }
    gvga->drawFrame[byte] = (gvga->drawFrame[byte] & ~mask) | color;
}

void gfx_set1bpp(GVga *gvga, int x, int y, int color) {
    uint row = y * gvga->width / _8_PIXELS_PER_BYTE;
    uint byte = row + x / _8_PIXELS_PER_BYTE;
    uint mask = 1 << (7 - (x % 8));
    if (color) {
        gvga->drawFrame[byte] |= mask;
    } else {
        gvga->drawFrame[byte] &= ~mask;
    }
}

void gfx_set(GVga *gvga, int x, int y, int color) {
    switch(gvga->bits ) {
        case 1:
            gfx_set1bpp(gvga, x, y, color);
            break;
        case 2:
            gfx_set2bpp(gvga, x, y, color);
            break;
        case 4:
            gfx_set4bpp(gvga, x, y, color);
            break;
        case 8:
            gfx_set8bpp(gvga, x, y, color);
            break;
    }
}

void gfx_line(GVga *gvga, int x0, int y0, int x1, int y1, int color) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    while(x0 != x1 || y0 != y1) {
        gfx_set(gvga, x0, y0, color);
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void gfx_clear(GVga *gvga, int color) {
    switch(gvga->bits) {
        case 1:
            memset(gvga->drawFrame, color ? 0xff : 0x00, gvga->width * gvga->height / _8_PIXELS_PER_BYTE);
            return;
        case 2:
            memset(gvga->drawFrame, (color | (color << 2) | (color << 4) | (color << 6)), gvga->width * gvga->height / _4_PIXELS_PER_BYTE);
            return;
        case 4:
            memset(gvga->drawFrame, (color | (color << 4)), gvga->width * gvga->height / _2_PIXELS_PER_BYTE);
            return;
        case 8:
            memset(gvga->drawFrame, color, gvga->width * gvga->height / _1_PIXELS_PER_BYTE);
            return;
    }
}

void gfx_box(GVga *gvga, int x0, int y0, int x1, int y1, int color) {
    gfx_line(gvga, x0, y0, x1, y0, color);
    gfx_line(gvga, x1, y0, x1, y1, color);
    gfx_line(gvga, x1, y1, x0, y1, color);
    gfx_line(gvga, x0, y1, x0, y0, color);
}

void gfx_box_fill(GVga *gvga, int x0, int y0, int x1, int y1, int color) {
    for(int y=y0; y<=y1; y++) {
        gfx_line(gvga, x0, y, x1, y, color);
    }
}

#define FONT_WIDTH 8
#define FONT_HEIGHT 8
void gfx_char(GVga *gvga, uint16_t x, uint16_t y, uint8_t c, int color) {
    uint row = y * gvga->rowBytes;
    uint col = x / gvga->pixelsPerByte;
    uint8_t *font = &fontData[c*8];
    for (int i=0; i<8; i++) {
        uint8_t byte = *font++;
        gvga->showFrame[row + col] = byte;
        row += gvga->rowBytes;
    }
}

uint8_t _xlate(uint8_t c) {
    if (c >= '@' && c <= '_') return c - '@' + 0;
    if (c >= ' ' && c <= '?') return c - ' ' + 32;
    if (c >= '`' && c <= 0x7f) return c - '`' + 128;
    return c;
}
void gfx_text(GVga *gvga, uint16_t x, uint16_t y, char *text, int color) {
    while(*text) {
        uint8_t c = _xlate(*text++);
        gfx_char(gvga, x, y, c, color);
        x += 8;
    }
}