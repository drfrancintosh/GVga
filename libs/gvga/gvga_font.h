#pragma once
#include <stdint.h>

typedef struct GVgaFont {
    uint8_t width;
    uint8_t height;
    uint8_t firstChar;
    uint8_t lastChar;
    uint8_t *data;
} GVgaFont;

extern GVgaFont *gvga_font_init(uint8_t with, uint8_t height, uint8_t firstChar, uint8_t lastChar, uint8_t *data);
extern void gvga_font_destroy(GVgaFont *gvga_font);
extern uint8_t gvga_font_get(GVgaFont *gvga_font, uint8_t c, uint8_t line);


inline uint8_t gvga_font_getLine(GVgaFont *gvga_font, uint8_t c, uint8_t line) {
    return gvga_font->data[c * gvga_font->height + line];
}
