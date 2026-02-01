#include "network.h"
#include "wifi.h"

#include "zephyr/kernel.h"
#include "zephyr/logging/log.h"

LOG_MODULE_REGISTER(network, CONFIG_LOG_DEFAULT_LEVEL);

K_EVENT_DEFINE(network_events);

static void network_event_forwarder(void *arg1, void *arg2, void *arg3)
{
	while (1) {
		uint32_t events = k_event_wait(&wifi_events,
					      WIFI_EVENT_DISCONNECTED | WIFI_EVENT_IP_ACQUIRED,
					      false, K_FOREVER);

		if (events & WIFI_EVENT_DISCONNECTED) {
			k_event_set(&network_events, NETWORK_EVENT_DISCONNECTED);
			k_event_clear(&wifi_events, WIFI_EVENT_DISCONNECTED);
			LOG_INF("Network disconnected event forwarded");
		}

		if (events & WIFI_EVENT_IP_ACQUIRED) {
			k_event_set(&network_events, NETWORK_EVENT_CONNECTED);
			k_event_clear(&wifi_events, WIFI_EVENT_IP_ACQUIRED);
			LOG_INF("Network connected event forwarded");
		}
	}
}

K_THREAD_DEFINE(network_event_thread, 1024, network_event_forwarder, NULL, NULL, NULL, 7, 0, 0);
