/*	TWAI Network receive Example

	This example code is in the Public Domain (or CC0 licensed, at your option.)

	Unless required by applicable law or agreed to in writing, this
	software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
	CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/twai.h" // Update from V4.2

#include "frame.h"

static const char *TAG = "TWAI_V5";

extern QueueHandle_t xQueueTwai;

static const twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

#if CONFIG_CAN_BITRATE_25
static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_25KBITS();
#define BITRATE "Bitrate is 25 Kbit/s"
#elif CONFIG_CAN_BITRATE_50
static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_50KBITS();
#define BITRATE "Bitrate is 50 Kbit/s"
#elif CONFIG_CAN_BITRATE_100
static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_100KBITS();
#define BITRATE "Bitrate is 100 Kbit/s"
#elif CONFIG_CAN_BITRATE_125
static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_125KBITS();
#define BITRATE "Bitrate is 125 Kbit/s"
#elif CONFIG_CAN_BITRATE_250
static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_250KBITS();
#define BITRATE "Bitrate is 250 Kbit/s"
#elif CONFIG_CAN_BITRATE_500
static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
#define BITRATE "Bitrate is 500 Kbit/s"
#elif CONFIG_CAN_BITRATE_800
static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_800KBITS();
#define BITRATE "Bitrate is 800 Kbit/s"
#elif CONFIG_CAN_BITRATE_1000
static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_1MBITS();
#define BITRATE "Bitrate is 1 Mbit/s"
#endif

static const twai_general_config_t g_config =
    TWAI_GENERAL_CONFIG_DEFAULT(CONFIG_CTX_GPIO, CONFIG_CRX_GPIO, TWAI_MODE_NORMAL);

// Format and print the twai message
void twai_print_frame(twai_message_t frame) {
    int ext = frame.extd;
    int rtr = frame.rtr;

    if (ext == 0) {
        printf("Standard ID: 0x%03"PRIx32"%*s", frame.identifier, 5, "");
    } else {
        printf("Extended ID: 0x%08"PRIx32, frame.identifier);
    }
    printf("  DLC: %d Data: ", frame.data_length_code);

    if (rtr == 0) {
        for (int i = 0; i < frame.data_length_code; i++) {
            printf("0x%02x ", frame.data[i]);
        }
    } else {
        printf("REMOTE REQUEST FRAME");
    }
    printf("\n");
}

void twai_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Start");
    ESP_LOGI(TAG, "TWAI_BITRATE=%d",CONFIG_TWAI_BITRATE);
    ESP_LOGI(TAG, "CTX_GPIO=%d",CONFIG_CTX_GPIO);
    ESP_LOGI(TAG, "CRX_GPIO=%d",CONFIG_CRX_GPIO);

    ESP_ERROR_CHECK(twai_driver_install(&g_config, &t_config, &f_config));
    ESP_LOGI(TAG, "Driver installed");
    ESP_ERROR_CHECK(twai_start());
    ESP_LOGI(TAG, "Driver started");

	while (1) {
		twai_message_t rx_msg;
		esp_err_t ret = twai_receive(&rx_msg, portMAX_DELAY);
		if (ret == ESP_OK) {
			ESP_LOGD(TAG,"twai_receive identifier=0x%"PRIx32" flags=0x%"PRIx32" data_length_code=%d",
				rx_msg.identifier, rx_msg.flags, rx_msg.data_length_code);

#if CONFIG_ENABLE_PRINT
			twai_print_frame(rx_msg);
#endif
			my_twai_frame_t my_frame;
			my_frame.extd = rx_msg.extd;
			my_frame.rtr = rx_msg.rtr;
			my_frame.identifier = rx_msg.identifier;
			my_frame.data_length_code = rx_msg.data_length_code;
			for(int i=0;i<rx_msg.data_length_code;i++) my_frame.data[i] = rx_msg.data[i];
			if (xQueueSend(xQueueTwai, &my_frame, 100) != pdPASS) {
				ESP_LOGE(TAG, "xQueueSend Fail");
				break;
			}

		} else {
			ESP_LOGE(TAG, "twai_receive Fail %s", esp_err_to_name(ret));
			break;
		}
	} // end while

	vTaskDelete(NULL);
}

