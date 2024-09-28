#include "gvga.h"
#include "_gvga.h"
#include "gvga_font.h"

inline void push0(uint32_t *buf, uint16_t value) {
    buf[_gvga_bufptr] = value;
}

inline void push1(uint32_t *buf, uint16_t value) {
    buf[_gvga_bufptr++] |= (value << 16);
}

inline void push32(uint32_t *buf, uint16_t value) {
    buf[_gvga_bufptr] |= value << 16;
}


static int32_t __time_critical_func(_scanline_render_1bpp)(GVga *gvga, uint32_t *buf, size_t buf_length, int width, int scanline) {
    _gvga_bufptr = 0;
    bool isTextMode = gvga->mode & GVGA_MODE_TEXT;
    uint16_t fontHeight = gvga->font->height;
    uint16_t idx = scanline * width / _8_PIXELS_PER_BYTE;
    uint8_t *row = &gvga->showFrame[idx];
    uint16_t fontLine;
    uint8_t *fontData;
    int cols = width / _8_PIXELS_PER_BYTE;
    if (isTextMode) {
        idx = (scanline / fontHeight) * (width / _8_PIXELS_PER_BYTE);
        fontLine = scanline % gvga->font->height;
        fontData = gvga->font->data + fontLine; // NOTE: this is a pointer to the font data for the current line offset by the scanline
        row = &gvga->showFrame[idx];
        cols = gvga->cols;
    }

    uint16_t byte = isTextMode ? fontData[(*row++) * fontHeight]: *row++;
    uint16_t *colors = &_gvga_paletteBuf[byte * _8_PIXELS_PER_BYTE];
    push0(buf, COMPOSABLE_RAW_RUN);
    push1(buf, *colors++);
    push0(buf, width-3);
    push1(buf, *colors++);
    push0(buf, *colors++);
    push1(buf, *colors++);
    push0(buf, *colors++);
    push1(buf, *colors++);
    push0(buf, *colors++);
    push1(buf, *colors++);
    cols--;
    for(int i=0; i<cols; i++) {
        byte = isTextMode ? fontData[(*row++) * 8]: *row++; // (* 8) for timing -> only works for 8 pixel high fonts
        colors = &_gvga_paletteBuf[byte * _8_PIXELS_PER_BYTE];
        push0(buf, *colors++);
        push1(buf, *colors++);
        push0(buf, *colors++);
        push1(buf, *colors++);
        push0(buf, *colors++);
        push1(buf, *colors++);
        push0(buf, *colors++);
        push1(buf, *colors++);
    }
    push0(buf, COMPOSABLE_RAW_1P);
    push1(buf, 0);
    // note we must end with a black pixel
    push32(buf, COMPOSABLE_EOL_ALIGN);
    return _gvga_bufptr;
}

static int32_t __time_critical_func(_scanline_render_2bpp)(GVga *gvga, uint32_t *buf, size_t buf_length, int width, int scanline) {
    _gvga_bufptr = 0;
    uint8_t *row = &gvga->showFrame[scanline * width / _4_PIXELS_PER_BYTE];
    uint16_t *colors = &_gvga_paletteBuf[(*row++) * _4_PIXELS_PER_BYTE];
    push0(buf, COMPOSABLE_RAW_RUN);
    push1(buf, *colors++);
    push0(buf, width-3);
    push1(buf, *colors++);
    push0(buf, *colors++);
    push1(buf, *colors++);
    for(int pixel=_4_PIXELS_PER_BYTE; pixel < width; pixel += _4_PIXELS_PER_BYTE) {
        colors = &_gvga_paletteBuf[(*row++) << 2];
        push0(buf, *colors++);
        push1(buf, *colors++);
        push0(buf, *colors++);
        push1(buf, *colors++);
    }
    push0(buf, COMPOSABLE_RAW_1P);
    push1(buf, 0);
    // note we must end with a black pixel
    push32(buf, COMPOSABLE_EOL_ALIGN);
    return _gvga_bufptr;
}

static int32_t __time_critical_func(_scanline_render_4bpp)(GVga *gvga, uint32_t *buf, size_t buf_length, int width, int scanline) {
    _gvga_bufptr = 0;
    uint8_t *row = &gvga->showFrame[scanline * width / _2_PIXELS_PER_BYTE];
    uint8_t byte = *row++;
    uint16_t *colors = &_gvga_paletteBuf[byte * _2_PIXELS_PER_BYTE];
    push0(buf, COMPOSABLE_RAW_RUN);
    push1(buf, *colors++);
    push0(buf, width-3);
    push1(buf, *colors++);
    for(int pixel=_2_PIXELS_PER_BYTE; pixel < width; pixel+= _2_PIXELS_PER_BYTE) {
        byte = *row++;
        colors = &_gvga_paletteBuf[byte * _2_PIXELS_PER_BYTE];
        push0(buf, *colors++);
        push1(buf, *colors++);
    }
    push0(buf, COMPOSABLE_RAW_1P);
    push1(buf, 0);
    // note we must end with a black pixel
    push32(buf, COMPOSABLE_EOL_ALIGN);
    return _gvga_bufptr;
}

static int32_t __time_critical_func(_scanline_render_8bpp)(GVga *gvga, uint32_t *buf, size_t buf_length, int width, int scanline) {
    _gvga_bufptr = 0;
    uint8_t *row = &gvga->showFrame[scanline * width];
    uint8_t *end = row + width;
    GVgaColor *palette = gvga->palette;
    push0(buf, COMPOSABLE_RAW_RUN);
    push1(buf, palette[*row++]);
    push0(buf, width-3);
    push1(buf, palette[*row++]);
    while (row < end) {
        push0(buf, palette[*row++]);
        push1(buf, palette[*row++]);
    }
    push0(buf, COMPOSABLE_RAW_1P);
    push1(buf, 0);
    // note we must end with a black pixel
    push32(buf, COMPOSABLE_EOL_ALIGN);
    return _gvga_bufptr;
}

static int32_t __time_critical_func(_scanline_render_blank_line)(GVga *gvga, uint32_t *buf, size_t buf_length, int width, int scanline, GVgaColor color) {
    _gvga_bufptr = 0;
    push0(buf, COMPOSABLE_COLOR_RUN);
    push1(buf, color); // a line of background color
    push0(buf, width-5);
    push1(buf, COMPOSABLE_RAW_2P); 
    push0(buf, color); 
    push1(buf, color); 
    push0(buf, COMPOSABLE_RAW_1P);
    push1(buf, 0);
    // note we must end with a black pixel
    push32(buf, COMPOSABLE_EOL_ALIGN);
    return _gvga_bufptr;
}
