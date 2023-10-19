/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct {
    bool need_hint;
    uint8_t volume;  // 0 - 100%
} sys_param_t;

esp_err_t settings_read_from_nvs(void);
esp_err_t settings_write_to_nvs(void);
sys_param_t *settings_get(void);
#ifdef __cplusplus
}
#endif
