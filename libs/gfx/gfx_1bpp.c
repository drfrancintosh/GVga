#include "gfx.h"
#include "gvga.h"
#include <stdlib.h>
#include <string.h>
// #include "c16.h"

#define _PPB 8
#define _PPB_SHIFT 3
#define FIXED_POINT_CONSTANT 15

static uint _bit_mask[] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};
static uint _hline_bits_left[] = {0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x01};
static uint _hline_bits_right[] = {0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff};

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

#define _swap(a, b)\
    tmp = a;\
    a = b;\
    b = tmp;\

inline uint _offset(GVga *gvga, uint x, uint y) {
    return y * gvga->rowBytes + (x >> _PPB_SHIFT);
}

static void *_set(GVga *gvga, uint offset, uint mask) {
    gvga->drawFrame[offset] |= mask;
    return NULL;
}
static void *_reset(GVga *gvga, uint offset, uint mask) {
    gvga->drawFrame[offset] &= ~mask;
    return NULL;
}

static void *set(GVga *gvga, uint x, uint y, uint pen) {
    uint offset = _offset(gvga, x, y);
    uint mask = _bit_mask[x & 0x07];
    if (pen) {
        _set(gvga, offset, mask);
    } else {
        _reset(gvga, offset, mask);
    }
    return NULL;
}

static void *hline(GVga *gvga, uint x0, uint y, uint x1, uint pen) {
    if (x0 > x1) {
        uint tmp;
        _swap(x0, x1);
    }
    uint offset0 = _offset(gvga, x0, y);
    uint offset1 = _offset(gvga, x1, y);
    void * (*_point)(GVga *gvga, uint offset, uint mask) = pen ? _set : _reset;

    if (offset0 == offset1) {
        uint mask = _hline_bits_left[x0 & 0x07] & _hline_bits_right[x1 & 0x07];
        return _point(gvga, offset0, mask);
    }
    _point(gvga, offset0, _hline_bits_left[x0 & 0x07]);
    _point(gvga, offset1, _hline_bits_right[x1 & 0x07]);
    offset0++;
    uint bytes = offset1 - offset0 ;
    if (bytes > 0) {
        memset(&gvga->drawFrame[offset0], pen ? 0xff : 0x00, bytes);
    }
    return NULL;
}

static void *vline(GVga *gvga, uint x, uint y0, uint y1, uint pen) {
    if (y0 > y1) {
        uint tmp;
        _swap(y0, y1);
    }
    uint lines = y1 - y0;
    uint offset = _offset(gvga, x, y0);
    uint mask = _bit_mask[x & 0x07];
    uint rowBytes = gvga->rowBytes;
    void * (*_point)(GVga *gvga, uint offset, uint mask) = pen ? _set : _reset;
    for(uint i=0; i<=lines; i++) {
        _point(gvga, offset, mask);
        offset += rowBytes;
    }
    return NULL;
}

inline void _lineq0(GVga *gvga, int dx, int dy, uint offset, uint mask, int rowBytes, void * (*_point)(GVga *gvga, uint offset, uint mask)) {
    uint frameBytes = gvga->frameBytes;
    if (dx > dy) {
        uint m = (dy << FIXED_POINT_CONSTANT) / dx;
        uint e = 0;
        for (int i=0; i<dx; i++) {
            _point(gvga, offset, mask);
            mask >>= 1;
            if (mask == 0) {
                mask = 0x80;
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
            _point(gvga, offset, mask);
            offset += rowBytes;
            if (offset > frameBytes) return;
            e += m;
            if (e >> FIXED_POINT_CONSTANT) {
                e -= 1 << FIXED_POINT_CONSTANT;
                mask >>= 1;
                if (mask == 0) {
                    mask = 0x80;
                    offset++;
                    if (offset > frameBytes) return;
                }
            }
        }
    }
}

static void *line(GVga *gvga, uint x0, uint y0, uint x1, uint y1, uint pen) {
    uint tmp;
    if (x0 >= gvga->width || x1 >= gvga->width || y0 >= gvga->height || y1 >= gvga->height) return NULL;
    int dy = y1 - y0;
    if (dy == 0) return hline(gvga, x0, y0, x1, pen);
    int dx = x1 - x0;
    if (dx == 0) return vline(gvga, x0, y0, y1, pen);
    void * (*_point)(GVga *gvga, uint offset, uint mask) = pen ? _set : _reset;
    if (x1 < x0) {
        _swap(x0, x1);
        _swap(y0, y1);
    }
    uint offset = _offset(gvga, x0, y0);
    uint mask = _bit_mask[x0 & 0x07];
    if (dx * dy > 0) {
        _lineq0(gvga, _abs(dx), _abs(dy), offset, mask, gvga->rowBytes, _point);
    } else {
        _lineq0(gvga, _abs(dx), _abs(dy), offset, mask, -gvga->rowBytes, _point);
    }
    return NULL;
}

static void *clear(GVga *gvga, uint pen) {
    if (pen) {
        memset(gvga->drawFrame, 0xff, gvga->frameBytes);
    } else {
        memset(gvga->drawFrame, 0x00, gvga->frameBytes);
    }
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

static void *drawText(GVga *gvga, uint x, uint y, char *text, uint pen) {
    uint rowBytes = gvga->rowBytes;
    uint row = y * rowBytes;
    uint col = x / gvga->pixelsPerByte;
    uint8_t *fontData = gvga->font->data;
    uint8_t *frame = &gvga->drawFrame[row + col];
    while(*text) {
        uint8_t *glyph = &fontData[(*text++) * 8];
        uint8_t *nextFrame = frame + 1;
        for (int i=0; i<8; i++) {
            if (pen) *frame = (*glyph++);
            else *frame = ~(*glyph++);
            // *frame = *glyph++;
            frame += rowBytes;
        }
        frame = nextFrame;
    }
    return NULL;
}

static Gfx _gfx_1bpp = {
    .set = set,
    .hline = hline,
    .vline = vline,
    .line = line,
    .clear = clear,
    .rect = rect,
    .rectFill = rectFill,
    .drawText = drawText
};

Gfx *gfx_1bpp = &_gfx_1bpp;