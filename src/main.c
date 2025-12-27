#include <stdio.h>

#include "wifi.h"
#include "zephyr/kernel.h"
#include "zephyr/drivers/gpio.h"
#include "zephyr/logging/log.h"
#include "zephyr/sys/printk.h"

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

int main(void)
{
	int ret = 0;

	printf("Sup?? Hello World!! %s\n", CONFIG_BOARD);

	bool led_state = true;

	/* Check if LED device is ready */
	if (!gpio_is_ready_dt(&led)) {
		printk("Error: LED device %s is not ready\n", led.port->name);
		return -1;
	}

	/* Configure LED pin as output */
	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		printk("Error: Cannot configure LED GPIO\n");
		return -1;
	}

	while (1) {
		/* Toggle LED */
		ret = gpio_pin_toggle_dt(&led);
		if (ret < 0) {
			printk("Error: Cannot toggle LED\n");
			return -1;
		}

		led_state = !led_state;

		k_sleep(K_SECONDS(1));
	}

	return 0;
}
