#ifndef _RENDER_H_
#define _RENDER_H_

#include <stdio.h>
#include <stdbool.h>
#include <time.h>

#define SIZE_OF_NODE 16

// #define USE_SYSTEM

typedef struct led_render {
#ifdef USE_SYSTEM
	char node[SIZE_OF_NODE];
#else
	FILE* f_brightness;
	FILE* f_pattern;
	FILE* f_engine;
	FILE* f_blink;
#endif

	bool invert;

	int width;
	int height;

	int sz_data;
	unsigned char* data;
} led_render;

led_render* lr_create(const char *node, int width, int height);
void lr_destroy(led_render* lr);

void lr_sram(led_render* lr, int x, int y, int color);
void lr_invert(led_render* lr, bool invert);
void lr_blank(led_render* lr, bool blank);
void lr_flush(led_render* lr);
void lr_clear(led_render* lr);
void lr_fill(led_render* lr, int x, int y, int color);

void lr_blink(led_render* lr, const char* type);
void lr_engine(led_render* lr, const char* cmd);
void lr_brightness(led_render* lr, int brightness);

void lr_time(led_render* lr);
void lr_debug(led_render* lr);

#endif