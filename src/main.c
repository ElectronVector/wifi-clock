#include <stdio.h>
#include <time.h>

#include "network.h"
#include "network_time.h"
#include "zephyr/kernel.h"
#include "zephyr/logging/log.h"

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

static void timer_handler(struct k_timer *dummy)
{
	struct timespec ts;
	int err = clock_gettime(CLOCK_REALTIME, &ts);
	if (!err) {
		struct tm *tm_struct;
		char time_str[64];
		time_t now = ts.tv_sec;

		tm_struct = gmtime(&now);
		if (tm_struct) {
			strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S UTC", tm_struct);
			LOG_INF("Timer tick: %s", time_str);
		} else {
			LOG_ERR("Failed to convert time");
		}
	} else {
		LOG_ERR("Failed to get time: %d", err);
	}
}

K_TIMER_DEFINE(tick_timer, timer_handler, NULL);

int main(void)
{
	printf("Sup?? Hello World!! %s\n", CONFIG_BOARD);

	k_timer_start(&tick_timer, K_SECONDS(10), K_SECONDS(10));

	while (1) {
		uint32_t events = k_event_wait(&network_events, NETWORK_EVENT_CONNECTED | NETWORK_EVENT_DISCONNECTED,
					      false, K_FOREVER);

		if (events & NETWORK_EVENT_CONNECTED) {
			LOG_INF("Main thread: Network connected");
			network_time_fetch_and_set();
			k_event_clear(&network_events, NETWORK_EVENT_CONNECTED);
		}

		if (events & NETWORK_EVENT_DISCONNECTED) {
			LOG_INF("Main thread: Network disconnected");
			k_event_clear(&network_events, NETWORK_EVENT_DISCONNECTED);
		}
	}

	return 0;
}
