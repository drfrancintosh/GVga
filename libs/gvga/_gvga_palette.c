#include "gvga.h"
#include "_gvga.h"

static uint16_t _gvga_paletteBuf[256 * 8];

static GVgaColor _gvga_palette16[16] = {
    GVGA_BLACK, GVGA_RED, // 1-bit colors
    GVGA_GREEN, GVGA_BLUE, // additional colors for 2-bits
    GVGA_YELLOW, GVGA_CYAN, GVGA_VIOLET, GVGA_WHITE, // additional colors for 3-bits
    GVGA_COLOR(15, 15, 15), GVGA_COLOR(15, 0, 0), GVGA_COLOR(0, 15, 0), GVGA_COLOR(0, 0, 15), // additional colors for 4-bits (half-bright)
    GVGA_COLOR(15, 15, 0), GVGA_COLOR(0, 15, 15), GVGA_COLOR(15, 15, 0), GVGA_COLOR(7, 7, 7)};// additional colors for 4-bits (half-bright)

static void _gvga_palette_1bpp(GVga *gvga) {
    for(uint i=0; i<256; i++) {
        for(uint j=0; j<8; j++) {
            uint bit = 1 << (7-j);
            uint index = (i & bit) ? 1 : 0;
            _gvga_paletteBuf[i * 8 + j] = gvga->palette[index];
        }
    }
}

static void _gvga_palette_2bpp(GVga *gvga) {
    for(uint i=0; i<256; i++) {
        uint chew0 = (i & 0xc0) >> 6;
        uint chew1 = (i & 0x30) >> 4;
        uint chew2 = (i & 0x0c) >> 2;
        uint chew3 = (i & 0x03);
        _gvga_paletteBuf[i * 4 + 0] = gvga->palette[chew0];
        _gvga_paletteBuf[i * 4 + 1] = gvga->palette[chew1];
        _gvga_paletteBuf[i * 4 + 2] = gvga->palette[chew2];
        _gvga_paletteBuf[i * 4 + 3] = gvga->palette[chew3];
    }
}

static  void _gvga_palette_4bpp(GVga *gvga) {
    for(uint i=0; i<256; i++) {
        uint nybble0 = (i & 0xf0) >> 4;
        uint nybble1 = i & 0x0f;
        _gvga_paletteBuf[i * _2_PIXELS_PER_BYTE + 0] = gvga->palette[nybble0];
        _gvga_paletteBuf[i * _2_PIXELS_PER_BYTE + 1] = gvga->palette[nybble1];
    }
}
