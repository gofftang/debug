#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include "render.h"
#include "service.h"

#define LED_NAME "hbs1632.0"

static void test_service(int argc, char *argv[])
{
    int cmd = argc > 2 ? atoi(argv[2]) : 0;

    uni_hal_led_register(LED_NAME);

    switch (cmd) {
    case 0:
        uni_hal_led_ctrl(LED_NAME, "Show Time");
        break;
    case 1:
        uni_hal_led_ctrl(LED_NAME, "Show Wave 0");
        break;
    case 2:
        uni_hal_led_ctrl(LED_NAME, "Show Wave 90");
        break;
    case 3:
        uni_hal_led_ctrl(LED_NAME, "Show Wave 180");
        break;
    case 4:
        uni_hal_led_ctrl(LED_NAME, "Show Wave 270");
        break;
    case 5:
        uni_hal_led_ctrl(LED_NAME, "Blink 1Hz");
        break;
    case 6:
        uni_hal_led_ctrl(LED_NAME, "Blink Off");
        break;
    case 7:
        uni_hal_led_ctrl(LED_NAME, "Engine Setup");
        break;
    case 8:
        uni_hal_led_ctrl(LED_NAME, "Engine Shutdown");
        break;
    case 9:
        uni_hal_led_ctrl(LED_NAME, "Engine Start");
        break;
    case 10:
        uni_hal_led_ctrl(LED_NAME, "Engine Stop");
        break;
    case 11:
        uni_hal_led_ctrl(LED_NAME, "Show Love");
        break;
    case 12:
        uni_hal_led_ctrl(LED_NAME, "Brightness 8");
        break;
    default:
        break;
    }

    while (1)
        sleep(1);

    // uni_hal_led_unregister(LED_NAME);
}

static void test_rander(int argc, char *argv[])
{
    led_render* lr;
    int cmd = argc > 2 ? atoi(argv[2]) : 0;

    lr = lr_create(LED_NAME, 16, 16);
    assert(lr != NULL);

    switch (cmd) {
    case 0: {
            int x = argc > 3 ? atoi(argv[3]) : 0;
            int y = argc > 4 ? atoi(argv[4]) : 0;
            int color = argc > 5 ? atoi(argv[5]) : 0;

            lr_fill(lr, x, y, color);
        }
        break;
    case 1:
        lr_blank(lr, true);
        break;
    case 2:
        lr_blank(lr, false);
        break;
    case 3:
        lr_clear(lr);
        break;
    case 4:
        lr_blank(lr, true);
        break;
    case 5: {
            int brig = argc > 3 ? atoi(argv[3]) : 0;

            lr_brightness(lr, brig);
        }
        break;
    case 6: {
            char* type = argc > 3 ? (char*)&argv[3] : "???";

            lr_blink(lr, type);
        }
        break;
    default:
        lr_debug(lr);
        break;
    }

    lr_destroy(lr);
}

int main(int argc, char* argv[])
{
    int type = argc > 1 ? atoi(argv[1]) : 0;

    switch (type) {
    case 1:
        test_rander(argc, argv);
        break;
    case 2:
        test_service(argc, argv);
        break;
    default:
        break;
    }

    return 0;
}
