#include "gvga.h"
#include "gfx.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include "ls7447.h"

/**
 * simple hello world program demonstrating the GVga library
 */

const uint BOARD_LED_PIN = 25; // Example: GPIO 25, which is connected to the onboard LED
int _state = 1;
uint32_t _msLast;
static GVgaColor _palette[] = {
	GVGA_BLUE, GVGA_COLOR(16, 28, 28), GVGA_GREEN, GVGA_BLUE, GVGA_YELLOW, GVGA_CYAN, GVGA_VIOLET, GVGA_BLACK,
	GVGA_COLOR(15, 15, 15), GVGA_COLOR(0, 15, 0), GVGA_COLOR(0, 15, 0), GVGA_COLOR(0, 0, 15),
	GVGA_COLOR(15, 15, 0), GVGA_COLOR(0, 15, 15), GVGA_COLOR(15, 15, 0), GVGA_COLOR(7, 7, 7)
};


static void _init_led() {
	// heartbeat led
	gpio_init(BOARD_LED_PIN);
	gpio_set_dir(BOARD_LED_PIN, GPIO_OUT);
}

static bool _blink_led(int rate) {
	uint32_t ms_since_boot = to_ms_since_boot(get_absolute_time());
	if (ms_since_boot - _msLast < rate) return false;
	_msLast = ms_since_boot;
	gpio_put(BOARD_LED_PIN, _state % 2);
	_state++;
	return true;
}

int main() {
	stdio_init_all();
	printf("\nGVga test\n");

	int width = 320;
	int height = 240;
	int bits = 1;
	bool doubleBuffer = false;
	bool interlaced = false;

	_init_led();

	GVga *gvga = gvga_init(width, height, bits, doubleBuffer, interlaced, NULL); // note: double-buffering enabled
	if (bits < 8) gvga_setPalette(gvga, _palette, 0, gvga->colors);
	else gvga_setPalette(gvga, _palette, 0, 16);
	gvga_start(gvga);

	int y = 0;

	gfx_text(gvga, 32, y, "**** COMMODORE 64 BASIC V2 ****", 0);
	y += 16;
	for(int row = 0; row < 16; row++) {
		for(int col = 0; col < 16; col++) {
			gfx_char(gvga, col * 16 + 32, y, row * 16 + col, 0);
		}
		y+=12;
	}
	gfx_text(gvga, 32, y, " !\"#$%&'()*+,-./0123456789:;<=>?", 0);
	y+=8;
	gfx_text(gvga, 32, y, "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_", 0);
	y+=8;
	gfx_text(gvga, 32, y, "`abcdefghijklmnopqrstuvwxyz{|}~\x7f", 0);

	while(true) {
		if (!_blink_led(100)) continue;
		gvga_sync(gvga); // wait for the other core to finish displaying the frame buffer
		gvga_swap(gvga, false); // double-buffering with/without copy
	}
}