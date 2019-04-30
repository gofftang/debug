#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>
#include "render.h"

// #define DEBUG
// #define USE_SYSTEM

#define DEFAULT_DATA_SIZE  128

#define ADDR_MAGIC   2

static bool outside(led_render* lr, int x, int y)
{
    return x < 0 || x >= lr->width || y < 0 || y >= lr->height;
}

static int get_index(led_render* lr, int x, int y)
{
    return (x + y * lr->width) / 8;
}

void lr_sram(led_render* lr, int x, int y, int color)
{
    int index, bit, flip;
    int bits[] = {
        4, 5, 6, 7, 0, 1, 2, 3
    };

    if (outside(lr, x, y)) {
        fprintf(stderr, "error: [LR] (%d,%d) outside\n", x, y);
        return;
    }

    index = get_index(lr, x, y);

    //led: 3 2 1 0 | 4 5 6 7 | 8  9  10 11 | 12 13 14 15
    //    --------+----------+-------------+-------------
    //bit: 7 6 5 4 | 0 1 2 3 | 15 14 13 12 |  8  9 10 11
    flip = x % 8;
    bit = bits[flip];

    if (color)
        lr->data[index] |= (1 << bit);
    else {
        lr->data[index] &= ~(1 << bit);
    }

    // if (x == 1)
    //     printf("index=%d y=%02d %d %02X\n", index, y, color, lr->data[index]);
}

void lr_flush(led_render* lr)
{
    char data[DEFAULT_DATA_SIZE] = {0};
    char* p = data;
    int i;

#ifdef USE_SYSTEM
    memcpy(p, "echo ", 5);
    p += 5;
#endif

    memcpy(p, "00 ", 3);
    p += 3;

    for (i = 0; i < lr->sz_data; i++) {
        sprintf(p, "%02x ", lr->data[i]);
        p += 3;
    }
#ifdef USE_SYSTEM
    sprintf(p, " > /sys/class/leds/%s/device/led_pattern", lr->node);
    system(data);
#else
    fwrite(data, p - data, 1, lr->f_pattern);
    fflush(lr->f_pattern);
#endif
}

void lr_fill(led_render* lr, int x, int y, int color)
{
    int index;
#ifdef USE_SYSTEM
    char data[DEFAULT_DATA_SIZE];
#endif

    if (outside(lr, x, y)) {
        printf("error: [LR] fill (%d,%d) is outside\n", x, y);
        return;
    }

    lr_sram(lr, x, y, color);
    index = get_index(lr, x, y);

#ifdef USE_SYSTEM
    sprintf(data, "echo \"%02x%02x\" > /sys/class/leds/%s/device/led_pattern",
                    index * ADDR_MAGIC, lr->data[index], lr->node);
    #ifdef DEBUG
        printf("[LR] fill {%s}\n", data);
    #endif
    system(data);
#else
    fprintf(lr->f_pattern, "%02x%02x", index * ADDR_MAGIC, lr->data[index]);
    fflush(lr->f_pattern);
#endif
}

void lr_invert(led_render* lr, bool invert)
{
    int i;

    if (invert != lr->invert) {
        for (i = 0; i < lr->sz_data; i++) {
            lr->data[i] = ~lr->data[i];
        }

        lr->invert = invert;
        lr_flush(lr);
    }
}

// void lr_flip(led_render* lr, int x0, int y0)
// {
//     int x, y;

//     for (i = x0; i < lr->width; i++) {

//     }

//     for (i = x0; i < lr->width; i++) {
//         lr->data[i] = ~lr->data[i];
//     }

//     for (i = x0; i < lr->width; i++) {
//         lr->data[i] = ~lr->data[i];
//     }
// }

void lr_clear(led_render* lr)
{
    memset(lr->data, 0, lr->sz_data);
    lr_flush(lr);
}

void lr_blank(led_render* lr, bool blank)
{
    if (blank) {
        memset(lr->data, 0xFF, lr->sz_data);
        lr_flush(lr);
    }
    else {
        lr_clear(lr);
    }
}

// static void kill(const char* key)
// {
//     char cmd[DEFAULT_DATA_SIZE];

//     snprintf(cmd, sizeof(cmd),
//         "ps aux | grep %s |  awk '{print $1}' | xargs kill -9", key);
//     system(cmd);
// }

void lr_blink(led_render* lr, const char* type)
{
#ifdef USE_SYSTEM
    char data[DEFAULT_DATA_SIZE];

    snprintf(data, sizeof(data),
        "echo \"%s\" > /sys/class/leds/%s/device/led_blink", type, lr->node);
    system(data);
#else
    fprintf(lr->f_blink, "%s", type);
    fflush(lr->f_blink);
#endif
}

void lr_engine(led_render* lr, const char* cmd)
{
#ifdef USE_SYSTEM
    char data[DEFAULT_DATA_SIZE];

    snprintf(data, sizeof(data),
        "echo \"%s\" > /sys/class/leds/%s/device/led_engine", cmd, lr->node);
    system(data);
#else
    fprintf(lr->f_engine, "%s", cmd);
    fflush(lr->f_engine);
#endif
}

void lr_brightness(led_render* lr, int brightness)
{
#ifdef USE_SYSTEM
    char data[DEFAULT_DATA_SIZE];

    snprintf(data, sizeof(data),
        "echo %d > /sys/class/leds/%s/brightness", brightness, lr->node);
    system(data);
#else
    fprintf(lr->f_brightness, "%d", brightness);
    fflush(lr->f_brightness);
#endif
}

led_render* lr_create(const char *node, int width, int height)
{
    char path[128];

    if (!node || width <= 0 || height <= 0) {
        fprintf(stderr, "error: [LR] init invalid param\n");
        return NULL;
    }

    led_render* lr = (led_render*)malloc(sizeof(led_render));
    if (!lr) {
        fprintf(stderr, "error: [LR] init malloc 1\n");
		return NULL;
    }

    lr->width = 16;
    lr->height = 16;
    lr->sz_data = lr->width * lr->height / 8;
    lr->data = malloc(lr->sz_data);
    if (!lr->data) {
        fprintf(stderr, "error: [LR] init malloc 2\n");
        goto free_lr;
    }
    memset(lr->data, 0, lr->sz_data);

#ifdef USE_SYSTEM
    strncpy(lr->node, node, sizeof(lr->node)/sizeof(char));
#else
    snprintf(path, sizeof(path), "/sys/class/leds/%s/device/led_pattern", node);
    lr->f_pattern = fopen(path, "wb");
    if (!lr->f_pattern) {
        fprintf(stderr, "error: [LR] open %s\n", path);
        goto free_data;
    }

    snprintf(path, sizeof(path), "/sys/class/leds/%s/brightness", node);
    lr->f_brightness = fopen(path, "wb");
    if (!lr->f_brightness) {
        fprintf(stderr, "error: [LR] open %s\n", path);
        goto close_pattern;
    }

    snprintf(path, sizeof(path), "/sys/class/leds/%s/device/led_blink", node);
    lr->f_blink = fopen(path, "wb");
    if (!lr->f_blink) {
        fprintf(stderr, "error: [LR] open %s\n", path);
        goto close_brightness;
    }

    snprintf(path, sizeof(path), "/sys/class/leds/%s/device/led_engine", node);
    lr->f_engine = fopen(path, "wb");
    if (!lr->f_engine) {
        fprintf(stderr, "error: [LR] open %s\n", path);
        goto close_blink;
    }
#endif

    return lr;

#ifndef USE_SYSTEM
close_blink:
    fclose(lr->f_blink);
close_brightness:
    fclose(lr->f_brightness);
close_pattern:
    fclose(lr->f_pattern);
free_data:
    free(lr->data);
#endif
free_lr:
    free(lr);
    return NULL;
}

void lr_destroy(led_render* lr)
{
    if (lr) {
        if (lr->data)
            free(lr->data);
#ifndef USE_SYSTEM
        fclose(lr->f_brightness);
        fclose(lr->f_pattern);
        fclose(lr->f_engine);
        fclose(lr->f_blink);
#endif
        free(lr);
    }
}

void lr_debug(led_render* lr)
{
#ifdef DEBUG
    int i;

    assert(lr);

    fprintf(stderr, "[LR] debug dump:\n");
    for (i = 0; i < lr->sz_data; i++) {
        fprintf(stderr, "%02x ", lr->data[i]);

        if (i % 2 == 1)
            fprintf(stderr, "\n");
    }
#endif
}
