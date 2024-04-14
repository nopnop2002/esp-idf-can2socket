/*	BSD Socket UDP Client

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
#include "esp_system.h"
#include "esp_log.h"
#include "driver/twai.h" // Update from V4.2
#include "esp_netif.h" // IP2STR
#include "lwip/sockets.h"

static const char *TAG = "UDP-CLIENT";

int format_text(twai_message_t rx_msg, char * buffer, int blen);
int format_json(twai_message_t rx_msg, char * buffer, int blen);
int format_xml(twai_message_t rx_msg, char * buffer, int blen);

extern QueueHandle_t xQueueTwai;

void udp_client_task(void *pvParameters) {
	ESP_LOGI(TAG, "Start UDP PORT=%d", CONFIG_UDP_PORT);

	/* Get the local IP address */
	esp_netif_ip_info_t ip_info;
	ESP_ERROR_CHECK(esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"), &ip_info));
	ESP_LOGI(TAG, "ip_info.ip="IPSTR, IP2STR(&ip_info.ip));

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(CONFIG_UDP_PORT);
#if CONFIG_UDP_LIMITED_BROADCAST
	addr.sin_addr.s_addr = htonl(INADDR_BROADCAST); /* send message to 255.255.255.255 */
	//addr.sin_addr.s_addr = inet_addr("255.255.255.255"); /* send message to 255.255.255.255 */
#elif CONFIG_UDP_DIRECTED_BROADCAST
	char directed[20];
	sprintf(directed, "%d.%d.%d.255", esp_ip4_addr1(&ip_info.ip), esp_ip4_addr2(&ip_info.ip), esp_ip4_addr3(&ip_info.ip));
	ESP_LOGI(TAG, "directed=[%s]", directed);
	//addr.sin_addr.s_addr = inet_addr("192.168.10.255"); /* send message to xxx.xxx.xxx.255 */
	addr.sin_addr.s_addr = inet_addr(directed); /* send message to xxx.xxx.xxx.255 */
#elif CONFIG_UDP_MULTICAST
	addr.sin_addr.s_addr = inet_addr(CONFIG_UDP_MULTICAST_ADDRESS);
#elif CONFIG_UDP_UNICAST
	addr.sin_addr.s_addr = inet_addr(CONFIG_UDP_UNICAST_ADDRESS);
	//addr.sin_addr.s_addr = inet_addr("192.168.10.46");
#endif

	// create the socket
	int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP ); // Create a UDP socket.
	LWIP_ASSERT("sock >= 0", sock >= 0);

	int ret;
	twai_message_t rx_msg;
	char buffer[512];
	while(1) {
		BaseType_t err = xQueueReceive(xQueueTwai, &rx_msg, portMAX_DELAY);
		if (err == pdTRUE) {
			ESP_LOGI(TAG,"twai_receive identifier=0x%"PRIx32" flags=0x%"PRIx32" data_length_code=%d",
				rx_msg.identifier, rx_msg.flags, rx_msg.data_length_code);
#if CONFIG_FORMAT_TEXT
			format_text(rx_msg, buffer, sizeof(buffer)-1);
#elif CONFIG_FORMAT_JSON
			format_json(rx_msg, buffer, sizeof(buffer)-1);
#elif CONFIG_FORMAT_XML
			format_xml(rx_msg, buffer, sizeof(buffer)-1);
#endif
			int buflen = strlen(buffer);
			ret = sendto(sock, buffer, buflen, 0, (struct sockaddr *)&addr, sizeof(addr));
			LWIP_ASSERT("ret == buflen", ret == buflen);
			ESP_LOGI(TAG, "sendto ret=%d",ret);
		} else {
			ESP_LOGE(TAG, "xQueueReceive fail");
			break;
		}
	} // end while

	// Close socket
	ret = close(sock);
	vTaskDelete( NULL );
}

