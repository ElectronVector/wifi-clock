#ifndef DISPLAY_H
#define DISPLAY_H

#include <time.h>

int display_init(void);
void display_show_message(const char *msg);
void display_show_time(const struct tm *time_utc);

#endif /* DISPLAY_H */
