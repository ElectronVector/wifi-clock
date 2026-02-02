#include "network_time.h"

#include <time.h>

#include "zephyr/kernel.h"
#include "zephyr/logging/log.h"
#include "zephyr/net/sntp.h"

LOG_MODULE_REGISTER(network_time, CONFIG_LOG_DEFAULT_LEVEL);

#define NTP_SERVER "pool.ntp.org"
#define NTP_TIMEOUT 3000
#define NTP_RETRY_COUNT 10

void network_time_fetch_and_set(void)
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
