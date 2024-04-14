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
#include "esp_system.h"
#include "esp_log.h"
#include "driver/twai.h" // Update from V4.2
#include "lwip/sockets.h"
#include "mdns.h"

#include <netdb.h> // hostent

static const char *TAG = "TCP-CLIENT";

extern QueueHandle_t xQueueTwai;

esp_err_t connect_server(int *sock, int port, char *host) {
	int _sock;
	int addr_family = AF_INET;
	int ip_protocol = IPPROTO_IP;

	struct sockaddr_in dest_addr;
	memset(&dest_addr, 0, sizeof(dest_addr));
	dest_addr.sin_family = AF_INET;
	//dest_addr.sin_port = htons(CONFIG_TCP_PORT);
	//dest_addr.sin_addr.s_addr = inet_addr(CONFIG_TCP_HOST);
	dest_addr.sin_port = htons(port);
	dest_addr.sin_addr.s_addr = inet_addr(host);
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

int format_text(twai_message_t rx_msg, char * buffer, int blen);
int format_json(twai_message_t rx_msg, char * buffer, int blen);
int format_xml(twai_message_t rx_msg, char * buffer, int blen);
int format_csv(twai_message_t rx_msg, char * buffer, int blen);

esp_err_t query_mdns_host(const char * host_name, char *ip)
{
	ESP_LOGD(__FUNCTION__, "Query A: %s", host_name);

	struct esp_ip4_addr addr;
	addr.addr = 0;

	esp_err_t err = mdns_query_a(host_name, 10000,	&addr);
	if(err){
		if(err == ESP_ERR_NOT_FOUND){
			ESP_LOGW(__FUNCTION__, "%s: Host was not found!", esp_err_to_name(err));
			return ESP_FAIL;
		}
		ESP_LOGE(__FUNCTION__, "Query Failed: %s", esp_err_to_name(err));
		return ESP_FAIL;
	}

	ESP_LOGD(__FUNCTION__, "Query A: %s.local resolved to: " IPSTR, host_name, IP2STR(&addr));
	sprintf(ip, IPSTR, IP2STR(&addr));
	return ESP_OK;
}

void convert_mdns_host(char * from, char * to)
{
	ESP_LOGI(__FUNCTION__, "from=[%s]",from);
	strcpy(to, from);
	char *sp;
	sp = strstr(from, ".local");
	if (sp == NULL) return;

	int _len = sp - from;
	ESP_LOGD(__FUNCTION__, "_len=%d", _len);
	char _from[128];
	strcpy(_from, from);
	_from[_len] = 0;
	ESP_LOGI(__FUNCTION__, "_from=[%s]", _from);

	char _ip[128];
	esp_err_t ret = query_mdns_host(_from, _ip);
	ESP_LOGI(__FUNCTION__, "query_mdns_host=%d _ip=[%s]", ret, _ip);
	if (ret != ESP_OK) return;

	strcpy(to, _ip);
	ESP_LOGI(__FUNCTION__, "to=[%s]", to);
}

void tcp_client_task(void *pvParameters)
{
	ESP_LOGI(TAG, "Start TCP HOST=[%s] TCP PORT=%d", CONFIG_TCP_HOST, CONFIG_TCP_PORT);

	// Initialize mDNS
	ESP_ERROR_CHECK( mdns_init() );

	// Resolve mDNS host name
	char host[128];
	ESP_LOGI(TAG, "CONFIG_TCP_HOST=[%s]", CONFIG_TCP_HOST);
	convert_mdns_host(CONFIG_TCP_HOST, host);
	ESP_LOGI(TAG, "host=[%s]", host);

	int sock;
	int ret;
	bool connected = false;
	twai_message_t rx_msg;
	char buffer[512];
	while (1) {
		BaseType_t err = xQueueReceive(xQueueTwai, &rx_msg, portMAX_DELAY);
		if (err == pdTRUE) {
			ESP_LOGI(TAG, "connected=%d", connected);
			if (!connected) {
				esp_err_t err = connect_server(&sock, CONFIG_TCP_PORT, host);
				ESP_LOGI(TAG, "connect_server err=%d", err);
				if (err != ESP_OK) continue;
				connected = true;
			}

			ESP_LOGI(TAG,"twai_receive identifier=0x%"PRIx32" flags=0x%"PRIx32" data_length_code=%d",
				rx_msg.identifier, rx_msg.flags, rx_msg.data_length_code);
#if CONFIG_FORMAT_TEXT
			format_text(rx_msg, buffer, sizeof(buffer)-1);
#elif CONFIG_FORMAT_JSON
			format_json(rx_msg, buffer, sizeof(buffer)-1);
#elif CONFIG_FORMAT_XML
			format_xml(rx_msg, buffer, sizeof(buffer)-1);
#elif CONFIG_FORMAT_CSV
			format_csv(rx_msg, buffer, sizeof(buffer)-1);
#endif
			ret = send(sock, buffer, strlen(buffer), 0);
			ESP_LOGI(TAG, "send ret=%d",ret);
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
