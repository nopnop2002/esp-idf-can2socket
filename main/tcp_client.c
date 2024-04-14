/*	BSD Socket TCP Client

	This example code is in the Public Domain (or CC0 licensed, at your option.)

	Unless required by applicable law or agreed to in writing, this
	software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
	CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
//#include "freertos/task.h"
//#include "freertos/message_buffer.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/twai.h" // Update from V4.2
#include "lwip/sockets.h"

#include <netdb.h> // hostent

static const char *TAG = "TCP-CLIENT";

extern QueueHandle_t xQueueTwai;

esp_err_t connect_server(int *sock) {
	int _sock;
	int addr_family = AF_INET;
	int ip_protocol = IPPROTO_IP;

	struct sockaddr_in dest_addr;
	memset(&dest_addr, 0, sizeof(dest_addr));
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(CONFIG_TCP_PORT);
	dest_addr.sin_addr.s_addr = inet_addr(CONFIG_TCP_HOST);
	ESP_LOGD(TAG, "dest_addr.sin_addr.s_addr=0x%"PRIx32, dest_addr.sin_addr.s_addr);
	if (dest_addr.sin_addr.s_addr == 0xffffffff) {
		ESP_LOGI(TAG, "convert from host to ip");
		struct hostent *hp;
		hp = gethostbyname(CONFIG_TCP_HOST);
		if (hp == NULL) {
			ESP_LOGE(TAG, "gethostbyname fail");
			while(1) { vTaskDelay(1); }
		}
		struct ip4_addr *ip4_addr;
		ip4_addr = (struct ip4_addr *)hp->h_addr;
		dest_addr.sin_addr.s_addr = ip4_addr->addr;
	}
	ESP_LOGD(TAG, "dest_addr.sin_addr.s_addr=0x%"PRIx32, dest_addr.sin_addr.s_addr);

	_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
	if (_sock < 0) {
		ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
		return ESP_FAIL;
	}
	ESP_LOGI(TAG, "Socket created, connecting to %s:%d", CONFIG_TCP_HOST, CONFIG_TCP_PORT);

	int err = connect(_sock, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_in6));
	if (err == 0) {
		ESP_LOGI(TAG, "Successfully connected");
		*sock = _sock;
		return ESP_OK;
	} else {
		ESP_LOGW(TAG, "Socket unable to connect: errno %d", errno);
		close(_sock);
		return ESP_FAIL;
	}
}

void tcp_client_task(void *pvParameters)
{
	ESP_LOGI(TAG, "Start TCP HOST=[%s] TCP PORT=%d", CONFIG_TCP_HOST, CONFIG_TCP_PORT);

	int sock;
	int ret;
	bool connected = false;
	twai_message_t rx_msg;
	char buffer[128];
	char wk[128];
	while (1) {
		BaseType_t err = xQueueReceive(xQueueTwai, &rx_msg, portMAX_DELAY);
		if (err == pdTRUE) {
			ESP_LOGI(TAG, "connected=%d", connected);
			if (!connected) {
				esp_err_t err = connect_server(&sock);
				ESP_LOGI(TAG, "connect_server err=%d", err);
				if (err != ESP_OK) continue;
				connected = true;
			}

			ESP_LOGI(TAG,"twai_receive identifier=0x%"PRIx32" flags=0x%"PRIx32" data_length_code=%d",
				rx_msg.identifier, rx_msg.flags, rx_msg.data_length_code);
			//int ext = rx_msg.flags & 0x01; // flags is Deprecated
			//int rtr = rx_msg.flags & 0x02; // flags is Deprecated
			int ext = rx_msg.extd;
			int rtr = rx_msg.rtr;
			ESP_LOGI(TAG, "ext=%x rtr=%x", ext, rtr);
			if (ext == 0) {
				sprintf(buffer, "Standard ID: 0x%03"PRIx32"		", rx_msg.identifier);
			} else {
				sprintf(buffer, "Extended ID: 0x%08"PRIx32, rx_msg.identifier);
			}

			sprintf(wk, " DLC: %d	 Data: ", rx_msg.data_length_code);
			strcat(buffer, wk);

			if (rtr == 0) {
				for (int i = 0; i < rx_msg.data_length_code; i++) {
					sprintf(wk, "0x%02x ", rx_msg.data[i]);
					strcat(buffer, wk);
				}
			} else {
				sprintf(wk, "REMOTE REQUEST FRAME");
				strcat(buffer, wk);
			}
			ret = send(sock, buffer, strlen(buffer), 0);
			if (ret < 0) {
				ESP_LOGW(TAG, "send fail ret=%d", ret);
				connected = false;
			}
		} else {
			ESP_LOGE(TAG, "xQueueReceive fail");
			break;
		}
	} // end while

	// Close socket
	shutdown(sock, 0);
	close(sock);
	vTaskDelete(NULL);
}
