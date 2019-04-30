#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <math.h>
#include "utils.h"
#include "render.h"
#include "kiss_fft.h"

#define DEBUG

#define MAX_DEVNO   4
#define NAME_SIZE   16
#define FIXED_WIDTH 16
#define FIXED_HEIGH 16

#define DOT_BORDER   1
#define DOT_WIDTH    6
#define DOT_HEIGHT   5

#define BRIGHTNESS_MAX 16

typedef struct led_device {
    char name[NAME_SIZE];
    led_render *render;

    int brightness;

    bool timing;
    struct tm now;

    bool waving;
    int degree;
    int wave[FIXED_WIDTH];

    bool exit;
    pthread_t pid;
    pthread_mutex_t lock;

    void* priv;
} led_device;

typedef struct led_service {
    led_device dev_list[MAX_DEVNO];
} led_service;

typedef enum session_type {
    ACT_LED_FULLY_OFF = 0x01,
    ACT_LED_FULLY_ON  = 0x02,
    ACT_LED_BRIGHTNESS = 0x04,
    ACT_LED_ENGINE = 0x08,
    ACT_LED_BLINK = 0x010,
    ACT_LED_DISPLAY_TIME = 0x20,
    ACT_LED_DISPLAY_WAVE = 0x40,
    ACT_LED_DISPLAY_LOVE = 0x80,
} session_t;

typedef struct led_session {
    session_t type;
    void* context;
    void* extra;
} led_session;

// 样本数量
#define SAMPLE_SZIE 512
// 预分频
#define PRESCALE    1
#define FFT_SIZE    SAMPLE_SZIE / PRESCALE
// 坐标显示个数
#define AXIS_SIZE   16


// #define WAVE_LENGTH 512
// // WAVE_LENGTH / 2          变换的共轭关系
// // (WAVE_LENGTH / 2) / 8    8分频提取分量
// #define FFT_SIZE    ((WAVE_LENGTH / 2) / 8)
// // 显示频率个数
// #define AXIS_COUNT  16

typedef struct wave_spectrum {
    kiss_fft_cpx cin[FFT_SIZE];
    kiss_fft_cpx cout[FFT_SIZE];
    kiss_fft_cfg state;
    int nfft;

    float amps[FFT_SIZE];
    float mags[FFT_SIZE];
    int nr_axises;
    int okey;
} wave_spectrum_t;

static led_service my_service, *service = &my_service;
static wave_spectrum_t wave_spectrum = {
    .nfft = FFT_SIZE,
    .nr_axises = AXIS_SIZE,
    .okey = 0,
};

static led_session* session_create(const char *cmd, void* context);
static void session_destroy(led_session* se);
static void session_exec(led_session* se);
static led_device* get_device(const char *name);
static void show_time(led_render* render);
static void show_random_wave(led_render* render);
static void show_spectrum_wave(led_render* render);
static void show_love(led_render* render);

static void flush_hour(led_render* render, int hour);
static void flush_min(led_render* render, int min);
static void flush_sec(led_render* render, int sec);

#if (DOT_WIDTH == 3)
static int dot_maps[][DOT_HEIGHT][DOT_WIDTH] =
{
    {
        { 1, 1, 1 },
        { 1, 0, 1 },
        { 1, 0, 1 },
        { 1, 0, 1 },
        { 1, 1, 1 }
    },
    {
        { 0, 0, 1 },
        { 0, 0, 1 },
        { 0, 0, 1 },
        { 0, 0, 1 },
        { 0, 0, 1 }
    },
    {
        { 1, 1, 1 },
        { 0, 0, 1 },
        { 1, 1, 1 },
        { 1, 0, 0 },
        { 1, 1, 1 }
    },
    {
        { 1, 1, 1 },
        { 0, 0, 1 },
        { 1, 1, 1 },
        { 0, 0, 1 },
        { 1, 1, 1 }
    },
    {
        { 1, 0, 1 },
        { 1, 0, 1 },
        { 1, 1, 1 },
        { 0, 0, 1 },
        { 0, 0, 1 }
    },
    {
        { 1, 1, 1 },
        { 1, 0, 0 },
        { 1, 1, 1 },
        { 0, 0, 1 },
        { 1, 1, 1 }
    },
    {
        { 1, 1, 1 },
        { 1, 0, 0 },
        { 1, 1, 1 },
        { 1, 0, 1 },
        { 1, 1, 1 }
    },
    {
        { 1, 1, 1 },
        { 0, 0, 1 },
        { 0, 0, 1 },
        { 0, 0, 1 },
        { 0, 0, 1 }
    },
    {
        { 1, 1, 1 },
        { 1, 0, 1 },
        { 1, 1, 1 },
        { 1, 0, 1 },
        { 1, 1, 1 }
    },
    {
        { 1, 1, 1 },
        { 1, 0, 1 },
        { 1, 1, 1 },
        { 0, 0, 1 },
        { 1, 1, 1 }
    }
};
#else
static int dot_maps[][DOT_HEIGHT][DOT_WIDTH] =
{
    {
        { 1, 1, 1, 1, 1, 1 },
        { 1, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 1 },
        { 1, 1, 1, 1, 1, 1 },
    },
    {
        { 0, 0, 0, 0, 0, 1 },
        { 0, 0, 0, 0, 0, 1 },
        { 0, 0, 0, 0, 0, 1 },
        { 0, 0, 0, 0, 0, 1 },
        { 0, 0, 0, 0, 0, 1 }
    },
    {
        { 1, 1, 1, 1, 1, 1 },
        { 0, 0, 0, 0, 0, 1 },
        { 1, 1, 1, 1, 1, 1 },
        { 1, 0, 0, 0, 0, 0 },
        { 1, 1, 1, 1, 1, 1 },
    },
    {
        { 1, 1, 1, 1, 1, 1 },
        { 0, 0, 0, 0, 0, 1 },
        { 1, 1, 1, 1, 1, 1 },
        { 0, 0, 0, 0, 0, 1 },
        { 1, 1, 1, 1, 1, 1 },
    },
    {
        { 1, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 1 },
        { 1, 1, 1, 1, 1, 1 },
        { 0, 0, 0, 0, 0, 1 },
        { 0, 0, 0, 0, 0, 1 }
    },
    {
        { 1, 1, 1, 1, 1, 1 },
        { 1, 0, 0, 0, 0, 0 },
        { 1, 1, 1, 1, 1, 1 },
        { 0, 0, 0, 0, 0, 1 },
        { 1, 1, 1, 1, 1, 1 },
    },
    {
        { 1, 1, 1, 1, 1, 1 },
        { 1, 0, 0, 0, 0, 0 },
        { 1, 1, 1, 1, 1, 1 },
        { 1, 0, 0, 0, 0, 1 },
        { 1, 1, 1, 1, 1, 1 },
    },
    {
        { 1, 1, 1, 1, 1, 1 },
        { 0, 0, 0, 0, 0, 1 },
        { 0, 0, 0, 0, 0, 1 },
        { 0, 0, 0, 0, 0, 1 },
        { 0, 0, 0, 0, 0, 1 }
    },
    {
        { 1, 1, 1, 1, 1, 1 },
        { 1, 0, 0, 0, 0, 1 },
        { 1, 1, 1, 1, 1, 1 },
        { 1, 0, 0, 0, 0, 1 },
        { 1, 1, 1, 1, 1, 1 },
    },
    {
        { 1, 1, 1, 1, 1, 1 },
        { 1, 0, 0, 0, 0, 1 },
        { 1, 1, 1, 1, 1, 1 },
        { 0, 0, 0, 0, 0, 1 },
        { 1, 1, 1, 1, 1, 1 },
    }
};
#endif

static void* thread_fn(void *arg)
{
    led_device *dev = (led_device*)arg;
    struct timeval tv_last, tv_now;
    int usec;
    int tick = 0;

    while (!dev->exit) {
        gettimeofday(&tv_last,NULL);
#ifdef DEBUG
        // printf("last: %ld\n", tv_last.tv_sec*1000 + tv_last.tv_usec/1000);
#endif
        pthread_mutex_lock(&dev->lock);

        if (0 && dev->timing && (tick % 2) == 0) {
            time_t new;
            struct tm* tm_new;
            struct tm* tm_now;
            int sync = 0;

            time(&new);
            tm_new = localtime(&new);

            tm_now = &dev->now;

            if (tm_new->tm_sec != tm_now->tm_sec){
                flush_sec(dev->render, tm_new->tm_sec);
                sync++;
            }

            if (tm_new->tm_min != tm_now->tm_min) {
                flush_min(dev->render, tm_new->tm_min);
                sync++;
            }

            if (tm_new->tm_hour != tm_now->tm_hour) {
                flush_hour(dev->render, tm_new->tm_hour);
                sync++;
            }

#ifdef DEBUG
            // printf("[LS] now timing: %d-%d-%d %d:%d:%d\n",
            //     tm_now->tm_year+1900, tm_now->tm_mon+1, tm_now->tm_mday,
            //     tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec);
            // printf("[LS] new timing: %d-%d-%d %d:%d:%d\n",
            //     tm_new->tm_year+1900, tm_new->tm_mon+1, tm_new->tm_mday,
            //     tm_new->tm_hour, tm_new->tm_min, tm_new->tm_sec);
#endif

            if (sync)
                memcpy(&dev->now, tm_new, sizeof(struct tm));
        }

        show_spectrum_wave(dev->render);
        // show_random_wave(dev->render);

        pthread_mutex_unlock(&dev->lock);

        gettimeofday(&tv_now,NULL);
#ifdef DEBUG
        // printf("now: %ld\n", tv_now.tv_sec*1000 + tv_now.tv_usec/1000);
#endif
        usec = MAX(50000, 250000 - tv_now.tv_usec);
        usleep(500000);
        tick++;
    }

    printf("[LS] %d exit\n", (int)pthread_self());
    pthread_exit(NULL);

    return NULL;
}

int uni_hal_led_register(const char *name)
{
    int i;
    led_device *dev;

    if (!name) {
        fprintf(stderr, "error: %d invalid name\n", __LINE__);
        return -1;
    }

    for (i = 0; i < MAX_DEVNO; i++) {
        dev = &service->dev_list[i];
        if (dev->name[0] && strcmp(name, dev->name) == 0) {
            printf("info: found %s registered\n", name);
            return 0;
        }
    }

    for (i = 0; i < MAX_DEVNO; i++) {
        dev = &service->dev_list[i];
        if (!dev->name[0]) {
            strncpy(dev->name, name, sizeof(dev->name));
            dev->render = lr_create(name, FIXED_WIDTH, FIXED_HEIGH);
            if (!dev->render) {
                fprintf(stderr, "error: create render\n");
                return -2;
            }

            dev->exit = false;
            if (pthread_create(&dev->pid, NULL, thread_fn, (void *)dev)) {
                fprintf(stderr, "error: create pthread\n");
                dev->exit = true;
                lr_destroy(dev->render);
                dev->render = NULL;
                dev->name[0] = 0;
                return -3;
            }

            if (pthread_mutex_init(&dev->lock, NULL)) {
                fprintf(stderr, "error: init mutex\n");
                dev->exit = true;
                lr_destroy(dev->render);
                dev->render = NULL;
                dev->name[0] = 0;
            }

            {
                time_t now;

                time(&now);
                memcpy(&dev->now, localtime(&now), sizeof(struct tm));
            }
            srand(time(NULL));

            printf("[LS] register %s handle=(%d,%x)\n",
                    name, (int)dev->pid, (unsigned int)dev->render);
            break;
        }
    }

    return i == MAX_DEVNO ? -4 : 0;
}

void uni_hal_led_unregister(const char *name)
{
    int i;
    led_device* dev;

    if (!name)
        return;

    for (i = 0; i < MAX_DEVNO; i++) {
        dev = &service->dev_list[i];
        if (strcmp(dev->name, name) == 0) {
            printf("[LS] unregister %s handle=(%d,%x)\n",
                    name, (int)dev->pid, (unsigned int)dev->render);
            dev->exit = true;
            // sleep(2);
            pthread_mutex_destroy(&dev->lock);
            lr_destroy(dev->render);
            dev->render = NULL;
            dev->name[0] = 0;
            break;
        }
    }
}

int uni_hal_led_ctrl(const char *name, const char *cmd)
{
    led_device* dev;
    led_session* se;

    if (!cmd) {
        fprintf(stderr, "[LS ctrl] invalid cmd\n");
        return -1;
    }

    dev = get_device(name);
    if (!dev) {
        fprintf(stderr, "[LS ctrl] %s\n", name ? name : "???");
        return -2;
    }

    if (strcmp(name, "hbs1632.2"))
        return 0;

#ifdef DEBUG
    printf("[LS ctrl] %s %s\n", name, cmd);
#endif

    se = session_create(cmd, dev);
    if (!se) {
        fprintf(stderr, "[LS ctrl] action create error\n");
        return -3;
    }
    session_exec(se);
    session_destroy(se);

    return 0;
}

float hypot_fabs(kiss_fft_cpx *y)
{
    return hypot((float)abs(y->r), (float)abs(y->i));
}

float np_clip(float x, float min, float max)
{
    if (x < min)
        return min;
    else if (x > max)
        return max;
    else
        return x;
}

void spectrum(char *buf, int len)
{
    wave_spectrum_t *ws = &wave_spectrum;
    float factor;
    int i;
    // static int dump = 0;

    // if (dump == 10) {
    //     printf("=== buf\n");
    //     for (i = 0; i < len; i++) {
    //         printf("%d, ", buf[i]);
    //     }
    //     printf("===\n");
    // }
    // dump++;

    if (!ws->okey) {
        ws->state = kiss_fft_alloc(ws->nfft, 0, NULL, NULL);
        if (ws->state) {
            kiss_fft_scalar zero;

            // ugly way of setting short,int,float,double, or __m128 to zero
            memset(&zero,0,sizeof(zero));
            for (i = 0; i < ws->nfft; i++)
                ws->cin[i].i = zero;

            ws->okey = 1;
        }
    }

    if (ws->okey) {
        if (PRESCALE > 1) {
            for (i = 0; i < ws->nfft; i++) {
                ws->cin[i].r = (buf[i * 8] + buf[i * 8 + 1] + buf[i * 8 + 2]
                                + buf[i * 8 + 3] + buf[i * 8 + 4] + buf[i * 8 + 5]
                                + buf[i * 8 + 6] + buf[i * 8 + 7]) / 8.0;
            }
        }
        else {
            for (i = 0; i < ws->nfft; i++) {
                ws->cin[i].r = buf[i];
            }
        }

        kiss_fft(ws->state, ws->cin, ws->cout);

        // amp[0] = np.abs(mag[0])/fftSize
        // amp[endIndex] = np.abs(mag[endIndex])/fftSize
        // amp[1:endIndex] = np.abs(mag[1:endIndex])*temp
        // amp = 20*np.log10(np.clip(np.abs(amp), 1e-20, 1e100))

        ws->amps[0] = hypot_fabs(&ws->cout[0]) / ws->nfft;
        ws->amps[ws->nfft - 1] = hypot_fabs(&ws->cout[ws->nfft - 1]) / ws->nfft;
        factor = 2.0 / ws->nfft;
        for (i = 1; i < ws->nfft - 1; i++) {
            ws->amps[i] = hypot_fabs(&ws->cout[i]) * factor;
            // ws->mags[i] = 20*log10(np_clip(fabs(ws->amps[i]), 1e-20, 1e100));
        }


        // 计算得到幅度基数
        // 如果要分16份，就基数amps[i] * 16
        factor = (float)ws->nr_axises * PRESCALE * 2 / len;
        for (i = 0; i < ws->nfft; i++) {
            ws->amps[i] *= factor;
        }

        //20000表示的最大频点20KHZ,这里的20-20K之间坐标的数据成对数关系,这是音频标准
        //方法中20为低频起点20HZ，16为段数
        // printf("freqs: ");
        // factor = pow(20000 / 20, 1.0 / ws->nfft);
        for (i = 0; i < ws->nfft; i++) {
            //乘方，30为低频起点，sampleratePoint数组中存的就是坐标值
            // ws->freqs[i] = 20 * pow(factor, i);
            // printf("(%f mV, %f dB)\n", ws->amps[i], ws->mags[i]);
        }
        // printf("\n");
    }
}

int uni_hal_led_feed_buffer(char *buf, int len)
{
    spectrum(buf, len);

    return 0;
}

static bool isbrightness(int brig)
{
    return (brig >= 0) && (brig < BRIGHTNESS_MAX);
}

static bool isdegree(int deg)
{
    return (deg == 0) || (deg == 90) || (deg == 180) || (deg == 270);
}

static led_session* session_create(const char *cmd, void* context)
{
    led_session* se;

    se = malloc(sizeof(*se));
    if (!se) {
        fprintf(stderr, "[LS] malloc se %s\n", cmd);
        return NULL;
    }
    memset(se, 0, sizeof(*se));
    se->context = context;

    if (strncmp(cmd, "Fully On", 8) == 0) {
        se->type = ACT_LED_FULLY_ON;
    }
    else if (strncmp(cmd, "Fully Off", 9) == 0) {
        se->type = ACT_LED_FULLY_OFF;
    }
    else if (strncmp(cmd, "Show Time", 9) == 0) {
        se->type = ACT_LED_DISPLAY_TIME;
    }
    else if (strncmp(cmd, "Show Wave", 9) == 0) {
        int deg = -1;
        int* p = (int*)&se->extra;
        led_device* dev = (led_device*)context;

        if (strlen(cmd) == 9) {
            dev->degree = 0;
        } else if (strlen(cmd) > 10) {
            sscanf(cmd+10, "%d", &deg);
            dev->degree = isdegree(deg) ? deg : dev->degree;
        }

        *p = dev->degree;
        se->type = ACT_LED_DISPLAY_WAVE;
    }
    else if (strncmp(cmd, "Show Love", 9) == 0) {
        se->type = ACT_LED_DISPLAY_LOVE;
    }
    else if (strncmp(cmd, "Brightness", 10) == 0) {
        int brig = -1;
        int* p = (int*)&se->extra;
        led_device* dev = (led_device*)context;

        if (strlen(cmd) > 11) {
            if (strncmp(cmd+11, "Up", 2) == 0) {
                brig = dev->brightness + 3;
                if (brig >= BRIGHTNESS_MAX)
                    brig = BRIGHTNESS_MAX - 1;
            }
            else if (strncmp(cmd+11, "Down", 4) == 0) {
                brig = dev->brightness - 3;
                if (brig < 0)
                    brig = 0;
            } else {
                printf("sscanf ret = %d\n", sscanf(cmd+11, "%d", &brig));
            }
            printf("sscanf brig = %d\n", brig);

            dev->brightness = isbrightness(brig) ? brig : dev->brightness;
            *p = dev->brightness;
        }
        else {
            *p = dev->brightness;
        }
        se->type = ACT_LED_BRIGHTNESS;
    }
    else if (strncmp(cmd, "Engine", 6) == 0) {
        if (strlen(cmd) > 6)
            se->extra = strdup(cmd+6);
        else
            se->extra = NULL;
        se->type = ACT_LED_ENGINE;
    }
    else if (strncmp(cmd, "Blink", 5) == 0) {
        if (strlen(cmd) > 6)
            se->extra = strdup(cmd+6);
        else
            se->extra = NULL;
        se->type = ACT_LED_BLINK;
    } else {
        free(se);
        fprintf(stderr, "[LS] invalid cmd %s\n", cmd);
        return NULL;
    }

    return se;
}

static void session_destroy(led_session* se)
{
    if (se) {
        switch (se->type) {
        case ACT_LED_BLINK:
        case ACT_LED_ENGINE:
            if (se->extra)
                free(se->extra);
            break;
        default:
            break;
        }

        free(se);
    }
}

static void session_exec(led_session* se)
{
    if (se) {
        led_device* dev = (led_device*)se->context;

        pthread_mutex_lock(&dev->lock);

        if (se->type & ACT_LED_FULLY_ON) {
            dev->timing = false;
            dev->waving = false;
            lr_blank(dev->render, true);
#ifdef DEBUG
            printf("[LS] exec Fully on\n");
#endif
        }

        if (se->type & ACT_LED_FULLY_OFF) {
            dev->timing = false;
            dev->waving = false;
            lr_clear(dev->render);
#ifdef DEBUG
            printf("[LS] exec Fully off\n");
#endif
        }

        if (se->type & ACT_LED_BRIGHTNESS) {
            lr_brightness(dev->render, (int)se->extra);
#ifdef DEBUG
            printf("[LS] exec Brightness %d\n", (int)se->extra);
#endif
        }

        if (se->type & ACT_LED_ENGINE) {
            lr_engine(dev->render, se->extra ? (char*)se->extra : "???");
#ifdef DEBUG
            printf("[LS] exec Engine %s\n", se->extra ? (char*)se->extra : "???");
#endif
        }

        if (se->type & ACT_LED_BLINK) {
            lr_blink(dev->render, se->extra ? (char*)se->extra : "???");
#ifdef DEBUG
            printf("[LS] exec Blink %s\n", se->extra ? (char*)se->extra : "???");
#endif
        }

        if (se->type & ACT_LED_DISPLAY_TIME) {
            dev->timing = true;
            dev->waving = false;
            show_time(dev->render);
#ifdef DEBUG
            printf("[LS] exec Show time\n");
#endif
        }

        if (se->type & ACT_LED_DISPLAY_WAVE) {
            dev->timing = false;
            dev->waving = true;
            show_random_wave(dev->render);
#ifdef DEBUG
            printf("[LS] exec Show waving\n");
#endif
        }

        if (se->type & ACT_LED_DISPLAY_LOVE) {
            dev->timing = false;
            dev->waving = false;
            show_love(dev->render);
#ifdef DEBUG
            printf("[LS] exec Show love\n");
#endif
        }

        pthread_mutex_unlock(&dev->lock);
    }
}

static led_device* get_device(const char *name)
{
    int i;
    led_device* dev;

    if (!name)
        return NULL;

    for (i = 0; i < MAX_DEVNO; i++) {
        dev = &service->dev_list[i];
        if (dev->name[0] && strcmp(dev->name, name) == 0)
            return dev;
    }

    return NULL;
}

static bool isdigt(int digit)
{
    return digit >= 0 && digit < 10;
}

static void show_digit(led_render* render, int x0, int y0, int digit)
{
    int x, y;

    return;

    if (!isdigt(digit)) {
        printf("error: [LS] invalid digit: %d\n", digit);
        return;
    }

    for (y = 0; y < DOT_HEIGHT; y++) {
        for (x = 0; x < DOT_WIDTH; x++) {
            int** map = (int**)((int*)dot_maps + (digit * DOT_WIDTH * DOT_HEIGHT));
            int* line = (int*)map + y * DOT_WIDTH;

            lr_sram(render, x0 + x, y0 + y, line[x]);
        }
    }

    lr_flush(render);
}

static void flush_hour(led_render* render, int hour)
{
#if (DOT_WIDTH == 3)
    show_digit(render, 1, 1, hour / 10);
    show_digit(render, 5, 1, hour % 10);
#else
    show_digit(render, 1, 2, hour / 10);
    show_digit(render, 9, 2, hour % 10);
#endif
}

static void flush_min(led_render* render, int min)
{
#if (DOT_WIDTH == 3)
    show_digit(render, 9, 7, min / 10);
    show_digit(render, 13, 7, min % 10);
#else
    show_digit(render, 1, 9, min / 10);
    show_digit(render, 9, 9, min % 10);
#endif
}

static void flush_sec(led_render* render, int sec)
{
#if (DOT_WIDTH == 3)
    int y0 = 14;
    int ten = (sec / 10) % 6;
    int per = sec % 10;
    int x;

    for (x = 0; x < 6; x++) {
        lr_fill(render, x, y0, x < ten);
    }

    for (x = 0; x < 10; x++) {
        lr_fill(render, x, y0+1, x < per);
    }
#endif
}

static void show_time(led_render* render)
{
    time_t now;
    struct tm* tm_now;

    return;

    time(&now);
    tm_now = localtime(&now);

#ifdef DEBUG
    printf("[LS] datatime: %d-%d-%d %d:%d:%d\n",
        tm_now->tm_year+1900, tm_now->tm_mon+1, tm_now->tm_mday,
        tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec);
#endif

    lr_clear(render);
    flush_hour(render, tm_now->tm_hour);
    flush_min(render, tm_now->tm_min);
    flush_sec(render, tm_now->tm_sec);
}

static void show_wave(led_render* render, int wave[FIXED_WIDTH])
{
    led_device *dev;
    int x, y;
    int hit, top;

    dev = get_device("hbs1632.2");
    if (!dev) {
        return;
    }

    lr_clear(render);
    // draw base
    for (x = 0; x < render->width; x++) {
        hit = CLIP(wave[x], 0, render->width-1);
        if (hit > dev->wave[x]) {
            dev->wave[x] = hit;
            top = hit;
        }
        else {
            dev->wave[x] = MAX(0, dev->wave[x]-1);
            top = dev->wave[x];
        }

        // if (x == 1)
        //     printf("(%d, %d, %d)\n", hit, top, dev->wave[x]);

        hit = render->width - hit;
        for (y = 0; y < render->height; y++) {
            lr_sram(render, x, y, y >= hit);
        }

        // if (top > 0) {
        //     top = render->width - top;
        //     lr_sram(render, x, top, 1);
        // }
    }

    lr_flush(render);

    // printf("\n");
}

static void show_random_wave(led_render* render)
{
    int wave[FIXED_WIDTH];
    int i;

    for (i = 0; i < FIXED_WIDTH; i++)
        wave[i] = 1 + rand() % (FIXED_WIDTH - 2);

    show_wave(render, wave);
}

static void show_spectrum_wave(led_render* render)
{
    int wave[FIXED_WIDTH];
    wave_spectrum_t *ws;
    // int i;

    ws = &wave_spectrum;
    // for (i = 0; i < FIXED_WIDTH; i++)
    wave[0] = (int)ws->amps[1];    // 86Hz
    wave[1] = (int)ws->amps[2];    // 172Hz
    wave[2] = (int)ws->amps[3];    // 258Hz
    wave[3] = (int)ws->amps[5];    // 430Hz
    wave[4] = (int)ws->amps[7];    // 603Hz
    wave[5] = (int)ws->amps[9];    // 775Hz
    wave[6] = (int)ws->amps[13];   // 1.1KHz
    wave[7] = (int)ws->amps[19];   // 1.6KHz
    wave[8] = (int)ws->amps[23];   // 1.9KHz
    wave[9] = (int)ws->amps[26];   // 2.2KHz
    wave[10] = (int)ws->amps[56];  // 4.8KHz
    wave[11] = (int)ws->amps[90];  // 7.7KHz
    wave[12] = (int)ws->amps[128]; // 11KHz
    wave[13] = (int)ws->amps[163]; // 14KHz
    wave[14] = (int)ws->amps[198]; // 17KHz
    wave[15] = (int)ws->amps[232]; // 20KHz

    show_wave(render, wave);
}

static void show_love(led_render* render)
{
#define LOVE_WIDTH  16
#define LOVE_HEIGHT 12
#define LOVE_Y0     2
    bool map[LOVE_HEIGHT][LOVE_WIDTH] = {
        {0, 0, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0},
        {0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 0},
        {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
        {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
        {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
        {0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0},
        {0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0},
        {0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0},
        {0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0}
    };
    int x, y;
    int width, height;

    return;

#ifdef DEBUG
    // printf("[LS] loving\n");
#endif

    lr_clear(render);

    width = MIN(render->width, LOVE_WIDTH);
    height = MIN(render->height, LOVE_HEIGHT);

    for (y = 0; y < height; y++) {
        bool* line = (bool*)map + y * LOVE_WIDTH;

        for (x = 0; x < width; x++) {
            lr_sram(render, x, LOVE_Y0 + y, line[x]);
        }
    }

    lr_flush(render);
}
