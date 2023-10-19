#pragma once
#include <M5Unified.h>
#include "settings.h"
#include "midi_defines.h"

extern LGFX_Sprite base_canvas;

void declaration_components_init();

typedef enum {

    KEY_ID_BTN_0 = 22,
    KEY_ID_BTN_1 = 23,
    KEY_ID_MAX = 24,
} key_id_t;

enum {
	ACTION_RELEASED = 0x00,
	ACTION_PRESSED = 0x80,
};

static const uint16_t k_channel_colors[7] = {
	0xFF64,0x04FF,0xE8E4,0xF81F,0x07FF,0x07E6,0xFD87 
};
static const uint32_t k_channel_colors_888[7] = {
    0xFCEE21, 0x009FFF, 0xED1C24, 0xFF00FF, 0x00FFFF, 0x00DF3C, 0xFBB03B,
};