/* SPDX-License-Identifier: MIT */

#include <zephyr/init.h>
#include <zephyr/kernel.h>

#include <dt-bindings/zmk/keys.h>
#include <zmk/events/keycode_state_changed.h>

static void send_key(uint32_t keycode) {
    raise_zmk_keycode_state_changed_from_encoded(keycode, true, k_uptime_get());
    k_msleep(30);
    raise_zmk_keycode_state_changed_from_encoded(keycode, false, k_uptime_get());
    k_msleep(30);
}

static void usb_self_test_work_handler(struct k_work *work) {
    static const uint32_t message[] = {
        F, I, R, M, W, A, R, E, SPACE, O, K, ENTER,
    };

    ARG_UNUSED(work);

    for (size_t i = 0; i < ARRAY_SIZE(message); i++) {
        send_key(message[i]);
    }
}

K_WORK_DELAYABLE_DEFINE(usb_self_test_work, usb_self_test_work_handler);

static int usb_self_test_init(void) {
    k_work_schedule(&usb_self_test_work, K_SECONDS(5));
    return 0;
}

SYS_INIT(usb_self_test_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
