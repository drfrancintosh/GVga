#include "gfx.h"
#include "gvga.h"
#include <stdlib.h>
#include <string.h>

#define _PPB 1
#define _PPB_SHIFT 8
#define FIXED_POINT_CONSTANT 15

static uint tmp;

inline int _sgn(int x) {
    return x < 0 ? -1 : 1;
}
inline int _abs(int x) {
    return x < 0 ? -x : x;
}

inline uint _min(uint a, uint b) {
    return a < b ? a : b;
}

inline uint _max(uint a, uint b) {
    return a > b ? a : b;
}

#define _swap(a, b) \
    tmp = a;\
    a = b;\
    b = tmp;\

inline uint _offset(GVga *gvga, uint x, uint y) {
    return y * gvga->rowBytes + x;
}

static void *set(GVga *gvga, uint x, uint y, uint pen) {
    pen = pen & 0xff;
    uint offset = _offset(gvga, x, y);
    gvga->drawFrame[offset] = pen;
    return NULL;
}

static void *hline(GVga *gvga, uint x0, uint y, uint x1, uint pen) {
    pen = 0xff;
    if (x0 > x1) {
        _swap(x0, x1);
    }
    uint offset = _offset(gvga, x0, y);
    uint8_t *frame = &gvga->drawFrame[offset];
    for(uint i = x0; i <= x1; i++) {
        *frame++ = pen;
    }
    return NULL;
}

static void *vline(GVga *gvga, uint x, uint y0, uint y1, uint pen) {
    pen = 0xff;
    if (y0 > y1) {
        _swap(y0, y1);
    }
    uint offset = _offset(gvga, x, y0);
    uint rowBytes = gvga->rowBytes;
    uint8_t *frame = &gvga->drawFrame[offset];
    for (uint i=y0; i<=y1; i++) {
        *frame = pen;
        frame += rowBytes;
    }
    return NULL;
}

inline void _lineq0(GVga *gvga, uint dx, uint dy, uint offset, int rowBytes, uint pen) {
    uint frameBytes = gvga->frameBytes;
    uint8_t *frame = &gvga->drawFrame[offset];
    if (dx > dy) {
        uint m = (dy << FIXED_POINT_CONSTANT) / dx;
        uint e = 0;
        for (int i=0; i<dx; i++) {
            frame[offset++] = pen;
            e += m;
            if (e >> FIXED_POINT_CONSTANT) {
                e -= 1 << FIXED_POINT_CONSTANT;
                offset += rowBytes;
                if (offset > frameBytes) return;
            }
        }
    } else {
        uint m = (dx << FIXED_POINT_CONSTANT) / dy;
        uint e = 0;
        for (int i=0; i<dy; i++) {
            frame[offset] = pen;
            offset += rowBytes;
            if (offset > frameBytes) return;
            e += m;
            if (e >> FIXED_POINT_CONSTANT) {
                e -= 1 << FIXED_POINT_CONSTANT;
                offset++;
                if (offset > frameBytes) return;
            }
        }
    }
}

static void *line(GVga *gvga, uint x0, uint y0, uint x1, uint y1, uint pen) {
    pen &= 0xff;
    if (x0 >= gvga->width || x1 >= gvga->width || y0 >= gvga->height || y1 >= gvga->height) return NULL;
    int dy = y1 - y0;
    if (dy == 0) return hline(gvga, x0, y0, x1, pen);
    int dx = x1 - x0;
    if (dx == 0) return vline(gvga, x0, y0, y1, pen);
    if (x1 < x0) {
        _swap(x0, x1);
        _swap(y0, y1);
    }
    uint offset = _offset(gvga, x0, y0);
    if (dx * dy > 0) {
        _lineq0(gvga, _abs(dx), _abs(dy), offset, gvga->rowBytes, pen);
    } else {
        _lineq0(gvga, _abs(dx), _abs(dy), offset, -gvga->rowBytes, pen);
    }
    return NULL;
}

static void *clear(GVga *gvga, uint pen) {
    pen &= 0xff;
    memset(gvga->drawFrame, pen, gvga->frameBytes);
    return NULL;
}

static void *rect(GVga *gvga, uint x0, uint y0, uint x1, uint y1, uint pen) {
    hline(gvga, x0, y0, x1, pen);
    hline(gvga, x0, y1, x1, pen);
    vline(gvga, x0, y0, y1, pen);
    vline(gvga, x1, y0, y1, pen);
    return NULL;
}

static void *rectFill(GVga *gvga, uint x0, uint y0, uint x1, uint y1, uint pen) {
    for(int y=y0; y<=y1; y++) {
        hline(gvga, x0, y, x1, pen);
    }
    return NULL;
}

static void *drawText(GVga *gvga, uint x, uint y, char *text, uint pen, uint bgnd) {
    uint rowBytes = gvga->rowBytes;
    uint row = y * rowBytes;
    uint col = x;
    uint8_t *fontData = gvga->font->data;
    uint8_t *frame = &gvga->drawFrame[row + col];
    while(*text) {
        uint8_t *glyph = &fontData[(*text++) * 8];
        uint8_t *nextFrame = frame + 8;
        for (int i=0; i<8; i++) {
            uint8_t byte = *glyph++;
            for(int j=0; j<8; j++) {
                frame[j] = (byte & (1 << (7-j))) ? pen : bgnd;
            }
        }
        frame = nextFrame;
    }
    return NULL;
}

static Gfx _gfx_8bpp = {
    .set = set,
    .hline = hline,
    .vline = vline,
    .line = line,
    .clear = clear,
    .rect = rect,
    .rectFill = rectFill,
    .drawText = drawText
};

Gfx *gfx_8bpp = &_gfx_8bpp;