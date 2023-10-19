#include "app_task.h"
#include "app_common.h"
#include "declaration.h"
#include <FastLED.h>

#define TAG "Led"

#define LED_PIN     25
#define NUM_LEDS    30
#define BRIGHTNESS  128
#define LED_TYPE    WS2812
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];
/*
													[5.4]
		[0.0]		[1.1]		[2.2]		[3.3] 	[6.4]

		[4.5]		[8.6]		[9.7]		[10.8]	[11.9]

		[12.10]	[13.11]	[14.12]	[15.13]	[21.14]

		[16.15]	[17.16]	[18.17]	[19.18]	[20.19]
*/

// led 0 ~ 20
static uint8_t key_led_map[KEY_ID_MAX] = {
	0, 1, 2, 3, 5, 4, 4, 255, 6, 7, 8, 9, 10, 11, 12, 13, 15, 16, 17, 18, 19, 14, 255
};
static uint8_t key_channel_map[] = {
	0, 1, 2, 3, 255, 255, 255, 255, 4, 5, 6
};

static SemaphoreHandle_t led_semaphore = NULL;
static uint8_t led_mode = LED_MODE_RMS;
static ps_subscriber_t *led_sub = nullptr;
static ps_msg_t *msg = nullptr;
static uint8_t current_channel = 0;
static int active_key_num = 0;
uint32_t last_active_time = 0;

CRGBPalette16 currentPalette;
TBlendType    currentBlending;
extern CRGBPalette16 myRedWhiteBluePalette;
extern const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM;

void FillLEDsFromPaletteColors(uint8_t colorIndex)
{    
	int start = active_key_num ? 20 : 0;
	if (millis() - last_active_time < 3000) {
		start = 20;
	}
    for( int i = start; i < NUM_LEDS; ++i) {
        leds[i] = ColorFromPalette( RainbowColors_p, colorIndex, BRIGHTNESS, LINEARBLEND);
        colorIndex += 3;
    }
}

void app_led_set_by_key(int key, CRGB color, bool is_set=true)
{
	if (key >= sizeof(key_led_map)) {
		return;
	}
	uint8_t led = key_led_map[key];
	if (led != 255) {
		leds[led] = color;
		if (is_set) {
			active_key_num++;
		}
		else {
			last_active_time = millis();
			active_key_num--;
		}
	}
}

void app_led_task(void *args)
{
	uint8_t startIndex = 0;
	fill_solid(leds, NUM_LEDS, CRGB::Black);
	while (1)
	{    
		msg = ps_get(led_sub, 50);
		if (ps_has_topic(msg, "input.key"))
		{
			uint8_t key = msg->int_val & 0x3F;
			uint8_t action = msg->int_val & 0xC0;

			if (key >= KEY_ID_BTN_0) {
				continue;
			}

			if (key < sizeof(key_channel_map)) {
				uint8_t ch = key_channel_map[key];
				if (ch != 255) {
					if (action == ACTION_PRESSED) {
						if (active_key_num == 0) {
							fill_solid(leds, 20, CRGB::Black);
						}
						app_led_set_by_key(key, CRGB(k_channel_colors_888[ch]));
					}
					else {
						app_led_set_by_key(key, CRGB::Black, false);
					}
					FastLED.show();
					continue;
				}
			}
			
			if (action == ACTION_PRESSED) {
				if (active_key_num == 0) {
					fill_solid(leds, 20, CRGB::Black);
				}
				app_led_set_by_key(key, CRGB(k_channel_colors_888[current_channel]));
			}
			else {
				app_led_set_by_key(key, CRGB::Black, false);
			}
			FastLED.show();
		}
		else if (ps_has_topic(msg, "midi.ch")) {
			ESP_LOGI(TAG, "channel changed to %d", (int)msg->int_val);
			current_channel = (uint8_t)msg->int_val;
		}
		ps_unref_msg(msg);

		FillLEDsFromPaletteColors(startIndex++);
		FastLED.show();
		// FastLED.delay(100);
	}
}

void app_led_task_init()
{
	FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
    FastLED.setBrightness(BRIGHTNESS);
	led_sub = ps_new_subscriber(15, PS_STRLIST("input.key", "midi.ch"));
	xTaskCreatePinnedToCore(&app_led_task, "LedTask", 3 * 1024, NULL, 1, NULL, 1);
}

void app_led_set_mode(uint8_t mode)
{
	led_mode = mode;
}