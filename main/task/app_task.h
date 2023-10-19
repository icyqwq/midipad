#pragma once

#include <M5Unified.h>
#include "declaration.h"

#define MIDI_MAX_CHANNELS 7

typedef struct
{
	union {
		int data;
		void *args;
	};
	uint8_t cmd;
} app_common_cmd_t;

enum
{
	LED_MODE_RMS = 0,
	LED_MODE_ROTATE
};



void app_led_task_init();
void app_led_set_mode(uint8_t mode);

void app_display_task_init();

void app_input_task_init();

void app_midi_task_init();