#include "gfx.h"
#include "gvga.h"
#include <stdlib.h>
#include <string.h>
// #include "c16.h"

#define _PPB 4
#define _PPB_SHIFT 2
#define FIXED_POINT_CONSTANT 15

static uint _bits_mask[] = {0x3f, 0xcf, 0xf3, 0xfc};
static uint _byte_mask[] = {0x00, 0xc0, 0xf0, 0xfc};
static uint _byte_mask2[] = {0x3c, 0x0f, 0x03, 0x00};
static uint _pen_bits[16] = {
    0x00, 0x40, 0x80, 0xc0,
    0x00, 0x10, 0x20, 0x30,
    0x00, 0x04, 0x08, 0x0c,
    0x00, 0x01, 0x02, 0x03
};
static uint _pen_bits2[16] = {
    0x00, 0x55, 0xaa, 0xff,
    0x00, 0x15, 0x2a, 0x3f,
    0x00, 0x05, 0x0a, 0x0f,
    0x00, 0x01, 0x02, 0x03
};
static uint _pen_bits3[16] = {
    0x00, 0x40, 0x80, 0xc0,
    0x00, 0x50, 0xa0, 0xf0,
    0x00, 0x54, 0xa8, 0xfc,
    0x00, 0x55, 0xaa, 0xff
};

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
    return y * gvga->rowBytes + (x / _PPB);
}

static void *_set(GVga *gvga, uint offset, uint mask, uint penMask) {
    gvga->drawFrame[offset] = gvga->drawFrame[offset] & ~mask | penMask;
    return NULL;
}

static void *set(GVga *gvga, uint x, uint y, uint pen) {
    uint offset = _offset(gvga, x, y);
    uint shift = x & 0x03;
    uint mask = _bits_mask[shift];
    uint penBits = _pen_bits[shift << 2 + (pen & 0x03)];
    gvga->drawFrame[offset] = (gvga->drawFrame[offset] & mask) | penBits;
    return NULL;
}

static void *hline(GVga *gvga, uint x0, uint y, uint x1, uint pen) {
    if (x0 > x1) {
        uint tmp;
        _swap(x0, x1);
    }
    uint offset0 = _offset(gvga, x0, y);
    uint offset1 = _offset(gvga, x1, y);
    uint shift = x0 & 0x03;
    uint mask = _byte_mask[shift];
    uint idx = shift << 2 + (pen & 0x03);
    uint penBits = _pen_bits2[idx];
    gvga->drawFrame[offset0] = (gvga->drawFrame[offset0] & mask) | penBits;
    if (offset0 == offset1) return NULL;
    gvga->drawFrame[offset1] = (gvga->drawFrame[offset0] & mask) | penBits;
    mask = _byte_mask2[shift];
    penBits = _pen_bits3[idx];
    gvga->drawFrame[offset0] = (gvga->drawFrame[offset0] & mask) | penBits;
    penBits = _pen_bits3[12 + (pen & 0x03)];
    offset0++;
    uint bytes = offset1 - offset0 ;
    if (bytes > 0) {
        memset(&gvga->drawFrame[offset0], penBits, bytes);
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
    uint shift = x & 0x03;
    uint mask = _bits_mask[x & 0x03];
    uint rowBytes = gvga->rowBytes;
    uint penBits = _pen_bits[shift << 2 + (pen & 0x03)];
    for(uint i=0; i<=lines; i++) {
        gvga->drawFrame[offset] = (gvga->drawFrame[offset] & mask) | penBits;
        offset += rowBytes;
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
            mask >>= 2;
            penBits >>= 2;
            if (mask == 0) {
                mask = 0xc0;
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
                mask >>= 2;
                if (mask == 0) {
                    mask = 0xc0;
                    penBits = penReset;
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
    if (x1 < x0) {
        _swap(x0, x1);
        _swap(y0, y1);
    }
    uint offset = _offset(gvga, x0, y0);
    uint shift = x0 & 0x03;
    uint mask = _byte_mask[shift];
    pen &= 0x03;
    uint penBits = _pen_bits[shift << 2 + pen];

    if (dx * dy > 0) {
        _lineq0(gvga, _abs(dx), _abs(dy), offset, mask, gvga->rowBytes, penBits, _pen_bits[12 + pen]);
    } else {
        _lineq0(gvga, _abs(dx), _abs(dy), offset, mask, -gvga->rowBytes, penBits, _pen_bits[12 + pen]);
    }
    return NULL;
}

static void *clear(GVga *gvga, uint pen) {
    memset(gvga->drawFrame, _pen_bits3[12 + pen], gvga->frameBytes);
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

static uint8_t _bitDoubler[16] = {0, 3, 12, 15, 48, 51, 60, 63, 192, 195, 204, 207, 240, 243, 252, 255};
static void *drawText(GVga *gvga, uint x, uint y, char *text, uint pen, uint bgnd) {
    uint rowBytes = gvga->rowBytes;
    uint rowBytes_2 = rowBytes - 2;
    uint row = y * rowBytes;
    uint col = x / gvga->pixelsPerByte;
    uint8_t *fontData = gvga->font->data;
    uint8_t *frame = &gvga->drawFrame[row + col];
    uint penMask = pen | pen << 2 | pen << 4 | pen << 6;

    while(*text) {
        uint8_t *glyph = &fontData[(*text++) * 8];
        uint8_t *nextFrame = frame + 2;
        for (int i=0; i<8; i++) {
            uint8_t byte = *glyph++;
            *frame++ = (_bitDoubler[byte / 16] & penMask);
            *frame++ = (_bitDoubler[byte % 16] & penMask);
            frame += rowBytes_2;
        }
        frame = nextFrame;
    }
    return NULL;
}

static Gfx _gfx_2bpp = {
    .set = set,
    .hline = hline,
    .vline = vline,
    .line = line,
    .clear = clear,
    .rect = rect,
    .rectFill = rectFill,
    .drawText = drawText
};

Gfx *gfx_2bpp = &_gfx_2bpp;
