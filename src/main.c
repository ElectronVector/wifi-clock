#include <stdio.h>

#include "wifi.h"
#include "zephyr/kernel.h"
#include "zephyr/logging/log.h"

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

int main(void)
{
	printf("Sup?? Hello World!! %s\n", CONFIG_BOARD);

	while (1) {
		uint32_t events = k_event_wait(&wifi_events, WIFI_EVENT_CONNECTED | WIFI_EVENT_DISCONNECTED | WIFI_EVENT_IP_ACQUIRED,
					      false, K_FOREVER);

		if (events & WIFI_EVENT_CONNECTED) {
			LOG_INF("Main thread: Wi-Fi connected");
			k_event_clear(&wifi_events, WIFI_EVENT_CONNECTED);
		}

		if (events & WIFI_EVENT_DISCONNECTED) {
			LOG_INF("Main thread: Wi-Fi disconnected");
			k_event_clear(&wifi_events, WIFI_EVENT_DISCONNECTED);
		}

		if (events & WIFI_EVENT_IP_ACQUIRED) {
			LOG_INF("Main thread: IP address acquired");
			k_event_clear(&wifi_events, WIFI_EVENT_IP_ACQUIRED);
		}
	}

	return 0;
}
