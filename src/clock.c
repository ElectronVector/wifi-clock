#include "clock.h"

#include <time.h>

#include "display.h"
#include "zephyr/kernel.h"
#include "zephyr/logging/log.h"

LOG_MODULE_REGISTER(clock, CONFIG_LOG_DEFAULT_LEVEL);

ZBUS_CHAN_DEFINE(clock_event_chan, struct clock_event_msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
		 ZBUS_MSG_INIT(.utc_time = {0}));

static struct k_timer one_second_timer;
static struct k_work clock_tick_work;

static void clock_tick_work_handler(struct k_work *work)
{
	struct timespec ts;
	struct tm tm_utc;
	time_t now;
	struct clock_event_msg msg;
	int ret;

	ARG_UNUSED(work);

	ret = clock_gettime(CLOCK_REALTIME, &ts);
	if (ret != 0) {
		LOG_ERR("Failed to get time: %d", ret);
		return;
	}

	now = ts.tv_sec;
	if (gmtime_r(&now, &tm_utc) == NULL) {
		LOG_ERR("Failed to convert time");
		return;
	}

	display_show_time(&tm_utc);

	msg.utc_time = tm_utc;
	ret = zbus_chan_pub(&clock_event_chan, &msg, K_NO_WAIT);
	if (ret != 0) {
		LOG_ERR("Failed to publish clock event: %d", ret);
	}
}

static void timer_handler(struct k_timer *timer)
{
	ARG_UNUSED(timer);
	k_work_submit(&clock_tick_work);
}

int clock_init(void)
{
	k_work_init(&clock_tick_work, clock_tick_work_handler);
	k_timer_init(&one_second_timer, timer_handler, NULL);
	return 0;
}

int clock_start(uint32_t offset_ms)
{
	if (offset_ms == 0) {
		offset_ms = 1;
	}

	k_timer_start(&one_second_timer, K_MSEC(offset_ms), K_SECONDS(1));
	return 0;
}

void clock_stop(void)
{
	k_timer_stop(&one_second_timer);
}
