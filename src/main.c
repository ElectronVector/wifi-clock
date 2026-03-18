#include "clock.h"
#include "display.h"
#include "network_time.h"
#include "wifi.h"
#include "zephyr/kernel.h"
#include "zephyr/logging/log.h"
#include <zephyr/zbus/zbus.h>

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

ZBUS_SUBSCRIBER_DEFINE(network_wifi_sub, 4);

int main(void)
{
	display_init();
	clock_init();
	display_show_message("Connecting WiFi");

	while (1) {
		const struct zbus_channel *chan;
		struct wifi_event_msg msg;

		if (zbus_sub_wait(&network_wifi_sub, &chan, K_FOREVER) != 0) {
			continue;
		}

		if (zbus_chan_read(chan, &msg, K_MSEC(100)) != 0) {
			continue;
		}

		if (msg.events & WIFI_EVENT_IP_ACQUIRED) {
			LOG_INF("Main thread: IP acquired");
			display_show_message("WiFi connected");
			uint32_t offset_ms = network_time_get_and_set();
			clock_start(offset_ms);
		}

		if (msg.events & WIFI_EVENT_DISCONNECTED) {
			LOG_INF("Main thread: Network disconnected");
			clock_stop();
			display_show_message("WiFi disconnect");
		}
	}

	return 0;
}
