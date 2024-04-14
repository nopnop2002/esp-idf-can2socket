/*	TWAI Network Example

	This example code is in the Public Domain (or CC0 licensed, at your option.)

	Unless required by applicable law or agreed to in writing, this
	software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
	CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/twai.h" // Update from V4.2

static const char *TAG = "TWAI";

extern QueueHandle_t xQueueTwai;

void twai_task(void *pvParameters)
{
	ESP_LOGI(TAG,"task start");

	twai_message_t rx_msg;
	while (1) {
		//esp_err_t ret = twai_receive(&rx_msg, pdMS_TO_TICKS(10));
		esp_err_t ret = twai_receive(&rx_msg, portMAX_DELAY);
		if (ret == ESP_OK) {
			ESP_LOGD(TAG,"twai_receive identifier=0x%"PRIx32" flags=0x%"PRIx32" data_length_code=%d",
				rx_msg.identifier, rx_msg.flags, rx_msg.data_length_code);

			//int ext = rx_msg.flags & 0x01; // flags is Deprecated
			//int rtr = rx_msg.flags & 0x02; // flags is Deprecated
			int ext = rx_msg.extd;
			int rtr = rx_msg.rtr;
			ESP_LOGD(TAG, "ext=%x rtr=%x", ext, rtr);

#if CONFIG_ENABLE_PRINT
			if (ext == 0) {
				printf("Standard ID: 0x%03"PRIx32"     ", rx_msg.identifier);
			} else {
				printf("Extended ID: 0x%08"PRIx32, rx_msg.identifier);
			}
			printf(" DLC: %d	Data: ", rx_msg.data_length_code);

			if (rtr == 0) {
				for (int i = 0; i < rx_msg.data_length_code; i++) {
					printf("0x%02x ", rx_msg.data[i]);
				}
			} else {
				printf("REMOTE REQUEST FRAME");

			}
			printf("\n");
#endif
			if (xQueueSend(xQueueTwai, &rx_msg, portMAX_DELAY) != pdPASS) {
				ESP_LOGE(TAG, "xQueueSend Fail");
			}

		} else {
			ESP_LOGE(TAG, "twai_receive Fail %s", esp_err_to_name(ret));
		}
	} // end while

	// Never reach here
	vTaskDelete(NULL);
}

