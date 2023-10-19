#define ARDUINOJSON_DECODE_UNICODE 1
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include <Arduino.h>
#include "nvs_flash.h"
#include "nvs.h"
#include "nvs_handle.hpp"
#include <M5Unified.h>
#include <SD.h>
#include "declaration.h"
#include "app_common.h"
#include "app_task.h"
#include <WiFi.h>

#define TAG "MAIN"


void test()
{

}

extern "C" void app_main(void)
{
    esp_log_level_set("*", ESP_LOG_INFO); 
    esp_log_level_set("gpio", ESP_LOG_WARN);

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    M5.begin();
    M5.Display.setRotation(0);
    M5.Display.setBrightness(100);

    ESP_LOGI("MAIN", "\n\nHELLO\n\n");
    SD.begin(GPIO_NUM_4, SPI, 20000000);
    ESP_LOGI(TAG, "SD Card: %d, %d MiB", SD.cardType(), SD.cardSize() / 1024 / 1024);

    ps_init();

    declaration_components_init();

    settings_read_from_nvs();
    ESP_LOGI(TAG, "settings_get()->volume: %d", settings_get()->volume);

    // setupWiFi();

    app_led_task_init();
    app_display_task_init();
    app_input_task_init();
    app_midi_task_init();
    system_timer_init();

    while (1)
    {
        PAUSE
    }
}
