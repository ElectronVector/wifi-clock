#ifndef WIFI_H
#define WIFI_H

#include <zephyr/kernel.h>

#define WIFI_EVENT_CONNECTED    BIT(0)
#define WIFI_EVENT_DISCONNECTED BIT(1)
#define WIFI_EVENT_IP_ACQUIRED  BIT(2)
#define WIFI_EVENT_DNS_CONFIGURED BIT(3)

extern struct k_event wifi_events;

int wifi_connect();

#endif //WIFI_H