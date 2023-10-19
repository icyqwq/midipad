#pragma once

#include "inttypes.h"

void fpng_buffer_init(int w, int h);
uint8_t* fpng_get_buffer();
void convert_rgb888_to_rgb565(uint8_t *p, uint32_t len, bool swap);
int fpng_decode_memory(const uint8_t *pImage, uint32_t image_size, uint32_t width, uint32_t height);
int fpng_decode_memory(const uint8_t *pImage, uint32_t image_size, uint8_t *out, uint32_t width, uint32_t height);
