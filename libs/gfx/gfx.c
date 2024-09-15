#include "gfx.h"
#include <stdlib.h>
#include <string.h>

// minimal graphics library for GVga
void gfx_set8bpp(GVga *gvga, int x, int y, int color) {
    uint row = y * gvga->width;
    uint byte = row + x;
    gvga->bitplanes[0][byte] = color;
}

void gfx_set4bpp(GVga *gvga, int x, int y, int color) {
    uint row = y * gvga->width / gvga->pixelsPerByte;
    uint byte = row + x / gvga->pixelsPerByte;
    uint mask;
    if (x % 2 == 0) {
        mask = 0x0f;
        color = color << 4;
    } else {
        mask = 0xf0;
    }
    gvga->bitplanes[0][byte] = (gvga->bitplanes[0][byte] & mask) | color;
}

void gfx_set2bpp(GVga *gvga, int x, int y, int color) {
    uint row = y * gvga->width / gvga->pixelsPerByte;
    uint byte = row + x / gvga->pixelsPerByte;
    uint mask;
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
    gvga->bitplanes[0][byte] = (gvga->bitplanes[0][byte] & ~mask) | color;
}

void gfx_set1bpp(GVga *gvga, int x, int y, int color) {
    uint row = y * gvga->width / gvga->pixelsPerByte;
    uint byte = row + x / gvga->pixelsPerByte;
    uint mask = 1 << (7 - (x % 8));
    if (color) {
        gvga->bitplanes[0][byte] |= mask;
    } else {
        gvga->bitplanes[0][byte] &= ~mask;
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
            memset(gvga->bitplanes[0], color ? 0xff : 0x00, gvga->width * gvga->height / 8);
            return;
        case 2:
            memset(gvga->bitplanes[0], (color | (color << 2) | (color << 4) | (color << 6)), gvga->width * gvga->height / 4);
            return;
        case 4:
            memset(gvga->bitplanes[0], (color | (color << 4)), gvga->width * gvga->height / 2);
            return;
        case 8:
            memset(gvga->bitplanes[0], color, gvga->width * gvga->height);
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