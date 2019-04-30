#ifndef HAL_INC_UNI_LED_H_
#define HAL_INC_UNI_LED_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
Usage:
	eg. file node is 'hbs1632'

	uni_hal_led_register("hbs1632");
	uni_hal_led_ctrl("hbs1632", "Fully On");

	... // put any LED ctrl here

	uni_hal_led_unregister("hbs1632");
*/

int uni_hal_led_register(const char *name);
void uni_hal_led_unregister(const char *name);

/*
@cmd: command list
	'Fully On' (On/Off)
	'Brightness 0'(0~15/Up/Down)
	'Blink 0.5Hz'(0.5Hz/1Hz/2Hz)
	'Show Time'
	'Show Wave' (Show Wave 0/90/180/270)
	'Engine Setup' (Setup/Shutdown/Start/Stop)
*/
int uni_hal_led_ctrl(const char *name, const char *cmd);

int uni_hal_led_feed_buffer(const char *buf, int size);

#ifdef __cplusplus
}
#endif
#endif
