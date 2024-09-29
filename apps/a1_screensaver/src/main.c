#include <stdio.h>

#include "pico/rand.h"
#include "pico/stdlib.h"

#include "gvga.h"
#include "gfx.h"

/**
 * classic screensaver program demonstrating the GVga library
**/

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define SCREEN_BITS 2
#define SCREEN_COLORS (1 << SCREEN_BITS)
#define DOUBLE_BUFFER true
#define INTERLACED false
#define SYNC false

#define LINE_COUNT 256

Gfx *gfx = NULL;

const uint BOARD_LED_PIN = 25; // Example: GPIO 25, which is connected to the onboard LED
int state = 1;

int min(int a, int b) {
    return a < b ? a : b;
}

uint32_t msNow() {
	return to_ms_since_boot(get_absolute_time());
}

static void _init_led() {
	// heartbeat led
	gpio_init(BOARD_LED_PIN);
	gpio_set_dir(BOARD_LED_PIN, GPIO_OUT);
}

static bool _blink_led(int rate) {
    static uint32_t _msLast = 0;
    static int _state = 1;
	uint32_t ms_since_boot = to_ms_since_boot(get_absolute_time());
	if (ms_since_boot - _msLast < rate) return false;
	_msLast = ms_since_boot;
	gpio_put(BOARD_LED_PIN, _state % 2);
	_state++;
	return true;
}


uint32_t rnd_non_zero(int min, int max) {
    uint32_t result = 0;
    while(result == 0) {
        uint32_t random = get_rand_32();
        result = (random % (max - min + 1)) + min;
    }
    return result;
}

typedef struct {
    int x1, y1, x2, y2;
    int dx1, dy1, dx2, dy2;
    int color;
} Line;

Line lines[LINE_COUNT];

void init_line(Line *line) {
    line->x1 = rnd_non_zero(0, SCREEN_WIDTH - 1);
    line->y1 = rnd_non_zero(0, SCREEN_HEIGHT - 1);
    line->x2 = rnd_non_zero(0,  SCREEN_WIDTH - 1);
    line->y2 = rnd_non_zero(0, SCREEN_HEIGHT - 1);
    line->dx1 = rnd_non_zero(-10, 10);
    line->dy1 = rnd_non_zero(-10, 10);
    line->dx2 = rnd_non_zero(-10, 10);
    line->dy2 = rnd_non_zero(-10, 10);
    line->color = rnd_non_zero(1, SCREEN_COLORS - 1);
}

void move_line(Line *line) {
    line->x1 += line->dx1;
    line->y1 += line->dy1;
    line->x2 += line->dx2;
    line->y2 += line->dy2;

    if (line->x1 < 0 || line->x1 >= SCREEN_WIDTH) {
        line->x1 = line->x1 < 0 ? 0 : SCREEN_WIDTH - 1;
        line->dx1 = -line->dx1;
    }
    if (line->y1 < 0 || line->y1 >= SCREEN_HEIGHT) {
        line->y1 = line->y1 < 0 ? 0 : SCREEN_HEIGHT - 1;
        line->dy1 = -line->dy1;
    }
    if (line->x2 < 0 || line->x2 >= SCREEN_WIDTH) {
        line->x2 = line->x2 < 0 ? 0 : SCREEN_WIDTH - 1;
        line->dx2 = -line->dx2;
    }
    if (line->y2 < 0 || line->y2 >= SCREEN_HEIGHT) {
        line->y2 = line->y2 < 0 ? 0 : SCREEN_HEIGHT - 1;
        line->dy2 = -line->dy2;
    }
}


void draw_lines(GVga *gvga) {
    for (int i = 0; i < LINE_COUNT; i++) {
        gfx->line(gvga, lines[i].x1, lines[i].y1, lines[i].x2, lines[i].y2, lines[i].color);
    }
}

void move_lines() {
    for (int i = LINE_COUNT - 1; i > 0; i--) {
        lines[i] = lines[i-1];
        lines[i].color = (lines[i].color + 1) % SCREEN_COLORS;
    }
    move_line(&lines[0]);
}

void init_screensaver() {
    init_line(&lines[0]);
    move_line(&lines[0]);
}

static uint dx = 0;
static uint dy = 1;
static uint x0 = 0;
static uint y0 = 0;
static uint x1 = SCREEN_WIDTH - 1;
static uint y1 = SCREEN_HEIGHT - 1;

void screensaver(GVga *gvga) {
	gfx->clear(gvga, 0);
    for (int i=0; i<10; i+=2) {
	    gfx->rect(gvga, i, i, SCREEN_WIDTH - i - 1, SCREEN_HEIGHT - i - 1, 1);
    }
    gfx->line(gvga, x0, y0, x1, y1, 1);
    x0 += dx;
    y0 += dy;
    x1 -= dx;
    y1 -= dy;
    if (y0 == SCREEN_HEIGHT) {
        dx = 1;
        dy = 0;
        y0 = SCREEN_HEIGHT - 1;
        y1 = 0;
    }
    if (x0 == SCREEN_WIDTH) {
        dx = 0;
        dy = -1;
        x0 = SCREEN_WIDTH - 1;
        x1 = 0;
    }
    if (y1 == SCREEN_HEIGHT) {
        dx = -1;
        dy = 0;
        y0 = 0;
        y1 = SCREEN_HEIGHT - 1;
    }
    if (x1 == SCREEN_WIDTH) {
        dx = 0;
        dy = 1;
        x0 = 0;
        x1 = SCREEN_WIDTH - 1;
    }
	move_lines(); 
    draw_lines(gvga);
}

void nerdStats(GVga *gvga, uint16_t x, uint16_t y, uint16_t fps, uint32_t msPerFrame, uint16_t pen) {
    char buf[80];
    sprintf(buf, "FPS=%d ms/Frame=%ld, (lines=%d pen=%d)", fps, msPerFrame, LINE_COUNT, pen);
    gfx->drawText(gvga, x, y += 10, buf, pen);
    sprintf(buf, "dblbuf=%b interlace=%d sync=%d", DOUBLE_BUFFER, INTERLACED, SYNC);
    gfx->drawText(gvga, x, y += 10, buf, pen);
    sprintf(buf, "(w=%d, h=%d, b=%d mem=%d)", gvga->width, gvga->height, gvga->bits, gvga->rowBytes * gvga->height * (DOUBLE_BUFFER ? 2 : 1));
    gfx->drawText(gvga, x, y += 10, buf, pen);
}

int main() {
    uint16_t pen = 0;
    uint16_t fps = 0;
    uint32_t msPerFrame = 0;
    uint32_t now = msNow();

	stdio_init_all();
	printf("\nGVga test\n");
    _init_led();

    gfx = gfx_1bpp;

	GVgaColor palette[8] = {GVGA_WHITE, GVGA_RED, GVGA_GREEN, GVGA_BLUE, GVGA_YELLOW, GVGA_CYAN, GVGA_MAGENTA, GVGA_BLACK };
	GVga *gvga = gvga_init(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BITS, DOUBLE_BUFFER, INTERLACED, NULL);
	gvga_setPalette(gvga, palette, 0, min(8, SCREEN_COLORS));
    // gvga_setBorderColors(gvga, GVGA_RED, GVGA_GREEN, GVGA_BLACK, GVGA_BLACK);
	gvga_start(gvga);
	gfx->rect(gvga, 10, 10, SCREEN_WIDTH - 10, SCREEN_HEIGHT - 10, 1);

	init_screensaver();

	uint32_t then = msNow();
    int frameCounter = 0;
	while(1) {
        _blink_led(100);
        if (SYNC) gvga_sync(gvga);
        msPerFrame = msNow() - now;
        now = msNow();
		screensaver(gvga);
        nerdStats(gvga, (gvga->width - 36 * 8) / 2, (gvga->height - 3 * 12) / 2, fps, msPerFrame, pen);
        frameCounter++;
        if (now - then > 1000) {
            fps = frameCounter;
            frameCounter = 0;
            then = msNow();
            pen = (pen + 1) % SCREEN_COLORS;
            if (pen == 0) pen = 1;
        }
        if (DOUBLE_BUFFER) gvga_swap(gvga, false);
	}
}