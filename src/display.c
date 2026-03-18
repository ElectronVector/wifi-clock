#include "display.h"

#include <string.h>

#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/drivers/auxdisplay.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(display, CONFIG_LOG_DEFAULT_LEVEL);

#define LCD_NODE DT_NODELABEL(lcd_char_display)

static const struct device *lcd;
static const size_t lcd_cols = DT_PROP(LCD_NODE, columns);
static const size_t lcd_rows = DT_PROP(LCD_NODE, rows);

static bool display_ready(void)
{
	return (lcd != NULL) && device_is_ready(lcd);
}

static void display_write_centered_line(uint8_t row, const char *text)
{
	size_t len;
	size_t write_len;
	size_t col;
	char line[lcd_cols];

	if ((text == NULL) || (row >= lcd_rows)) {
		return;
	}

	len = strlen(text);
	write_len = (len > lcd_cols) ? lcd_cols : len;
	col = (lcd_cols - write_len) / 2;

	memset(line, ' ', lcd_cols);
	memcpy(line + col, text, write_len);

	auxdisplay_cursor_position_set(lcd, AUXDISPLAY_POSITION_ABSOLUTE, 0, row);
	auxdisplay_write(lcd, line, lcd_cols);
}

int display_init(void)
{
	lcd = DEVICE_DT_GET(DT_NODELABEL(lcd_char_display));

	while (!device_is_ready(lcd)) {
		LOG_INF("Waiting for LCD to be ready");
		k_sleep(K_SECONDS(1));
	}

	auxdisplay_clear(lcd);
	return 0;
}

void display_show_message(const char *msg)
{
	char line0[17] = {0};
	char line1[17] = {0};
	size_t i = 0;
	size_t line_len = 0;
	uint8_t row = 0;

	if (!display_ready()) {
		return;
	}

	auxdisplay_clear(lcd);
	if (msg == NULL) {
		return;
	}

	while (msg[i] != '\0' && row < lcd_rows) {
		if (msg[i] == '\n') {
			row++;
			line_len = 0;
			i++;
			continue;
		}

		if (line_len < lcd_cols) {
			if (row == 0) {
				line0[line_len] = msg[i];
			} else {
				line1[line_len] = msg[i];
			}
			line_len++;
		}
		i++;
	}

	display_write_centered_line(0, line0);
	display_write_centered_line(1, line1);
}

void display_show_time(const struct tm *time_utc)
{
	char time_str[17];
	char date_str[17];

	if (!display_ready() || (time_utc == NULL)) {
		return;
	}

	strftime(time_str, sizeof(time_str), "%H:%M:%S", time_utc);
	strftime(date_str, sizeof(date_str), "%Y-%m-%d", time_utc);

	display_write_centered_line(0, time_str);
	display_write_centered_line(1, date_str);
}
