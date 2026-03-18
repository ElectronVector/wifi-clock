#ifndef WIFI_H
#define WIFI_H

#include <zephyr/kernel.h>
#include <zephyr/zbus/zbus.h>

#define WIFI_EVENT_CONNECTED    BIT(0)
#define WIFI_EVENT_DISCONNECTED BIT(1)
#define WIFI_EVENT_IP_ACQUIRED  BIT(2)
#define WIFI_EVENT_DNS_CONFIGURED BIT(3)

struct wifi_event_msg {
	uint32_t events;
};

ZBUS_OBS_DECLARE(network_wifi_sub);
ZBUS_CHAN_DECLARE(wifi_event_chan);

int wifi_connect();

#endif //WIFI_H
