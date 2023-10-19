#include <Arduino.h>
#include "app_task.h"
#include "app_common.h"
#include <deque>
#include "FifteenStep.h"
#include "declaration.h"

#define TAG "midi"

static const uint8_t k_key_tone_map[] = {
    255, 255, 255, 255, 0, // 4
	255, 255, 255, 255, 255, // 9
	255, 7, 1, 2, 3, // 14
	8, 4, 5, 6, 10, // 19
	11, 9 // 21
};

static const uint8_t k_key_channel_map[] = {
	0, 1, 2, 3, 255,
	255, 255, 255, 4, 5, 6,
};


enum {
	KNOB_FUNC_VOL = 0,
	KNOB_FUNC_PRESSURE,
	KNOB_FUNC_MAX
};

static const uint8_t k_sharp_semitone_table[] = {1, 3, 6, 8, 10};
static const uint8_t k_semitone_table[] = {0, 2, 4, 5, 7, 9, 11};

HardwareSerial &MIDISerial = Serial1;
static SemaphoreHandle_t midi_semaphore = NULL;
static ps_subscriber_t *midi_sub = nullptr;
static ps_msg_t *msg = nullptr;
FifteenStep seq = FifteenStep();
midi_note_t notes[12];
int16_t channel_instruments[MIDI_MAX_CHANNELS];
int16_t channel_pressure[MIDI_MAX_CHANNELS];
int16_t channel_strength[MIDI_MAX_CHANNELS];
uint8_t current_channel = 0;
int16_t scale_start = MIDI_SCALE_C3;
uint32_t note_id = 0;
static int16_t strength_sel = 0;
static uint8_t knob_func_sel = KNOB_FUNC_VOL;
static uint8_t scale_lut[12];
bool scale_up = false;

static void app_midi_step_cb(int current, int last) {

//   if(current % 2 == 0)
//     digitalWrite(13, HIGH);
//   else
//     digitalWrite(13, LOW);

}

static void app_midi_write(uint8_t channel, uint8_t command, uint8_t arg1, uint8_t arg2 = 0) {

  if(command < 128) {
    // shift over command
    command <<= 4;
  }
  command |= channel;
  char buf[3] = {command, arg1, arg2};

  // send MIDI data
  MIDISerial.write(buf, 3);
}

static void app_midi_change_program(uint8_t channel, uint8_t prog) 
{
	MIDISerial.write(MIDI_CMD_PROGRAM_CHANGE + channel);
	MIDISerial.write(prog);
}

static void app_midi_change_pressure(uint8_t channel, uint8_t val)
{
	MIDISerial.write(MIDI_CMD_CONTROL_CHANGE + channel);
	MIDISerial.write(0x40);
	MIDISerial.write(val);
}

static void app_midi_test()
{
	uint8_t notes[] = {60, 62, 64, 65, 67, 69, 71, 72}; // C4, D4, E4, F4, G4, A4, B4, C5

	for (int i = 0; i < 8; i++) {
		app_midi_write(0, 0x90, notes[i], 30);
		delay(100);
		app_midi_write(0, 0x80, notes[i], 0x45);
	}

}

static uint8_t app_midi_search_scale(uint8_t note)
{
	if (note > 11) {
		return 255;
	}
	uint8_t sub_note = scale_start % 12;
	
	if (note < 7) { // minus
		uint8_t found = 0;
		uint8_t i = 0;
		uint8_t n = 12;
		uint8_t scale = 0;
		while (n--)
		{
			scale = scale_start + i;
			if (is_element_in_array(k_semitone_table, scale % 12, sizeof(k_semitone_table))) {
				if (found == note) {
					ESP_LOGI(TAG, "found minus scale = %d", scale);
					break;
				}
				found++;
			}
			i++;
		}
		if (n == 0 && found == 0) {
			ESP_LOGW(TAG, "scale notfound, start %d, note %d", scale_start, note);
			return 255;
		}
		return scale;
	}
	else { // half
		uint8_t found = 0;
		uint8_t i = 0;
		uint8_t n = 12;
		uint8_t scale = 0;
		note -= 7;
		while (n--)
		{
			scale = scale_start + i;
			if (is_element_in_array(k_sharp_semitone_table, scale % 12, sizeof(k_sharp_semitone_table))) {
				if (found == note) {
					ESP_LOGI(TAG, "found half scale = %d", scale);
					break;
				}
				found++;
			}
			i++;
		}
		if (n == 0 && found == 0) {
			ESP_LOGW(TAG, "scale notfound, start %d, note %d", scale_start, note + 7);
			return 255;
		}
		return scale;
	}
}

static void app_midi_press_note_key(uint8_t note)
{
	midi_note_t &n = notes[note];

	n.channel = current_channel;
	// n.scale = app_midi_search_scale(note);
	n.scale = scale_lut[note];
	if (scale_up) {
		n.scale += 12;
	}
	n.strength = channel_strength[current_channel];
	n.time = millis();
	n.status = MIDI_NOTE_STATUS_START;
	n.id = note_id++;

	ESP_LOGI(TAG, "[pressed] channel: %d, scale: %d, strength: %d", n.channel, n.scale, n.strength);
	ps_pub_buf_create("midi.note", &n, sizeof(midi_note_t));
	app_midi_write(n.channel, MIDI_CMD_NOTE_ON, n.scale, n.strength);
}

static void app_midi_release_note_key(uint8_t note)
{
	midi_note_t &n = notes[note];
	n.time = millis() - n.time;
	n.status = MIDI_NOTE_STATUS_END;

	ESP_LOGI(TAG, "[released] channel: %d, scale: %d, time: %d", n.channel, n.scale, n.time);
	ps_pub_buf_create("midi.note", &n, sizeof(midi_note_t));
	app_midi_write(n.channel, MIDI_CMD_NOTE_OFF, n.scale, n.strength);
}

static void app_midi_process_key(uint8_t key, uint8_t action)
{
	ESP_LOGI(TAG, "Key %d, %s", key, action == ACTION_PRESSED ? "pressed" : "released");
	
	if (key < sizeof(k_key_tone_map)) {
		uint8_t note = k_key_tone_map[key];
		if (note != 255) {
			if (action == ACTION_PRESSED) {
				app_midi_press_note_key(note);
			} 
			else {
				app_midi_release_note_key(note);
			}
			return;
		}
	}

	if (key < sizeof(k_key_channel_map)) {
		uint8_t ch = k_key_channel_map[key];
		if (ch != 255) {
			current_channel = ch;
			PS_PUB_INT("midi.ch", current_channel);
			PS_PUB_DBL("midi.str", channel_strength[current_channel] / 127.0f);
			PS_PUB_DBL("midi.pres", channel_pressure[current_channel] / 127.0f);
			return;
		}
	}

	if (key == KEY_ID_BTN_0 && action == ACTION_PRESSED) { // reset instrument
		channel_instruments[current_channel] = 0;
		app_midi_change_program(current_channel, channel_instruments[current_channel]);
		PS_PUB_INT("midi.prog", current_channel << 8 | channel_instruments[current_channel]);
	}

	if (key == KEY_ID_BTN_1 && action == ACTION_PRESSED) { // switch knobs
		knob_func_sel++;
		if (knob_func_sel >= KNOB_FUNC_MAX) {
			knob_func_sel = 0;
		}
		PS_PUB_INT("midi.knob", knob_func_sel);
		ESP_LOGI(TAG, "knob_func_sel %d", knob_func_sel);
	}

	if (key == 6) {
		if (action == ACTION_PRESSED) {
			scale_up = true;
		}
		else {
			scale_up = false;
		}
	}

}

static void app_midi_process_enc0(int inc)
{
	channel_instruments[current_channel] += inc;
	if (channel_instruments[current_channel] < 0) {
		channel_instruments[current_channel] = MIDI_SOUNDS_MAX - abs(channel_instruments[current_channel]);
	}
	else if (channel_instruments[current_channel] >= MIDI_SOUNDS_MAX) {
		channel_instruments[current_channel] = channel_instruments[current_channel] - MIDI_SOUNDS_MAX;
	}
	app_midi_change_program(current_channel, channel_instruments[current_channel]);
	ESP_LOGI(TAG, "instrument: %d, inc: %d", channel_instruments[current_channel], inc);
	PS_PUB_INT("midi.prog", current_channel << 8 | channel_instruments[current_channel]);
}

static float app_midi_process_channel_data(int16_t &channel_data, int inc)
{
	channel_data += inc;
	if (channel_data < 0) {
		channel_data = 0;
		return 0.0;
	}
	if (channel_data > 127) {
		channel_data = 127;
		return 1.0;
	}
	return (channel_data / 127.0f);
}

static void app_midi_process_enc1(int inc)
{
	float pct = 0;
	if (knob_func_sel == KNOB_FUNC_VOL) {
		pct = app_midi_process_channel_data(channel_strength[current_channel], inc);
		ESP_LOGI(TAG, "channel %d strength %d", current_channel, channel_strength[current_channel]);
		PS_PUB_DBL("midi.str", pct);
		return;
	}

	if (knob_func_sel == KNOB_FUNC_PRESSURE) {
		pct = app_midi_process_channel_data(channel_pressure[current_channel], inc);
		app_midi_change_pressure(current_channel, channel_pressure[current_channel]);
		ESP_LOGI(TAG, "channel %d pressure %d", current_channel, channel_pressure[current_channel]);
		PS_PUB_DBL("midi.pres", pct);
		return;
	}
}

static void app_midi_task(void *args)
{
	memset(notes, 0, sizeof(notes));
	memset(channel_pressure, 0, sizeof(channel_pressure));

	// MIDISerial.write(MIDI_CMD_PITCH_BEND + 0);
	// MIDISerial.write(0x00);
	// MIDISerial.write(0x3F);

	for (int i = 0; i < MIDI_MAX_CHANNELS; i++) {
		channel_instruments[i] = i;
		app_midi_change_program(i, i);
		PS_PUB_INT("midi.prog", i << 8 | i);
		app_midi_change_pressure(i, 0);
		channel_strength[i] = 127;
		channel_pressure[i] = 0;
	}
	PS_PUB_DBL("midi.str", channel_strength[current_channel] / 127.0f);
	PS_PUB_DBL("midi.pres", channel_pressure[current_channel] / 127.0f);
	PS_PUB_INT("midi.ss", scale_start);
	PS_PUB_INT("midi.knob", knob_func_sel);

	app_midi_test();


	while (1)
	{
		msg = ps_get(midi_sub, -1);

		if (ps_has_topic(msg, "input.key"))
		{
			uint8_t key = msg->int_val & 0x3F;
			uint8_t action = msg->int_val & 0xC0;
			ps_unref_msg(msg);
			app_midi_process_key(key, action);
			continue;
		}
		else if (ps_has_topic(msg, "input.enc0"))
		{
			app_midi_process_enc0(msg->int_val);
		}
		else if (ps_has_topic(msg, "input.enc1"))
		{
			app_midi_process_enc1(msg->int_val);
		}
		else if (ps_has_topic(msg, "input.vr"))
		{
			int vr = (int)msg->int_val;
			if (vr > 0) {
				scale_start += 12;
			}
			else if (vr < 0) {
				scale_start -= 12;
			}
			// scale_start = (vr / 100) + scale_start;
			if (scale_start < MIDI_SCALE_C_MINUS1) {
				scale_start = MIDI_SCALE_C_MINUS1;
			}
			if (scale_start > MIDI_SCALE_C8) {
				scale_start = MIDI_SCALE_C8;
			}
			// if (is_element_in_array(k_sharp_semitone_table, scale_start % 12, sizeof(k_sharp_semitone_table))) {
			// 	scale_start--;
			// }
			for (int i = 0; i < 12; i++) {
				scale_lut[i] = app_midi_search_scale(i);
			}
			ESP_LOGI(TAG, "vr = %d, new scale_start = %d", vr, scale_start);
			PS_PUB_INT("midi.ss", scale_start);
		}

		ps_unref_msg(msg);
		
	}
}

void app_midi_task_init()
{
	MIDISerial.begin(31250, SERIAL_8N1, -1, 19);

	for (int i = 0; i < 12; i++) {
		scale_lut[i] = app_midi_search_scale(i);
	}

	seq.begin();
	seq.setMidiHandler(app_midi_write);
	seq.setStepHandler(app_midi_step_cb);
	midi_sub = ps_new_subscriber(15, PS_STRLIST("input.key", "input.enc0", "input.enc1", "input.vr"));
	xTaskCreatePinnedToCore(&app_midi_task, "midiTask", 4 * 1024, NULL, 15, NULL, 1);
}
