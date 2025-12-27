#include <stdio.h>
#include <time.h>

#include "wifi.h"
#include "zephyr/kernel.h"
#include "zephyr/logging/log.h"
#include "zephyr/net/sntp.h"

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

#define NTP_SERVER "pool.ntp.org"
#define NTP_TIMEOUT 3000

static void fetch_and_log_time(void)
{
	struct sntp_time ts;
	int res;

	LOG_INF("Fetching time from NTP server: %s", NTP_SERVER);

	res = sntp_simple(NTP_SERVER, NTP_TIMEOUT, &ts);
	if (res < 0) {
		LOG_ERR("SNTP query failed: %d", res);
		return;
	}

	time_t now = (time_t)ts.seconds;
	struct tm *tm_struct;
	char time_str[64];

	tm_struct = gmtime(&now);
	if (tm_struct) {
		strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S UTC", tm_struct);
		LOG_INF("Current time: %s", time_str);
	} else {
		LOG_ERR("Failed to convert time");
	}
}

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
			fetch_and_log_time();
			k_event_clear(&wifi_events, WIFI_EVENT_IP_ACQUIRED);
		}
	}

	return 0;
}
