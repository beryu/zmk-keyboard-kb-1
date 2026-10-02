#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(trackball_diagnostics, LOG_LEVEL_INF);
#define SENSOR_NODE DT_NODELABEL(pointing_device)

static void report_sensor(void *a, void *b, void *c) {
    const struct device *sensor = DEVICE_DT_GET(SENSOR_NODE);
    const struct gpio_dt_spec motion = GPIO_DT_SPEC_GET(SENSOR_NODE, irq_gpios);
    ARG_UNUSED(a);
    ARG_UNUSED(b);
    ARG_UNUSED(c);
    k_sleep(K_SECONDS(10));
    LOG_INF("PAW3222 diagnostic firmware; USB logs, Bluetooth input");
    LOG_INF("Pins: CS=P1.12 SCLK=P1.13 SDIO=P1.15 MOTION=P1.14");
    while (true) {
        int active = gpio_is_ready_dt(&motion) ? gpio_pin_get_dt(&motion) : -1;
        LOG_INF("sensor_ready=%d motion_active=%d (1=motion asserted)",
                device_is_ready(sensor), active);
        k_sleep(K_SECONDS(5));
    }
}

K_THREAD_DEFINE(trackball_diagnostic_thread, 1024, report_sensor,
                NULL, NULL, NULL, 10, 0, 0);
