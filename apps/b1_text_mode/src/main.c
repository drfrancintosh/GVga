#include "gvga.h"
#include "gfx.h"
#include "pico/stdlib.h"
#include <stdio.h>

/**
 * simple hello world program demonstrating the GVga library
 */

#define C64_CYAN GVGA_COLOR(16, 28, 28)

const uint BOARD_LED_PIN = 25; // Example: GPIO 25, which is connected to the onboard LED
int _state = 1;
uint32_t _msLast;
static GVgaColor _palette[] = {
	GVGA_BLUE, C64_CYAN, GVGA_GREEN, GVGA_BLUE, GVGA_YELLOW, GVGA_CYAN, GVGA_VIOLET, GVGA_BLACK,
	GVGA_COLOR(15, 15, 15), GVGA_COLOR(0, 15, 1), GVGA_COLOR(0, 15, 1), GVGA_COLOR(0, 1, 15),
	GVGA_COLOR(15, 15, 1), GVGA_COLOR(0, 15, 15), GVGA_COLOR(15, 15, 1), GVGA_COLOR(7, 7, 7)
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

static void _center(GVga *gvga, int row, char *text, GVgaColor color) {
	int col = (gvga->cols - strlen(text)) / 2;
	gvga_text(gvga, row, col, text, color);
}

int main() {
	static char buf[32];
	stdio_init_all();
	printf("\nGVga test\n");

	int width = 640;
	int height = 400;
	int bits = 1; // only single-bit implemnted
	bool doubleBuffer = false;
	bool interlaced = false;
	bool sync = false;

	_init_led();

	GVga *gvga = gvga_init(width, height, -bits, doubleBuffer, interlaced, NULL); // -bits=text mode
	if (bits < 8) gvga_setPalette(gvga, _palette, 0, gvga->colors);
	else gvga_setPalette(gvga, _palette, 0, 16);
	gvga_start(gvga);
	// gvga_setBorderColors(gvga, C64_CYAN, C64_CYAN, GVGA_BLACK, GVGA_BLACK);
	_center(gvga, 1, "**** COMMODORE 64 BASIC V2 ****", 1);
	for(int row = 0; row < gvga->rows; row++) {
		sprintf(buf, "%02d", row);
		gvga_text(gvga, row, 0, buf, 1);
	}
	for(int col = 0; col < gvga->cols; col+=10) {
		sprintf(buf, "%02d", col);
		gvga_text(gvga, 0, col, buf, 1);
	}
	sprintf(buf, "%2d", gvga->cols);
	gvga_text(gvga, 0, gvga->cols - 2, buf, 1);
	for(int row = 0; row < 16; row++) {
		for(int col = 0; col < 16; col++) {
			buf[0] = row * 16 + col;
			buf[1] = 0;
			gvga_char(gvga, row + 2, col+3, row * 16 + col, 1);
			gvga_text(gvga, row + 2, gvga->cols-18+col, buf, 1);
		}
	}
	int row = 19;
	int col = (gvga->cols - 16) / 2;
	gvga_text(gvga, row++, col, " !\"#$%&'()*+,-./", 1);
	gvga_text(gvga, row++, col, "0123456789:;<=>?", 1);
	gvga_text(gvga, row++, col, "@ABCDEFGHIJKLMNO", 1);
	gvga_text(gvga, row++, col, "PQRSTUVWXYZ[\\]^_", 1);
	gvga_text(gvga, row++, col, "`abcdefghijklmno", 1);
	gvga_text(gvga, row++, col, "pqrstuvwxyz{|}~\x7f", 1);
	while(true) {
		if (!_blink_led(100)) continue;
		// _palette[1] = GVGA_COLOR(rand() & 0x1f, rand() & 0x1f, rand() & 0x1f);
		// gvga_setPalette(gvga, _palette, 0, gvga->colors);
		if (sync) gvga_sync(gvga); // wait for the other core to finish displaying the frame buffer
		if (doubleBuffer) gvga_swap(gvga, true); // double-buffering with/without copy
	}
}