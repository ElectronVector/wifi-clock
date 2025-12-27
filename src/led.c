#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(led, CONFIG_LOG_DEFAULT_LEVEL);

#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

static void led_thread_fn(void *arg1, void *arg2, void *arg3)
{
    int ret;

    if (!gpio_is_ready_dt(&led)) {
        LOG_ERR("Error: LED device %s is not ready", led.port->name);
        return;
    }

    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        LOG_ERR("Error: Cannot configure LED GPIO");
        return;
    }

    while (1) {
        ret = gpio_pin_toggle_dt(&led);
        if (ret < 0) {
            LOG_ERR("Error: Cannot toggle LED");
            return;
        }

        k_sleep(K_SECONDS(1));
    }
}

K_THREAD_DEFINE(led_thread, 512, led_thread_fn, NULL, NULL, NULL, 7, 0, 0);
