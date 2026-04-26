/*
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
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_err.h"
#include "esp_log.h"
#include "cJSON.h"

#include "frame.h"

static const char *TAG = "MAIN";

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static int s_retry_num = 0;

QueueHandle_t xQueueTwai;

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
		esp_wifi_connect();
	} else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
		if (s_retry_num < CONFIG_ESP_MAXIMUM_RETRY) {
			esp_wifi_connect();
			s_retry_num++;
			ESP_LOGI(TAG, "retry to connect to the AP");
		} else {
			xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
		}
		ESP_LOGI(TAG,"connect to the AP fail");
	} else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
		ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
		ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
		s_retry_num = 0;
		xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
	}
}

esp_err_t wifi_init_sta(void)
{
	s_wifi_event_group = xEventGroupCreate();

	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	esp_netif_create_default_wifi_sta();

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	esp_event_handler_instance_t instance_any_id;
	esp_event_handler_instance_t instance_got_ip;
	ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
		ESP_EVENT_ANY_ID,
		&event_handler,
		NULL,
		&instance_any_id));
	ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
		IP_EVENT_STA_GOT_IP,
		&event_handler,
		NULL,
		&instance_got_ip));

	wifi_config_t wifi_config = {
		.sta = {
			.ssid = CONFIG_ESP_WIFI_SSID,
			.password = CONFIG_ESP_WIFI_PASSWORD,
			/* Setting a password implies station will connect to all security modes including WEP/WPA.
			 * However these modes are deprecated and not advisable to be used. Incase your Access point
			 * doesn't support WPA2, these mode can be enabled by commenting below line */
			.threshold.authmode = WIFI_AUTH_WPA2_PSK,

			.pmf_cfg = {
				.capable = true,
				.required = false
			},
		},
	};
	ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());

	/* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
	 * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
	esp_err_t ret_value = ESP_OK;
	EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
		WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
		pdFALSE,
		pdFALSE,
		portMAX_DELAY);

	/* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
	 * happened. */
	if (bits & WIFI_CONNECTED_BIT) {
		ESP_LOGI(TAG, "connected to ap SSID:%s password:%s", CONFIG_ESP_WIFI_SSID, CONFIG_ESP_WIFI_PASSWORD);
	} else if (bits & WIFI_FAIL_BIT) {
		ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s", CONFIG_ESP_WIFI_SSID, CONFIG_ESP_WIFI_PASSWORD);
		ret_value = ESP_FAIL;
	} else {
		ESP_LOGE(TAG, "UNEXPECTED EVENT");
		ret_value = ESP_FAIL;
	}

	/* The event will not be processed after unregister */
	ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
	ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
	vEventGroupDelete(s_wifi_event_group);
	return ret_value;
}

void udp_client_task(void *pvParameters);
void tcp_client_task(void *pvParameters);
void twai_task(void *pvParameters);

int format_text(my_twai_frame_t rx_msg, char * buffer, int blen) {
	char wk[128];
	if (rx_msg.extd == 0) {
		sprintf(buffer, "Standard ID: 0x%03"PRIx32"%*s", rx_msg.identifier, 5, "");
	} else {
		sprintf(buffer, "Extended ID: 0x%08"PRIx32, rx_msg.identifier);
	}

	sprintf(wk, "  DLC: %d	Data: ", rx_msg.data_length_code);
	strcat(buffer, wk);

	if (rx_msg.rtr == 0) {
		for (int i = 0; i < rx_msg.data_length_code; i++) {
			sprintf(wk, "0x%02x ", rx_msg.data[i]);
			strcat(buffer, wk);
		}
	} else {
		sprintf(wk, "REMOTE REQUEST FRAME");
		strcat(buffer, wk);
	}
	if (strlen(buffer) > blen) {
		ESP_LOGE(TAG, "buffer is too small");
		return -1;
	}
	return strlen(buffer);
}

int format_json(my_twai_frame_t rx_msg, char * buffer, int blen) {
	// JSON Serialize
	cJSON *root = cJSON_CreateObject();
	if (rx_msg.rtr == 0) {
		cJSON_AddStringToObject(root, "Type", "Data frame");
	} else {
		cJSON_AddStringToObject(root, "Type", "Remote frame");
	}
	if (rx_msg.extd == 0) {
		cJSON_AddStringToObject(root, "Format", "Standard frame");
	} else {
		cJSON_AddStringToObject(root, "Format", "Extended frame");
	}
	cJSON_AddNumberToObject(root, "ID", rx_msg.identifier);
	cJSON_AddNumberToObject(root, "Length", rx_msg.data_length_code);

	if (rx_msg.data_length_code > 0) {
		int i_numbers[8];
		for(int i=0;i<rx_msg.data_length_code;i++) {
			i_numbers[i] = rx_msg.data[i];
		}
		cJSON *intArray;
		intArray = cJSON_CreateIntArray(i_numbers, rx_msg.data_length_code);
		cJSON_AddItemToObject(root, "Data", intArray);
	}
	const char *my_json_string = cJSON_Print(root);
	ESP_LOGD(TAG, "my_json_string\n%s",my_json_string);
	strcpy(buffer, my_json_string);

	// Cleanup
	free((void *)my_json_string);
	cJSON_Delete(root);
	if (strlen(buffer) > blen) {
		ESP_LOGE(TAG, "buffer is too small");
		return -1;
	}
	return strlen(buffer);
}

int format_xml(my_twai_frame_t rx_msg, char * buffer, int blen) {
	char wk[128];
	strcpy(buffer, "<?xml version=\"1.0\"?>\n");
	strcat(buffer, "<can>\n");
	char TAB[2] = {0x09, 0x00};

	if (rx_msg.rtr == 0) {
		sprintf(wk, "%s<type>Data frame</type>\n", TAB);
	} else {
		sprintf(wk, "%s<type>Remote frame</type>\n", TAB);
	}
	strcat(buffer, wk);

	if (rx_msg.extd == 0) {
		sprintf(wk, "%s<format>Standard frame</format>\n", TAB);
		strcat(buffer, wk);
		sprintf(wk, "%s<id>0x%03"PRIx32"</id>\n", TAB, rx_msg.identifier);
		strcat(buffer, wk);
	} else {
		sprintf(wk, "%s<format>Extended frame</format>\n", TAB);
		strcat(buffer, wk);
		sprintf(wk, "%s<id>0x%08"PRIx32"</id>\n", TAB, rx_msg.identifier);
		strcat(buffer, wk);
	}

	sprintf(wk, "%s<length>%d</length>\n", TAB, rx_msg.data_length_code);
	strcat(buffer, wk);

	if (rx_msg.rtr == 0) {
		sprintf(wk, "%s<data>\n", TAB);
		strcat(buffer, wk);
		for (int i = 0; i < rx_msg.data_length_code; i++) {
			sprintf(wk, "%s%s<data%d>0x%02x</data%d>\n", TAB, TAB, i, rx_msg.data[i], i);
			strcat(buffer, wk);
		}
		sprintf(wk, "%s</data>\n", TAB);
		strcat(buffer, wk);
	}

	strcat(buffer, "</can>\n");
	if (strlen(buffer) > blen) {
		ESP_LOGE(TAG, "buffer is too small");
		return -1;
	}
	return strlen(buffer);
}

int format_csv(my_twai_frame_t rx_msg, char * buffer, int blen) {
	char wk[128];
	if (rx_msg.rtr == 0) {
		strcpy(buffer, "\"Data frame\",");
	} else {
		strcpy(buffer, "\"Remote frame\",");
	}

	if (rx_msg.extd == 0) {
		sprintf(wk, "\"Standard\",0x%03"PRIx32",", rx_msg.identifier);
	} else {
		sprintf(wk, "\"Extended\",0x%08"PRIx32",", rx_msg.identifier);
	}
	strcat(buffer, wk);

	sprintf(wk, "%d,", rx_msg.data_length_code);
	strcat(buffer, wk);

	if (rx_msg.rtr == 0) {
		for (int i=0;i<6;i++) {
			if (i < rx_msg.data_length_code) {
				sprintf(wk, "0x%02x,", rx_msg.data[i]);
			} else {
				sprintf(wk, ",");
			}
			strcat(buffer, wk);
		}
	} else {
		sprintf(wk, ",,,,,");
		strcat(buffer, wk);
	}
	if (strlen(buffer) > blen) {
		ESP_LOGE(TAG, "buffer is too small");
		return -1;
	}
	ESP_LOGD(TAG, "[%s]", buffer);
	return strlen(buffer);
}


void app_main()
{
	// Initialize NVS
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
	  ESP_ERROR_CHECK(nvs_flash_erase());
	  ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	// WiFi initialize
	ESP_ERROR_CHECK(wifi_init_sta());

	// Create Queue
	xQueueTwai = xQueueCreate( 10, sizeof(my_twai_frame_t) );
	configASSERT( xQueueTwai );

	// Start tasks
#if CONFIG_PROTOCOL_TCP
	xTaskCreate(tcp_client_task, "TCP", 1024*4, NULL, 2, NULL);
#endif
#if CONFIG_PROTOCOL_UDP
	xTaskCreate(udp_client_task, "UDP", 1024*4, NULL, 2, NULL);
#endif
	xTaskCreate(twai_task, "TWAI", 1024*6, NULL, 2, NULL);
}
