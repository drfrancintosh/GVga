#include "gfx.h"
#include <stdlib.h>
#include <string.h>
// #include "c16.h"

extern unsigned char fontData[];
typedef unsigned int uint;

#define _1_PIXELS_PER_BYTE 1
#define _2_PIXELS_PER_BYTE 2
#define _4_PIXELS_PER_BYTE 4
#define _8_PIXELS_PER_BYTE 8

// minimal graphics library for GVga
void gfx_set8bpp(GVga *gvga, int x, int y, uint16_t pen) {
    uint row = y * gvga->width / _1_PIXELS_PER_BYTE;
    uint byte = row + x;
    gvga->drawFrame[byte] = pen;
}

void gfx_set4bpp(GVga *gvga, int x, int y, uint16_t pen) {
    uint row = y * gvga->width / _2_PIXELS_PER_BYTE;
    uint byte = row + x / _2_PIXELS_PER_BYTE;
    uint mask = 0;
    if (x % 2 == 0) {
        mask = 0x0f;
        pen = pen << 4;
    } else {
        mask = 0xf0;
    }
    gvga->drawFrame[byte] = (gvga->drawFrame[byte] & mask) | pen;
}

void gfx_set2bpp(GVga *gvga, int x, int y, uint16_t pen) {
    uint row = y * gvga->width / _4_PIXELS_PER_BYTE;
    uint byte = row + x / _4_PIXELS_PER_BYTE;
    uint mask = 0;
    switch (x % 4) {
        case 0:
            mask = 0xc0;
            pen = pen << 6;
            break;
        case 1:
            mask = 0x30;
            pen = pen << 4;
            break;
        case 2:
            mask = 0x0c;
            pen = pen << 2;
            break;
        case 3:
            mask = 0x03;
            break;
    }
    gvga->drawFrame[byte] = (gvga->drawFrame[byte] & ~mask) | pen;
}

void gfx_set1bpp(GVga *gvga, int x, int y, uint16_t pen) {
    uint row = y * gvga->width / _8_PIXELS_PER_BYTE;
    uint byte = row + x / _8_PIXELS_PER_BYTE;
    uint mask = 1 << (7 - (x % 8));
    if (pen) {
        gvga->drawFrame[byte] |= mask;
    } else {
        gvga->drawFrame[byte] &= ~mask;
    }
}

void gfx_set(GVga *gvga, int x, int y, uint16_t pen) {
    switch(gvga->bits ) {
        case 1:
            gfx_set1bpp(gvga, x, y, pen);
            break;
        case 2:
            gfx_set2bpp(gvga, x, y, pen);
            break;
        case 4:
            gfx_set4bpp(gvga, x, y, pen);
            break;
        case 8:
            gfx_set8bpp(gvga, x, y, pen);
            break;
    }
}

void gfx_line(GVga *gvga, int x0, int y0, int x1, int y1, uint16_t pen) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    while(x0 != x1 || y0 != y1) {
        gfx_set(gvga, x0, y0, pen);
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

void gfx_clear(GVga *gvga, uint16_t pen) {    
    switch(gvga->bits) {
        case 1:
            memset(gvga->drawFrame, pen ? 0xff : 0x00, gvga->width * gvga->height / gvga->pixelsPerByte);
            return;
        case 2:
            memset(gvga->drawFrame, (pen | (pen << 2) | (pen << 4) | (pen << 6)), gvga->width * gvga->height / gvga->pixelsPerByte);
            return;
        case 4:
            memset(gvga->drawFrame, (pen | (pen << 4)), gvga->width * gvga->height / gvga->pixelsPerByte);
            return;
        case 8:
            memset(gvga->drawFrame, pen, gvga->width * gvga->height / gvga->pixelsPerByte);
            return;
    }
}

void gfx_box(GVga *gvga, int x0, int y0, int x1, int y1, uint16_t pen) {
    gfx_line(gvga, x0, y0, x1, y0, pen);
    gfx_line(gvga, x1, y0, x1, y1, pen);
    gfx_line(gvga, x1, y1, x0, y1, pen);
    gfx_line(gvga, x0, y1, x0, y0, pen);
}

void gfx_box_fill(GVga *gvga, int x0, int y0, int x1, int y1, uint16_t pen) {
    for(int y=y0; y<=y1; y++) {
        gfx_line(gvga, x0, y, x1, y, pen);
    }
}

#define FONT_WIDTH 8
#define FONT_HEIGHT 8

uint8_t _bitDoubler[16] = {0, 3, 12, 15, 48, 51, 60, 63, 192, 195, 204, 207, 240, 243, 252, 255};
uint8_t _bitQuadrupler[4] = {0, 0x0f, 0xf0, 0xff};

void gfx_char(GVga *gvga, uint16_t x, uint16_t y, uint8_t c, uint16_t pen) {
    pen = pen & (gvga->colors - 1);
    uint16_t penMask = 0xff;
    uint row = y * gvga->rowBytes;
    uint col = x / gvga->pixelsPerByte;
    uint8_t *ptr = &gvga->drawFrame[row + col];
    uint8_t *font = &gvga->font->data[c*8];
    switch(gvga->bits) {
        case 1:
            penMask = pen ? 0xff : 0x00;
            break;
        case 2:
            penMask = pen | pen << 2 | pen << 4 | pen << 6;
            break;
        case 4:
            penMask = pen | pen << 4;
            break;
        case 8:
            penMask = pen;
            break;
    }
    for (int i=0; i<8; i++) {
        uint8_t byte = *font++;
        switch (gvga->bits) {
            case 1:
                ptr[0] = byte & penMask;
                break;
            case 2:
                ptr[0] = (_bitDoubler[byte / 16] & penMask);
                ptr[1] = (_bitDoubler[byte % 16] & penMask);
                break;
            case 4:
                ptr[0] = (_bitQuadrupler[(byte & 0xc0) >> 6 ] & penMask);
                ptr[1] = (_bitQuadrupler[(byte & 0x30) >> 4 ] & penMask);
                ptr[2] = (_bitQuadrupler[(byte & 0x0c) >> 2 ] & penMask);
                ptr[3] = (_bitQuadrupler[(byte & 0x03) ] & penMask);
                break;
            case 8:
                for(int j=0; j<8; j++) {
                    ptr[j] = (byte & (1 << (7-j))) ? pen : 0;
                }
                break;
        }
        ptr += gvga->rowBytes;
    }
}

void gfx_text(GVga *gvga, uint16_t x, uint16_t y, char *text, uint16_t pen) {
    while(*text) {
        gfx_char(gvga, x, y, *text++, pen);
        x += 8;
    }
}