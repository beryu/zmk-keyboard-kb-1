/* SPDX-License-Identifier: MIT */

#include <zephyr/init.h>
#include <zephyr/kernel.h>

#include <dt-bindings/zmk/keys.h>
#include <dt-bindings/zmk/hid_usage_pages.h>
#include <zmk/endpoints.h>
#include <zmk/hid.h>
#include <zmk/keys.h>
#include <zmk/usb.h>

static void send_key(uint32_t keycode) {
    zmk_key_t usage_id = ZMK_HID_USAGE_ID(keycode);

    zmk_hid_keyboard_press(usage_id);
    zmk_endpoints_send_report(HID_USAGE_KEY);
    k_msleep(30);
    zmk_hid_keyboard_release(usage_id);
    zmk_endpoints_send_report(HID_USAGE_KEY);
    k_msleep(30);
}

static void usb_self_test_work_handler(struct k_work *work) {
    static const uint32_t message[] = {
        F, I, R, M, W, A, R, E, SPACE, O, K, ENTER,
    };

    if (!zmk_usb_is_hid_ready()) {
        k_work_reschedule(k_work_delayable_from_work(work), K_MSEC(250));
        return;
    }

    k_msleep(1000);

    for (size_t i = 0; i < ARRAY_SIZE(message); i++) {
        send_key(message[i]);
    }
}

K_WORK_DELAYABLE_DEFINE(usb_self_test_work, usb_self_test_work_handler);

static int usb_self_test_init(void) {
    k_work_schedule(&usb_self_test_work, K_MSEC(250));
    return 0;
}

SYS_INIT(usb_self_test_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
