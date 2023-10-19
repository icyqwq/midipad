#pragma once

#include <ArduinoJson.h>
#include "pubsub.h"

#define WAVE_HEADER_SIZE    44
#define BYTE_RATE           (16000 * (16 / 8))
#define SIGN(x) (x > 0 ? 1 : -1)

#define PROB printf("prob %d\n", __LINE__);
#define MEMCHECK printf("memcheck >>>>>> [%s : %d] %d, %d\n", __FILE__, __LINE__, esp_get_free_heap_size(), esp_get_free_internal_heap_size());
#define PAUSE while (1) delay(1000000);

#ifndef PI
  #define PI 3.14159265358979323846
#endif


struct __attribute__((packed)) wav_header_t
{
  char RIFF[4];
  uint32_t chunk_size;
  char WAVEfmt[8];
  uint32_t fmt_chunk_size;
  uint16_t audiofmt;
  uint16_t channel;
  uint32_t sample_rate;
  uint32_t byte_per_sec;
  uint16_t block_size;
  uint16_t bit_per_sample;
};

struct __attribute__((packed)) sub_chunk_t
{
  char identifier[4];
  uint32_t chunk_size;
  uint8_t data[1];
};

struct SpiRamAllocator {
  void* allocate(size_t size) {
    return heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
  }

  void deallocate(void* pointer) {
    heap_caps_free(pointer);
  }

  void* reallocate(void* ptr, size_t new_size) {
    return heap_caps_realloc(ptr, new_size, MALLOC_CAP_SPIRAM);
  }
};

using SpiRamJsonDocument = BasicJsonDocument<SpiRamAllocator>;

void system_timer_init();
void ps_pub_buf_create(const char* topic, void *p_buf, uint32_t size);

bool is_element_in_array(const uint8_t *array, uint8_t data, uint8_t size) ;

float calculate_rms(int16_t * data, uint32_t len, uint8_t channel);
float update_ema_rms(float ema_rms, float new_rms, float alpha);
void generate_wav_header(char* wav_header, uint32_t wav_size, uint32_t sample_rate, uint8_t channels);
float log_map(float a, float a1, float a2, float b1, float b2);
void listFiles(const char * dirName);
bool is_point_inside_circle(int x, int y, int x0, int y0, int r);
void closest_point_on_circle_edge(int x, int y, int x0, int y0, int r, int *closest_x, int *closest_y);
float ease_out_cubic(float x);
float lerp(float start, float end, float t);
void startSystemStatsTask();
void convertToXY(int x0, int y0, float r, float degree, int* x1, int* y1);
