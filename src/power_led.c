/* kb-1 has no battery: application power is supplied through USB-C. */
#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>

static const struct gpio_dt_spec power_led = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);

static int kb_1_power_led_init(void) {
    if (!gpio_is_ready_dt(&power_led)) {
        return -ENODEV;
    }

    /* led1 is the XIAO onboard blue LED. Logical active respects the board's GPIO_ACTIVE_LOW LED wiring. */
    return gpio_pin_configure_dt(&power_led, GPIO_OUTPUT_ACTIVE);
}

SYS_INIT(kb_1_power_led_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
