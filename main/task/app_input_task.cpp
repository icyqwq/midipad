#include "app_task.h"
#include "app_common.h"
#include <deque>

#define TAG "Input"

#define IIC_FREQ 400000L
#define PIN_K14 35
#define PIN_VR  34
#define VR_MAX 1000
#define VR_MIN 520
#define VR_MIDDLE 750
#define VR_DEAD_ZONE 100

/* KEY MAP
		|---------------------------|
		|							|
		|							|
		|							|
		|			CORE 2			|
		|							|	[Encoder 0]
		|							|
		|							|	[Encoder 1]
		|___________________________|
		(VR)								
											[5]
			[0]		[1]		[2]		[3] 	[6]

		[4]		[8]		[9]		[10]	[11]

		[12]	[13]	[14]	[15]	[21]

		[16]	[17]	[18]	[19]	[20]

*/

typedef struct
{
	union
	{
		struct
		{
			uint8_t k1 : 1;//0
			uint8_t k2 : 1;//1
			uint8_t k3 : 1;//2
			uint8_t k4 : 1;//3
			uint8_t k5 : 1;//4
			uint8_t p1 : 1;//5
			uint8_t p2 : 1;//6
			uint8_t : 1; //7 not used
			uint8_t k6 : 1;//8
			uint8_t k7 : 1;//9
			uint8_t k8 : 1;//10
			uint8_t k9 : 1;//11
			uint8_t k10 : 1;//12
			uint8_t k11 : 1;//13
			uint8_t k12 : 1;//14
			uint8_t k13 : 1;//15
			uint8_t k15 : 1;//16
			uint8_t k16 : 1;//17
			uint8_t k17 : 1;//18
			uint8_t k18 : 1;//19
			uint8_t k19 : 1;//20
			uint8_t k14 : 1;//21
			uint8_t btn0 : 1;//22
			uint8_t btn1 : 1;//23
		};
		uint8_t key_buffer[3];
	};

	union
	{
		struct
		{
			int32_t cnt0;
			int32_t cnt1;
		};
		uint8_t enc_cnt_buffer[8];
	};

	union
	{
		struct
		{
			int32_t inc0;
			int32_t inc1;
		};
		uint8_t enc_inc_buffer[8];
	};

	union
	{
		struct
		{
			uint8_t btn0 : 1;
			uint8_t btn1 : 1;
		} encoder;
		uint8_t enc_btn_value;
	};

	uint16_t vr_val;

} input_data_t;

static SemaphoreHandle_t input_semaphore = NULL;
static ps_subscriber_t *input_sub = nullptr;
static ps_msg_t *msg = nullptr;
static input_data_t input_data;
static input_data_t input_data_last;
static uint32_t vr_last_send_time = 0;

static uint8_t app_input_update()
{
	uint8_t ret = true;

	input_data_last = input_data;

	input_data.key_buffer[0] = M5.In_I2C.readRegister8(0x58, 0x00, IIC_FREQ);
	if (!M5.In_I2C.readRegister(0x58, 0x10, input_data.enc_cnt_buffer, 8, IIC_FREQ)) {
		ESP_LOGW(TAG, "Input update failed, Addr: 0x58, Reg: 0x10");
		ret = false;
	}
	if (!M5.In_I2C.readRegister(0x58, 0x20, input_data.enc_inc_buffer, 8, IIC_FREQ)) {
		ESP_LOGW(TAG, "Input update failed, Addr: 0x58, Reg: 0x20");
		ret = false;
	}
	input_data.enc_btn_value = M5.In_I2C.readRegister8(0x58, 0x40, IIC_FREQ);

	if (!M5.In_I2C.readRegister(0x59, 0x00, input_data.key_buffer + 1, 2, IIC_FREQ)) {
		ESP_LOGW(TAG, "Input update failed, Addr: 0x59, Reg: 0x00");
		ret = false;
	}

	input_data.k14 = digitalRead(PIN_K14);
	input_data.vr_val = analogRead(PIN_VR);

	input_data.btn0 = input_data.encoder.btn0;
	input_data.btn1 = input_data.encoder.btn1;

	return ret;
}

static void app_input_check_status() 
{
#if 0
	printf("Keys: %d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d %d%d\n",
           input_data.k1, input_data.k2, input_data.k3, input_data.k4, input_data.k5,
           input_data.k6, input_data.k7, input_data.k8, input_data.k9, input_data.k10, input_data.k11, input_data.k12,
           input_data.k13, input_data.k14, input_data.k15, input_data.k16, input_data.k17, input_data.k18, input_data.k19, input_data.p1, input_data.p2);
	printf("Encoder0: %d, %d, %d %d | Encoder1: %d, %d, %d %d\n", input_data.cnt0, input_data.inc0, input_data.btn0, input_data.encoder.btn0, input_data.cnt1, input_data.inc1, input_data.btn1, input_data.encoder.btn1);
	printf("VR: %u\n", input_data.vr_val);
#endif

	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 8; j++) {
			uint8_t last = (input_data_last.key_buffer[i] >> j) & 0x1;
			uint8_t current = (input_data.key_buffer[i] >> j) & 0x1;
			if (!current && last) {
				PS_PUB_INT("input.key", ( i * 8 + j) | 0x80);
				// ESP_LOGI(TAG, "%d pressed\n", i * 8 + j);
			} 
			else if(current && !last) {
				PS_PUB_INT("input.key",  i * 8 + j);
				// ESP_LOGI(TAG, "%d released\n", i * 8 + j);
			}  
		}
	}
	if (input_data.inc0 != 0) {
		PS_PUB_INT("input.enc0", 1);
	}
	if (input_data.inc1 != 0) {
		PS_PUB_INT("input.enc1", input_data.inc1);
	}

	int diff = ((int)input_data.vr_val) - (int)VR_MIDDLE;
	if (abs(diff) > VR_DEAD_ZONE) {
		if (millis() - vr_last_send_time > 300) {
			vr_last_send_time = millis();
			PS_PUB_INT("input.vr", diff);
		}
	}
}

static void app_input_task(void *args)
{
	uint8_t ret = true;

	vTaskDelay(100);
	app_input_update();
	app_input_update(); // dummy read at start

	while (1)
	{
		msg = ps_get(input_sub, -1);
		if (PS_IS_NIL(msg)) { // timetick event
			ret = app_input_update();
			if (!ret) {
				M5.Rtc.setAlarmIRQ(1);
				M5.Power.Axp192.powerOff();
				vTaskDelay(1000);
				ps_unref_msg(msg);
				continue;
			}
			app_input_check_status();
		}
		ps_unref_msg(msg);
	}
}

void app_input_task_init()
{
	pinMode(PIN_K14, INPUT);
	input_sub = ps_new_subscriber(10, PS_STRLIST("t"));
	xTaskCreatePinnedToCore(&app_input_task, "inputTask", 4 * 1024, NULL, 10, NULL, 1);
}
