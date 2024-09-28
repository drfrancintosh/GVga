#include "gvga_font.h"
#include <stdlib.h>

GVgaFont *gvga_font_init(uint8_t width, uint8_t height, uint8_t firstChar, uint8_t lastChar, uint8_t *data) {
    GVgaFont *gvga_font = calloc(1, sizeof(GVgaFont));
    if (gvga_font == 0) return 0;
    gvga_font->width = width;
    gvga_font->height = height;
    gvga_font->firstChar = firstChar;
    gvga_font->lastChar = lastChar;
    gvga_font->data = data;
    return gvga_font;
}

uint8_t gvga_font_get(GVgaFont *gvga_font, uint8_t c, uint8_t line) {
    if (c < gvga_font->firstChar || c > gvga_font->lastChar) return 0;
    return gvga_font->data[(c - gvga_font->firstChar) * gvga_font->height + line];
}

void gvga_font_destroy(GVgaFont *gvga_font) {
    free(gvga_font);
}