#include "gfx.h"
#include "gvga.h"
#include <stdlib.h>
#include <string.h>
// #include "c16.h"

#define _PPB 4
#define _PPB_SHIFT 2
#define FIXED_POINT_CONSTANT 15

static uint _byte_mask[] = {0xc0, 0x30, 0x0c, 0x03};
static uint _pen_bits[16] = {
    0x00, 0x40, 0x80, 0xc0,
    0x00, 0x10, 0x20, 0x30,
    0x00, 0x04, 0x08, 0x0c,
    0x00, 0x01, 0x02, 0x03
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

static void *set(GVga *gvga, uint x, uint y, uint pen) {
    pen = pen & 0x03;
    uint offset = _offset(gvga, x, y);
    uint shift = x & 0x03;
    uint mask = _byte_mask[shift];
    uint penBits = _pen_bits[(shift << 2) + pen];
    gvga->drawFrame[offset] = (gvga->drawFrame[offset] & ~mask) | penBits;
    return NULL;
}

static void *hline(GVga *gvga, uint x0, uint y, uint x1, uint pen, uint bgnd) {
    if (x0 > x1) {
        uint tmp;
        _swap(x0, x1);
    }
    pen &= 0x03;
    uint frameBytes = gvga->frameBytes;
    uint offset = _offset(gvga, x0, y);
    uint shift = x0 & 0x03;
    uint penBits = _pen_bits[(shift << 2) + pen];
    uint mask = _byte_mask[shift];
    for(int i=x0; i<=x1; i++) {
        gvga->drawFrame[offset] = (gvga->drawFrame[offset] & ~mask) | penBits;
        mask >>= 2;
        penBits >>= 2;
        if (mask == 0) {
            mask = 0xc0;
            penBits = pen << 6;
            offset++;
            if (offset > frameBytes) return NULL;
        }
    }
    return NULL;
}

static void *vline(GVga *gvga, uint x, uint y0, uint y1, uint pen) {
    if (y0 > y1) {
        uint tmp;
        _swap(y0, y1);
    }
    pen &= 0x03;
    uint lines = y1 - y0;
    uint offset = _offset(gvga, x, y0);
    uint shift = x & 0x03;
    uint mask = _byte_mask[shift];
    uint rowBytes = gvga->rowBytes;
    uint penBits = _pen_bits[(shift << 2) + pen];
    for(uint i=0; i<=lines; i++) {
        gvga->drawFrame[offset] = (gvga->drawFrame[offset] & ~mask) | penBits;
        offset += rowBytes;
        if (offset > gvga->frameBytes) return NULL;
    }
    return NULL;
}

inline void _lineq0(GVga *gvga, uint dx, uint dy, uint offset, uint mask, uint rowBytes, uint penBits, uint penReset) {
    uint frameBytes = gvga->frameBytes;
    uint8_t *frame = gvga->drawFrame;
    if (dx > dy) {
        uint m = (dy << FIXED_POINT_CONSTANT) / dx;
        uint e = 0;
        for (int i=0; i<dx; i++) {
            frame[offset] = (frame[offset]& ~mask) | penBits;
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
            frame[offset] = (frame[offset] & ~mask) | penBits;
            offset += rowBytes;
            if (offset > frameBytes) return;
            e += m;
            if (e >> FIXED_POINT_CONSTANT) {
                e -= 1 << FIXED_POINT_CONSTANT;
                mask >>= 2;
                penBits >>= 2;
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
    uint penBits = _pen_bits[(shift << 2) + pen];

    if (dx * dy > 0) {
        _lineq0(gvga, _abs(dx), _abs(dy), offset, mask, gvga->rowBytes, penBits, pen << 6);
    } else {
        _lineq0(gvga, _abs(dx), _abs(dy), offset, mask, -gvga->rowBytes, penBits, pen << 6);
    }
    return NULL;
}

static void *clear(GVga *gvga, uint pen) {
    pen &= 0x03;
    uint penBits = pen | pen << 2 | pen << 4 | pen << 6;
    memset(gvga->drawFrame, penBits, gvga->frameBytes);
    return NULL;
}

static void *rect(GVga *gvga, uint x0, uint y0, uint x1, uint y1, uint pen) {
    pen &= 0x03;
    hline(gvga, x0, y0, x1, pen);
    hline(gvga, x0, y1, x1, pen);
    vline(gvga, x0, y0, y1, pen);
    vline(gvga, x1, y0, y1, pen);
    return NULL;
}

static void *rectFill(GVga *gvga, uint x0, uint y0, uint x1, uint y1, uint pen) {
    pen &= 0x03;
    for(int y=y0; y<=y1; y++) {
        hline(gvga, x0, y, x1, pen);
    }
    return NULL;
}

static uint8_t _bitDoubler[16] = {0, 3, 12, 15, 48, 51, 60, 63, 192, 195, 204, 207, 240, 243, 252, 255};
static void *drawText(GVga *gvga, uint x, uint y, char *text, uint pen) {
    pen &= 0x03;
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
            *frame++ = (_bitDoubler[byte / 16]);
            *frame++ = (_bitDoubler[byte % 16]);
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
