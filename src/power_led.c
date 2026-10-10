/* kb-1 has no battery: application power is supplied through USB-C. */
#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>

#include <zmk/event_manager.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/keymap.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

enum led_channel { RED, GREEN, BLUE };

/* Use the GPIO wiring, not the swapped blue/green labels in the v0.3 board definition.
 * XIAO nRF52840 Plus: led0/P0.26 = red, led1/P0.30 = green, led2/P0.06 = blue.
 * The GPIO helpers respect GPIO_ACTIVE_LOW: logical 1 turns a channel on.
 */
static const struct gpio_dt_spec layer_leds[] = {
    [RED] = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios),
    [GREEN] = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios),
    [BLUE] = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios),
};

static bool leds_ready;

static int update_layer_led(uint8_t layer) {
    uint8_t channels = BIT(GREEN);

    switch (layer) {
    case 1:
        channels = BIT(RED) | BIT(GREEN);
        break;
    case 2:
        channels = BIT(RED) | BIT(BLUE);
        break;
    default:
        break;
    }

    for (size_t i = 0; i < ARRAY_SIZE(layer_leds); i++) {
        int err = gpio_pin_set_dt(&layer_leds[i], (channels & BIT(i)) != 0);
        if (err) {
            return err;
        }
    }

    return 0;
}

static int kb_1_power_led_init(void) {
    for (size_t i = 0; i < ARRAY_SIZE(layer_leds); i++) {
        if (!gpio_is_ready_dt(&layer_leds[i])) {
            return -ENODEV;
        }

        int err = gpio_pin_configure_dt(&layer_leds[i], GPIO_OUTPUT_INACTIVE);
        if (err) {
            return err;
        }
    }

    /* Layer 0 is active at boot, before any layer key is pressed. */
    int err = update_layer_led(0);
    leds_ready = (err == 0);
    return err;
}

SYS_INIT(kb_1_power_led_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

static int layer_led_listener(const zmk_event_t *eh) {
    if (leds_ready && as_zmk_layer_state_changed(eh)) {
        int err = update_layer_led(zmk_keymap_highest_layer_active());
        if (err) {
            LOG_ERR("Failed to update layer LED (%d)", err);
        }
    }

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(kb_1_layer_led, layer_led_listener);
ZMK_SUBSCRIPTION(kb_1_layer_led, zmk_layer_state_changed);
