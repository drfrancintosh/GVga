#include "gvga.h"
typedef unsigned int uint;

extern void gfx_set(GVga *gvga, int x, int y, uint16_t pen);
extern void gfx_line(GVga *gvga, int x0, int y0, int x1, int y1, uint16_t pen);
extern void gfx_box(GVga *gvga, int x0, int y0, int x1, int y1, uint16_t pen);
extern void gfx_clear(GVga *gvga, uint16_t color);
extern void gfx_box_fill(GVga *gvga, int x0, int y0, int x1, int y1, uint16_t pen);
extern void gfx_char(GVga *gvga, uint16_t x, uint16_t y, uint8_t c, uint16_t color);
extern void gfx_text(GVga *gvga, uint16_t x, uint16_t y, char *text, uint16_t pen);

typedef struct Gfx {
    void *(*set)(GVga *gvga, uint x, uint y, uint pen);
    void *(*hline)(GVga *gvga, uint x0, uint y0, uint x1, uint pen);
    void *(*vline)(GVga *gvga, uint x0, uint y0, uint x1, uint pen);
    void *(*line)(GVga *gvga, uint x0, uint y0, uint x1, uint y1, uint pen);
    void *(*clear)(GVga *gvga, uint pen);
    void *(*rect)(GVga *gvga, uint x0, uint y0, uint x1, uint y1, uint pen);
    void *(*rectFill)(GVga *gvga, uint x0, uint y0, uint x1, uint y1, uint pen);
    void *(*drawText)(GVga *gvga, uint x, uint y, char *text, uint pen, uint bgnd);
} Gfx;

extern  Gfx *gfx_1bpp;
extern  Gfx *gfx_2bpp;