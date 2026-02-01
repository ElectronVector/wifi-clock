#include <stdio.h>
#include <time.h>

#include "network.h"
#include "zephyr/kernel.h"
#include "zephyr/logging/log.h"
#include "zephyr/net/sntp.h"

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

	struct timespec tspec;
	tspec.tv_sec = now;  // Unix timestamp from SNTP
	tspec.tv_nsec = 0;
	clock_settime(CLOCK_REALTIME, &tspec);
}

int main(void)
{
	printf("Sup?? Hello World!! %s\n", CONFIG_BOARD);

	k_timer_start(&tick_timer, K_SECONDS(10), K_SECONDS(10));

	while (1) {
		uint32_t events = k_event_wait(&network_events, NETWORK_EVENT_CONNECTED | NETWORK_EVENT_DISCONNECTED,
					      false, K_FOREVER);

		if (events & NETWORK_EVENT_CONNECTED) {
			LOG_INF("Main thread: Network connected");
			fetch_and_log_time();
			k_event_clear(&network_events, NETWORK_EVENT_CONNECTED);
		}

		if (events & NETWORK_EVENT_DISCONNECTED) {
			LOG_INF("Main thread: Network disconnected");
			k_event_clear(&network_events, NETWORK_EVENT_DISCONNECTED);
		}
	}

	return 0;
}
