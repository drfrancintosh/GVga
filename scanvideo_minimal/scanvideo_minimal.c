/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>

#include "pico.h"
#include "pico/scanvideo.h"
#include "pico/scanvideo/composable_scanline.h"
#include "pico/multicore.h"
#include "pico/sync.h"
#include "pico/stdlib.h"
#include "hardware/clocks.h"

#define VGA_MODE vga_mode_320x240_60
extern const struct scanvideo_pio_program video_24mhz_composable;

// to make sure only one core updates the state when the frame number changes
// todo note we should actually make sure here that the other core isn't still rendering (i.e. all must arrive before either can proceed - a la barrier)

static void frame_update_logic();
static void render_scanline(struct scanvideo_scanline_buffer *dest, int core);

// "Worker thread" for each core
void render_loop() {
    static uint32_t last_frame_num = 0;
    static uint32_t last_scanline = 0xffff;
    int core_num = get_core_num();
    printf("Rendering on core %d\n", core_num);
    while (true) {
        struct scanvideo_scanline_buffer *scanline_buffer = scanvideo_begin_scanline_generation(true);
        if (last_scanline != scanline_buffer->scanline_id) {
            last_scanline = scanline_buffer->scanline_id;
            // mutex_enter_blocking(&frame_logic_mutex);
            // uint32_t frame_num = scanvideo_frame_number(scanline_buffer->scanline_id);
            // // Note that with multiple cores we may have got here not for the first
            // // scanline, however one of the cores will do this logic first before either
            // // does the actual generation
            // if (frame_num != last_frame_num) {
            //     last_frame_num = frame_num;
            //     frame_update_logic();
            // }
            // mutex_exit(&frame_logic_mutex);

            render_scanline(scanline_buffer, core_num);

            // Release the rendered buffer into the wild
        }
        scanvideo_end_scanline_generation(scanline_buffer);
    }
}

void fill_queue(int n) {
    for (int i = 0; i < n; i++) {
        struct scanvideo_scanline_buffer *scanline_buffer = scanvideo_begin_scanline_generation(true);
        render_scanline(scanline_buffer, 0);
        scanvideo_end_scanline_generation(scanline_buffer);
    }
}

void core1_func() {
    scanvideo_setup(&VGA_MODE);
    scanvideo_timing_enable(true);
    render_loop();
}

void frame_update_logic() {

}

#define MIN_COLOR_RUN 3

uint32_t push(uint32_t *buf, int n2, uint16_t value) {
    int n = n2/2;
    if (n2%2 == 0) {
        buf[n] = value;
    } else {
        buf[n] |= (value << 16);
    }
    return n2+1;
}

uint32_t push32(uint32_t *buf, int n2, uint16_t value) {
    int n = n2/2;
    if (n2%2 == 0) {
        buf[n] = value << 16;
    } else {
        buf[n] |= (value << 16);
    }
    return n2+1;
}

int32_t single_color_scanline_old(uint32_t *buf, size_t buf_length, int width, uint32_t color16) {
    int n=0;
    assert(buf_length >= 2);
    assert(width >= MIN_COLOR_RUN);
    // | jmp color_run | color | count-3 |  buf[0] =
    buf[n++] = COMPOSABLE_COLOR_RUN | (color16 << 16);
    buf[n++] = (width/2 - MIN_COLOR_RUN) | COMPOSABLE_COLOR_RUN << 16;
    buf[n++] = (0x1f<<5) |  (width/2 - MIN_COLOR_RUN) << 16;
    buf[n++] = (COMPOSABLE_RAW_1P) | 0 << 16;
    // note we must end with a black pixel
    buf[n++] = COMPOSABLE_EOL_ALIGN << 16;

    return n;
}

int32_t single_color_scanline(uint32_t *buf, size_t buf_length, int width, uint32_t color16) {
    int n=0;
    assert(buf_length >= 2);
    assert(width >= MIN_COLOR_RUN);
    // | jmp color_run | color | count-3 |  buf[0] =
    n = push(buf, n, COMPOSABLE_COLOR_RUN);
    n = push(buf, n, color16);
    n = push(buf, n, width/2 - MIN_COLOR_RUN);

    n = push(buf, n, COMPOSABLE_COLOR_RUN);
    n = push(buf, n, 0x1f<<5);
    n = push(buf, n, width/2 - MIN_COLOR_RUN);

    // note we must end with a black pixel
    n = push(buf, n, COMPOSABLE_RAW_1P);
    n = push(buf, n, 0);
    n = push32(buf, n+1, COMPOSABLE_EOL_ALIGN);
    return n/2;
}

#define COLOR(r,g,b) PICO_SCANVIDEO_PIXEL_FROM_RGB5(r, g, b)
int32_t striped_scanline(uint32_t *buf, size_t buf_length, int width, uint32_t color16) {
    int n=0;
    uint32_t colorBase = color16 / 80;
    uint16_t rgb[3] = {0, 0, 0};
    uint16_t color = COLOR(rgb[0], rgb[1], rgb[2]);
    n = push(buf, n, COMPOSABLE_RAW_RUN);
    n = push(buf, n, color);
    n = push(buf, n, 320-3);
    for(uint32_t i = 0; i < 9; i++) n = push(buf, n, color);
    for(uint32_t j=1; j <= 0x1f; j++) {
        rgb[colorBase] = j;
        color = COLOR(rgb[0], rgb[1], rgb[2]);
        for(uint32_t i = 0; i < 10; i++) n = push(buf, n, color);
    }
    n = push(buf, n, COMPOSABLE_RAW_1P);
    n = push(buf, n, 0);
    // note we must end with a black pixel
    n = push32(buf, n, COMPOSABLE_EOL_ALIGN);
    return n/2;
}

void render_scanline(struct scanvideo_scanline_buffer *dest, int core) {
    uint32_t *buf = dest->data;
    size_t buf_length = dest->data_max;

    int l = scanvideo_scanline_number(dest->scanline_id);
    uint16_t bgcolour = (uint16_t) l;
    dest->data_used = striped_scanline(buf, buf_length, VGA_MODE.width, bgcolour);
    dest->status = SCANLINE_OK;
}


int main(void) {
    multicore_launch_core1(core1_func);
    while(1);
}
