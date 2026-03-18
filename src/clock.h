#ifndef CLOCK_H
#define CLOCK_H

#include <stdint.h>
#include <time.h>

#include <zephyr/zbus/zbus.h>

struct clock_event_msg {
	struct tm utc_time;
};

ZBUS_CHAN_DECLARE(clock_event_chan);

int clock_init(void);
int clock_start(uint32_t offset_ms);
void clock_stop(void);

#endif /* CLOCK_H */
