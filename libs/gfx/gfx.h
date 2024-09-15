#include "gvga.h"

extern void gfx_set(GVga *gvga, int x, int y, int color);
extern void gfx_line(GVga *gvga, int x0, int y0, int x1, int y1, int color);
extern void gfx_box(GVga *gvga, int x0, int y0, int x1, int y1, int color);
extern void gfx_clear(GVga *gvga, int color);
extern void gfx_box_fill(GVga *gvga, int x0, int y0, int x1, int y1, int color);
