#include "app_task.h"
#include "app_common.h"
#include "declaration.h"
#include <deque>
#include "img_rgb565.h"

#define TAG "Display"

#define UI_SCALE_SPACE  16
#define UI_NOTE_HEIGHT 15

typedef struct {
	uint16_t x;
	uint16_t y;
	uint16_t w;
	uint16_t h;
	uint8_t scale;
	uint8_t status;
	uint32_t id;
	uint8_t channel;
} midi_note_disp_t;

static SemaphoreHandle_t display_semaphore = NULL;
static ps_subscriber_t *display_sub = nullptr;
static ps_msg_t *msg = nullptr;
static std::deque<midi_note_disp_t> notes;
static uint32_t frame_count = 0;
static int16_t scale_start = MIDI_SCALE_C3;
static uint8_t current_channel = 0;
static uint8_t channels[MIDI_MAX_CHANNELS];
static float knob_angle[2] = {0, 0};
static uint8_t knob_sel = 0;

const uint16_t k_rainbow_colors[16] = {
    0xF800,  // Red
    0xFC00,  // Orange
    0xFFE0,  // Yellow
    0xAFE0,  // Yellow-Green
    0x07E0,  // Green
    0x07F0,  // Green-Cyan
    0x07FF,  // Cyan
    0x047F,  // Cyan-Blue
    0x001F,  // Blue
    0x701F,  // Blue-Magenta
    0xF81F,  // Magenta
    0xF810,  // Magenta-Red
    0x7810,  // Purple-Red
    0x7800,  // Purple-Orange
    0x7820,  // Purple-Yellow
    0x7840   // Purple-Green
};


static const char* k_scale_texts[] = {
	"C-", "C#-", "D-", "D#-", "E-", "F-", "F#-", "G-", "G#-", "A-", "A#-", "B-",
    "C0", "C#0", "D0", "D#0", "E0", "F0", "F#0", "G0", "G#0", "A0", "A#0", "B0",
    "C1", "C#1", "D1", "D#1", "E1", "F1", "F#1", "G1", "G#1", "A1", "A#1", "B1",
    "C2", "C#2", "D2", "D#2", "E2", "F2", "F#2", "G2", "G#2", "A2", "A#2", "B2",
    "C3", "C#3", "D3", "D#3", "E3", "F3", "F#3", "G3", "G#3", "A3", "A#3", "B3",
    "C4", "C#4", "D4", "D#4", "E4", "F4", "F#4", "G4", "G#4", "A4", "A#4", "B4",
    "C5", "C#5", "D5", "D#5", "E5", "F5", "F#5", "G5", "G#5", "A5", "A#5", "B5",
    "C6", "C#6", "D6", "D#6", "E6", "F6", "F#6", "G6", "G#6", "A6", "A#6", "B6",
    "C7", "C#7", "D7", "D#7", "E7", "F7", "F#7", "G7", "G#7", "A7", "A#7", "B7",
    "C8", "C#8", "D8", "D#8", "E8", "F8", "F#8", "G8", "G#8", "A8", "A#8", "B8"
};

static void app_display_draw_knob(float angle, const char* label, int cx, int cy, uint8_t knob)
{
	if (angle < -135) {
		angle = -135;
	}
	if (angle > 135) {
		angle = 135;
	}

	base_canvas.setTextDatum(CC_DATUM);
	base_canvas.setFont(&fonts::Font0);
	base_canvas.setTextSize(1);
	base_canvas.setTextColor(WHITE);

	base_canvas.pushImageRotateZoomWithAA(cx, cy, 23, 23, angle, 1, 1, 46, 46, k_img565_knob_46x46, 0x1FF8);
	base_canvas.drawString(label, cx + 1, cy);
	if (knob_sel == knob) {
		base_canvas.pushImage(cx - 3, cy + 24, 9, 6, k_img565_indicator_on_90_9x6, (uint16_t)0x1FF8);
	}
}

static void app_display_draw()
{
	base_canvas.pushImage(0, 0, 240, 320, k_img565_base_240x320);
	// base_canvas.fillSprite(0);
	
	
	for (auto it = notes.begin(); it != notes.end(); ) {
		if (it->scale < scale_start || it->scale >= scale_start + 12) {
			++it;
			continue;
		}
		uint8_t y = (it->scale - scale_start) * UI_SCALE_SPACE;
		it->y = y + 7 - (it->h >> 1);
		base_canvas.fillRect(it->x, it->y, it->w, it->h, k_channel_colors[it->channel]);
		if (it->status == MIDI_NOTE_STATUS_START) {
			base_canvas.pushImage(3, y + 3, 6, 9, k_img565_indicator_on_6x9, (uint16_t)0x1FF8);
		}
		++it;
	}

	base_canvas.pushImage(30, 0, 10, 191, k_img565_strength_title_1_10x191, (uint16_t)0x1FF8);

	base_canvas.setTextDatum(CL_DATUM);
	base_canvas.setFont(&fonts::Font2);
	base_canvas.setTextSize(1);
	base_canvas.setTextColor(BLACK);
	for (int i = 0; i < 12; i++) {
		base_canvas.drawString(k_scale_texts[i + scale_start], 11, 7 + i * UI_SCALE_SPACE);
	}

	base_canvas.pushImage(3, current_channel * 18 + 197, 6, 9, k_img565_indicator_on_6x9, (uint16_t)0x1FF8);

	for (int i = 0; i < MIDI_MAX_CHANNELS; i++) {
		
		base_canvas.drawString(k_instrument_texts[channels[i]], 11, 7 + 194 + i * 18);
	}

	app_display_draw_knob(knob_angle[0], "STR", 197, 224, 0);
	app_display_draw_knob(knob_angle[1], "PRES", 197, 288, 1);

	base_canvas.pushSprite(0, 0);
}

static void app_display_logic()
{
	for (auto it = notes.begin(); it != notes.end(); ) {
		if (it->status == MIDI_NOTE_STATUS_START) {
			it->w++;
		}
		else {
			it->x++;
		}
		if (it->x >= 240) {
			it = notes.erase(it);
		} else {
			++it;
		}
	}
}

static void app_display_task(void *args)
{
	while (1)
	{
		msg = ps_get(display_sub, -1);
		
		if (ps_has_topic(msg, "t")) { // logic timeup event
			app_display_logic();
		}
		else if (ps_has_topic(msg, "r")) { // render timeup event
			app_display_draw();
		}
		else if (ps_has_topic(msg, "midi.note")) {
			ESP_LOGI(TAG, "new note event");
			midi_note_t *note = (midi_note_t *)msg->buf_val.ptr;

			if (note->status == MIDI_NOTE_STATUS_START) {
				midi_note_disp_t note_disp;

				note_disp.x = 34;
				note_disp.w = 1;
				note_disp.scale = note->scale;
				note_disp.h = (note->strength / 127.0f) * UI_NOTE_HEIGHT;
				if (note_disp.h < 3) {
					note_disp.h = 3;
				}
				note_disp.y = (note->scale - scale_start) * UI_SCALE_SPACE + 7 - (note_disp.h >> 1);;
				note_disp.id = note->id;
				note_disp.status = note->status;
				note_disp.channel = note->channel;
				notes.push_back(note_disp);
				ESP_LOGI(TAG, "new note %d added, h: %d, y: %d, notes: %d", note->id, note_disp.h, note_disp.y, notes.size());
			}
			else {
				for (auto it = notes.begin(); it != notes.end(); ) {
					if (it->id == note->id) {
						ESP_LOGI(TAG, "note %d stopped", note->id);
						it->status = MIDI_NOTE_STATUS_END;
						break;
					}
					++it;
				}
			}
		}
		else if (ps_has_topic(msg, "midi.ss")) {
			ESP_LOGI(TAG, "scale_start changed to %d", (int)msg->int_val);
			scale_start = (int)msg->int_val;
		}
		else if (ps_has_topic(msg, "midi.ch")) {
			ESP_LOGI(TAG, "channel changed to %d", (int)msg->int_val);
			current_channel = (uint8_t)msg->int_val;
		}
		else if (ps_has_topic(msg, "midi.prog")) {
			// ESP_LOGI(TAG, "channel changed to %d", (int)msg->int_val);
			uint8_t ch = msg->int_val >> 8;
			uint8_t sel = msg->int_val & 0xFF;
			channels[ch] = sel;
		}
		else if (ps_has_topic(msg, "midi.str")) {
			ESP_LOGI(TAG, "midi vol: %f", (float)msg->dbl_val);
			knob_angle[0] = (float)msg->dbl_val * 270 - 135;
		}
		else if (ps_has_topic(msg, "midi.pres")) {
			ESP_LOGI(TAG, "midi pres: %f", (float)msg->dbl_val);
			knob_angle[1] = (float)msg->dbl_val * 270 - 135;
		}
		else if (ps_has_topic(msg, "midi.knob")) {
			knob_sel = (int)msg->int_val;
		}

		vTaskDelay(1);
		ps_unref_msg(msg);
	}
}

void app_display_task_init()
{
	display_sub = ps_new_subscriber(15, PS_STRLIST("t", "r", "midi.note", "midi.ss", "midi.ch", "midi.prog", "midi.str", "midi.pres", "midi.knob"));
	xTaskCreatePinnedToCore(&app_display_task, "displayTask", 16 * 1024, NULL, 10, NULL, 1);
}
