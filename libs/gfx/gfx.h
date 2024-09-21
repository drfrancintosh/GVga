#include "gvga.h"

extern void gfx_set(GVga *gvga, int x, int y, uint16_t pen);
extern void gfx_line(GVga *gvga, int x0, int y0, int x1, int y1, uint16_t pen);
extern void gfx_box(GVga *gvga, int x0, int y0, int x1, int y1, uint16_t pen);
extern void gfx_clear(GVga *gvga, uint16_t color);
extern void gfx_box_fill(GVga *gvga, int x0, int y0, int x1, int y1, uint16_t pen);
extern void gfx_char(GVga *gvga, uint16_t x, uint16_t y, uint8_t c, uint16_t color);
extern void gfx_text(GVga *gvga, uint16_t x, uint16_t y, char *text, uint16_t pen);
