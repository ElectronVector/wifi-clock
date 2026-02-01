#ifndef NETWORK_H
#define NETWORK_H

#include <zephyr/kernel.h>

#define NETWORK_EVENT_CONNECTED BIT(0)
#define NETWORK_EVENT_DISCONNECTED BIT(1)

extern struct k_event network_events;

#endif // NETWORK_H
