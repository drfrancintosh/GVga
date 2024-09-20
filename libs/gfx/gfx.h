#include "gvga.h"

extern void gfx_set(GVga *gvga, int x, int y, int color);
extern void gfx_line(GVga *gvga, int x0, int y0, int x1, int y1, int color);
extern void gfx_box(GVga *gvga, int x0, int y0, int x1, int y1, int color);
extern void gfx_clear(GVga *gvga, int color);
extern void gfx_box_fill(GVga *gvga, int x0, int y0, int x1, int y1, int color);
extern void gfx_char(GVga *gvga, uint16_t x, uint16_t y, uint8_t c, int color);
extern void gfx_text(GVga *gvga, uint16_t x, uint16_t y, char *text, int color);
