#include <stdio.h>
#include <time.h>

#include "wifi.h"
#include "zephyr/kernel.h"
#include "zephyr/logging/log.h"
#include "zephyr/net/sntp.h"

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

#define NTP_SERVER "pool.ntp.org"
#define NTP_TIMEOUT 3000
#define NTP_RETRY_COUNT 10

static void fetch_and_log_time(void)
{
	struct sntp_time ts;
	int res;
	int retries = 0;

	LOG_INF("Fetching time from NTP server: %s", NTP_SERVER);

	do {
		res = sntp_simple(NTP_SERVER, NTP_TIMEOUT, &ts);
		if (res >= 0) {
			break;
		}
		retries++;
		LOG_WRN("SNTP query failed (attempt %d/%d): %d", retries, NTP_RETRY_COUNT, res);
		if (retries < NTP_RETRY_COUNT) {
			k_msleep(1000); // Wait a bit before retrying
		}
	} while (retries < NTP_RETRY_COUNT);

	if (res < 0) {
		LOG_ERR("SNTP query failed after %d retries: %d", NTP_RETRY_COUNT, res);
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
		uint32_t events = k_event_wait(&wifi_events, WIFI_EVENT_CONNECTED | WIFI_EVENT_DISCONNECTED |
							      WIFI_EVENT_IP_ACQUIRED | WIFI_EVENT_DNS_CONFIGURED,
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

		if (events & WIFI_EVENT_DNS_CONFIGURED) {
			LOG_INF("Main thread: DNS configured");
			k_event_clear(&wifi_events, WIFI_EVENT_DNS_CONFIGURED);
		}
	}

	return 0;
}
