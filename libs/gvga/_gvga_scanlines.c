#include "gvga.h"
#include "_gvga.h"

inline void push0(uint32_t *buf, uint16_t value) {
    buf[_gvga_bufptr] = value;
}

inline void push1(uint32_t *buf, uint16_t value) {
    buf[_gvga_bufptr++] |= (value << 16);
}

inline void push32(uint32_t *buf, uint16_t value) {
    buf[_gvga_bufptr] |= value << 16;
}


static int32_t __time_critical_func(_scanline_render_1bpp)(uint32_t *buf, size_t buf_length, int width, int scanline) {
    _gvga_bufptr = 0;
    uint8_t *row = &gvga->showFrame[scanline * width / _8_PIXELS_PER_BYTE];
    uint16_t *colors = &_gvga_paletteBuf[(*row++) * _8_PIXELS_PER_BYTE];
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
    for(int pixel=_8_PIXELS_PER_BYTE; pixel < width; pixel+=_8_PIXELS_PER_BYTE) {
        colors = &_gvga_paletteBuf[(*row++) * _8_PIXELS_PER_BYTE];
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

static int32_t __time_critical_func(_scanline_render_2bpp)(uint32_t *buf, size_t buf_length, int width, int scanline) {
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
        colors = &_gvga_paletteBuf[(*row++) * _4_PIXELS_PER_BYTE];
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

static int32_t __time_critical_func(_scanline_render_2bpp_interlaced)(uint32_t *buf, size_t buf_length, int width, int scanline) {
    _gvga_bufptr = 0;
    uint8_t *row = &gvga->showFrame[scanline * width / _4_PIXELS_PER_BYTE];
    uint8_t *end = row + width / _4_PIXELS_PER_BYTE;
    if ((_gvga_oddFrame && (scanline % 2 == 1)) || (!_gvga_oddFrame && (scanline % 2 == 0))) {
        uint16_t *colors = &_gvga_paletteBuf[(*row++) * _4_PIXELS_PER_BYTE];
        push0(buf, COMPOSABLE_RAW_RUN);
        push1(buf, *colors++);
        push0(buf, width-3);
        push1(buf, *colors++);
        push0(buf, *colors++);
        push1(buf, *colors++);
        while(row < end) {
            colors = &_gvga_paletteBuf[(*row++) * _4_PIXELS_PER_BYTE];
            push0(buf, *colors++);
            push1(buf, *colors++);
            push0(buf, *colors++);
            push1(buf, *colors++);
        }
    } else {
        push0(buf, COMPOSABLE_COLOR_RUN);
        push1(buf, gvga->palette[0]); // a line of background color
        push0(buf, width-5);
        push1(buf, COMPOSABLE_RAW_2P); 
        push0(buf, gvga->palette[0]); 
        push1(buf, gvga->palette[0]); 
    }
    push0(buf, COMPOSABLE_RAW_1P);
    push1(buf, 0);
    // note we must end with a black pixel
    push32(buf, COMPOSABLE_EOL_ALIGN);
    return _gvga_bufptr;
}

static int32_t __time_critical_func(_scanline_render_4bpp)(uint32_t *buf, size_t buf_length, int width, int scanline) {
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

static int32_t __time_critical_func(_scanline_render_4bpp_interlaced)(uint32_t *buf, size_t buf_length, int width, int scanline) {
    _gvga_bufptr = 0;
    uint8_t *row = &gvga->showFrame[scanline * width / _2_PIXELS_PER_BYTE];
    uint8_t *end = row + width / _2_PIXELS_PER_BYTE;
    if ((_gvga_oddFrame && (scanline % 2 == 1)) || (!_gvga_oddFrame && (scanline % 2 == 0))) {
        uint8_t byte = *row++;
        uint16_t *colors = &_gvga_paletteBuf[byte * _2_PIXELS_PER_BYTE];
        push0(buf, COMPOSABLE_RAW_RUN);
        push1(buf, *colors++);
        push0(buf, width-3);
        push1(buf, *colors++);
        while(row < end) {
            byte = *row++;
            colors = &_gvga_paletteBuf[byte * _2_PIXELS_PER_BYTE];
            push0(buf, *colors++);
            push1(buf, *colors++);
        }
    } else {
        push0(buf, COMPOSABLE_COLOR_RUN);
        push1(buf, gvga->palette[0]); // a line of background color
        push0(buf, width-5);
        push1(buf, COMPOSABLE_RAW_2P); 
        push0(buf, gvga->palette[0]); 
        push1(buf, gvga->palette[0]); 
    }
    push0(buf, COMPOSABLE_RAW_1P);
    push1(buf, 0);
    // note we must end with a black pixel
    push32(buf, COMPOSABLE_EOL_ALIGN);
    return _gvga_bufptr;
}

static int32_t __time_critical_func(_scanline_render_8bpp_interlaced)(uint32_t *buf, size_t buf_length, int width, int scanline) {
    _gvga_bufptr = 0;
    uint8_t *row = &gvga->showFrame[scanline * width];
    uint8_t *end = row + width;
    if ((_gvga_oddFrame && (scanline % 2 == 1)) || (!_gvga_oddFrame && (scanline % 2 == 0))) {
        GVgaColor *palette = gvga->palette;
        push0(buf, COMPOSABLE_RAW_RUN);
        push1(buf, palette[*row++]);
        push0(buf, width-3);
        push1(buf, palette[*row++]);
        while (row < end) {
            push0(buf, palette[*row++]);
            push1(buf, palette[*row++]);
        }
    } else {
        push0(buf, COMPOSABLE_COLOR_RUN);
        push1(buf, gvga->palette[0]); // a line of background color
        push0(buf, width-5);
        push1(buf, COMPOSABLE_RAW_2P); 
        push0(buf, gvga->palette[0]); 
        push1(buf, gvga->palette[0]); 
    }
    push0(buf, COMPOSABLE_RAW_1P);
    push1(buf, 0);
    // note we must end with a black pixel
    push32(buf, COMPOSABLE_EOL_ALIGN);
    return _gvga_bufptr;
}

static int32_t __time_critical_func(_scanline_render_8bpp)(uint32_t *buf, size_t buf_length, int width, int scanline) {
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

static int32_t __time_critical_func(_scanline_render_blank_line)(uint32_t *buf, size_t buf_length, int width, int scanline, GVgaColor color) {
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

static void __time_critical_func(render_loop)() {
    bool isBlocked = false;
    while (true) {
        struct scanvideo_scanline_buffer *dest = scanvideo_begin_scanline_generation(true);
        uint32_t *buf = dest->data;
        size_t buf_length = dest->data_max;
        uint16_t framenumber = scanvideo_frame_number(dest->scanline_id);
        uint16_t scanline = scanvideo_scanline_number(dest->scanline_id);
        _gvga_oddFrame = framenumber & 1;
        if (scanline == 0 && !isBlocked) {
            mutex_enter_blocking(gvga->scanning_mutex);
            isBlocked = true;
        }
        if (scanline < gvga->headerRows) {
            dest->data_used = _scanline_render_blank_line(buf, buf_length, gvga->width, scanline, gvga->borderColors[GVGA_TOP]);
        } else if (scanline < gvga->height + gvga->headerRows) {
            dest->data_used = gvga->scanline_render(buf, buf_length, gvga->width, scanline - gvga->headerRows);
        } else {
            dest->data_used = _scanline_render_blank_line(buf, buf_length, gvga->width, scanline, gvga->borderColors[GVGA_BOTTOM]);
        }
        dest->status = SCANLINE_OK;
        scanvideo_end_scanline_generation(dest);
        if ((scanline >= gvga->height - 1) && isBlocked) {
            mutex_exit(gvga->scanning_mutex);
            isBlocked = false;
        }
    }
}

