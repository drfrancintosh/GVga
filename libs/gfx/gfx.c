#include "gfx.h"
#include <stdlib.h>
#include <string.h>

// minimal graphics library for GVga
void gfx_set8(GVga *gvga, int x, int y, int color) {
    uint row = y * gvga->width;
    uint byte = row + x;
    gvga->bitplanes[0][byte] = color;
}

void gfx_set4(GVga *gvga, int x, int y, int color) {
    uint row = y * gvga->width / 2;
    uint byte = row + x / 2;
    uint mask;
    if (x % 2 == 0) {
        mask = 0x0f;
        color = color << 4;
    } else {
        mask = 0xf0;
    }
    gvga->bitplanes[0][byte] = (gvga->bitplanes[0][byte] & mask) | color;
}

void gfx_set(GVga *gvga, int x, int y, int color) {
    if (gvga->bits == 4) {
        gfx_set4(gvga, x, y, color);
        return;
    }
    if (gvga->bits == 8) {
        gfx_set8(gvga, x, y, color);
        return;
    }
    uint byte = x / 8 + y * gvga->width / 8;
    uint planes = gvga->bits;
    uint mask = 0x80 >> (x % 8);
    if (gvga->bits == 3) {
        color = gvga->palette[color];
    }
    for(int plane=0; plane<planes; plane++) {
        if (color & 0x01) {
            gvga->bitplanes[plane][byte] |= mask;
        } else {
            gvga->bitplanes[plane][byte] &= ~mask;
        }
        color = color >> 1;
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
    if (gvga->bits == 8) {
        memset(gvga->bitplanes[0], color, gvga->width * gvga->height);
        return;
    }
    if (gvga->bits == 4) {
        memset(gvga->bitplanes[0], (color | (color << 4)), gvga->width * gvga->height / 2);
        return;
    }
    if (gvga->bits == 3) {
        color = gvga->palette[color];
    }
    for(int i=0; i<gvga->bits; i++) {
        memset(gvga->bitplanes[i], color ? 0xff : 0x00, gvga->width * gvga->height / 8);
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