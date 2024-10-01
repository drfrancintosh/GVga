#include "gfx.h"
#include "gvga.h"
#include <stdlib.h>
#include <string.h>

#define _PPB 2
#define _PPB_SHIFT 4
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
    return y * gvga->rowBytes + (x / _PPB);
}

static void *set(GVga *gvga, uint x, uint y, uint pen) {
    pen = pen & 0x0f;
    uint offset = _offset(gvga, x, y);
    uint mask;
    uint penBits;
    if (x & 0x01) {
        mask = 0xf0;
        penBits = pen << 4;
    } else {
        mask = 0x0f;
        penBits = pen;
    }
    gvga->drawFrame[offset] = (gvga->drawFrame[offset] & ~mask) | penBits;
    return NULL;
}

static void *hline(GVga *gvga, uint x0, uint y, uint x1, uint pen) {
    pen = 0x0f;
    if (x0 > x1) {
        _swap(x0, x1);
    }
    uint offset = _offset(gvga, x0, y);
    uint penBits = pen << 4;
    uint8_t *frame = gvga->drawFrame[offset];
    for(uint i = x0; i <= x1; i++) {
        if (i & 0x01) {
            *frame = (*frame & 0x0f) | penBits;
            frame++;
        } else {
            *frame= (*frame & 0xf0) | pen;
        }
    }
    return NULL;
}

static void *vline(GVga *gvga, uint x, uint y0, uint y1, uint pen) {
    pen = 0x0f;
    if (y0 > y1) {
        _swap(y0, y1);
    }
    uint mask;
    uint penBits;
    uint offset = _offset(gvga, x, y0);
    uint rowBytes = gvga->rowBytes;
    uint8_t *frame = &gvga->drawFrame[offset];
    if (x & 0x01) {
        mask = 0x0f;
        penBits = pen;
    } else {
        mask = 0xf0;
        penBits = pen << 4;
    }
    for (uint i=y0; i<=y1; i++) {
        *frame = (*frame & mask) | penBits;
        frame += rowBytes;
    }
    return NULL;
}

inline void _lineq0(GVga *gvga, uint dx, uint dy, uint offset, uint mask, int rowBytes, uint penBits, uint penReset) {
    uint frameBytes = gvga->frameBytes;
    uint8_t *frame = &gvga->drawFrame[offset];
    if (dx > dy) {
        uint m = (dy << FIXED_POINT_CONSTANT) / dx;
        uint e = 0;
        for (int i=0; i<dx; i++) {
            frame[offset] = (frame[offset] & mask) | penBits;
            mask >>= 4;
            penBits >>= 4;
            if (mask == 0) {
                mask = 0xf0;
                penBits = penReset;
                offset++;
                if (offset > frameBytes) return;
            }
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
            frame[offset] = (frame[offset] & mask) | penBits;
            offset += rowBytes;
            if (offset > frameBytes) return;
            e += m;
            if (e >> FIXED_POINT_CONSTANT) {
                e -= 1 << FIXED_POINT_CONSTANT;
                mask >>= 4;
                if (mask == 0) {
                    mask = 0xf0;
                    penBits = penReset;
                    offset++;
                    if (offset > frameBytes) return;
                }
            }
        }
    }
}

static void *line(GVga *gvga, uint x0, uint y0, uint x1, uint y1, uint pen) {
    pen &= 0x0f;
    if (x0 >= gvga->width || x1 >= gvga->width || y0 >= gvga->height || y1 >= gvga->height) return NULL;
    int dy = y1 - y0;
    if (dy == 0) return hline(gvga, x0, y0, x1, pen);
    int dx = x1 - x0;
    if (dx == 0) return vline(gvga, x0, y0, y1, pen);
    if (x1 < x0) {
        _swap(x0, x1);
        _swap(y0, y1);
    }
    uint mask;
    uint penBits;
    if (x0 & 0x01) {
        mask = 0x0f;
        penBits = pen << 4;
    } else {
        mask = 0xf0;
        penBits = pen;
    }
    uint offset = _offset(gvga, x0, y0);
    if (dx * dy > 0) {
        _lineq0(gvga, _abs(dx), _abs(dy), offset, mask, gvga->rowBytes, penBits, pen << 4);
    } else {
        _lineq0(gvga, _abs(dx), _abs(dy), offset, mask, -gvga->rowBytes, penBits, pen << 4);
    }
    return NULL;
}

static void *clear(GVga *gvga, uint pen) {
    pen &= 0x0f;
    memset(gvga->drawFrame, pen << 4 | pen, gvga->frameBytes);
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

uint8_t _bitQuadrupler[4] = {0, 0x0f, 0xf0, 0xff};
static void *drawText(GVga *gvga, uint x, uint y, char *text, uint pen, uint bgnd) {
    uint rowBytes = gvga->rowBytes;
    uint rowBytes_4 = rowBytes - 4;
    uint row = y * rowBytes;
    uint col = x / gvga->pixelsPerByte;
    uint8_t *fontData = gvga->font->data;
    uint8_t *frame = &gvga->drawFrame[row + col];
    uint penMask = pen | pen << 4;

    while(*text) {
        uint8_t *glyph = &fontData[(*text++) * 8];
        uint8_t *nextFrame = frame + 4;
        for (int i=0; i<8; i++) {
            uint8_t byte = *glyph++;
            *frame++ = (_bitQuadrupler[byte >> 6] & penMask);
            *frame++ = (_bitQuadrupler[(byte >> 4) & 0x03] & penMask);
            *frame++ = (_bitQuadrupler[(byte >> 2) & 0x03] & penMask);
            *frame++ = (_bitQuadrupler[byte & 0x03] & penMask);
            frame += rowBytes_4;
        }
        frame = nextFrame;
    }
    return NULL;
}

static Gfx _gfx_4bpp = {
    .set = set,
    .hline = hline,
    .vline = vline,
    .line = line,
    .clear = clear,
    .rect = rect,
    .rectFill = rectFill,
    .drawText = drawText
};

Gfx *gfx_4bpp = &_gfx_4bpp;