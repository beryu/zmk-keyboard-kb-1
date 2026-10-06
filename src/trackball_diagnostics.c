#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <hal/nrf_gpio.h>

LOG_MODULE_REGISTER(trackball_diagnostics, LOG_LEVEL_INF);

#ifdef CONFIG_SHIELD_KB_1
#define CS_PIN NRF_GPIO_PIN_MAP(1, 12)
#define CLK_PIN NRF_GPIO_PIN_MAP(1, 13)
#define MOTION_PIN NRF_GPIO_PIN_MAP(1, 14)
#define SDIO_PIN NRF_GPIO_PIN_MAP(1, 15)

/* PAW3222's three-wire SPI: write the address, release SDIO, then read.
 * These conservative timings are the known-good diagnostic timings. */
static uint8_t read_register(uint8_t address) {
    uint8_t value = 0;

    nrf_gpio_pin_set(CLK_PIN);
    nrf_gpio_cfg_output(SDIO_PIN);
    nrf_gpio_pin_clear(CS_PIN);
    k_busy_wait(20);

    for (int bit = 7; bit >= 0; bit--) {
        nrf_gpio_pin_write(SDIO_PIN, (address >> bit) & 1);
        k_busy_wait(10);
        nrf_gpio_pin_clear(CLK_PIN);
        k_busy_wait(10);
        nrf_gpio_pin_set(CLK_PIN);
        k_busy_wait(10);
    }

    nrf_gpio_cfg_input(SDIO_PIN, NRF_GPIO_PIN_NOPULL);
    k_busy_wait(20);

    for (int bit = 7; bit >= 0; bit--) {
        nrf_gpio_pin_clear(CLK_PIN);
        k_busy_wait(10);
        nrf_gpio_pin_set(CLK_PIN);
        value |= nrf_gpio_pin_read(SDIO_PIN) << bit;
        k_busy_wait(10);
    }

    nrf_gpio_pin_set(CS_PIN);
    k_busy_wait(20);
    return value;
}

static void report_sensor(void *unused_a, void *unused_b, void *unused_c) {
    ARG_UNUSED(unused_a);
    ARG_UNUSED(unused_b);
    ARG_UNUSED(unused_c);

    k_sleep(K_SECONDS(10));
    nrf_gpio_pin_set(CS_PIN);
    nrf_gpio_cfg_output(CS_PIN);
    nrf_gpio_pin_set(CLK_PIN);
    nrf_gpio_cfg_output(CLK_PIN);
    nrf_gpio_cfg_input(SDIO_PIN, NRF_GPIO_PIN_NOPULL);
    nrf_gpio_cfg_input(MOTION_PIN, NRF_GPIO_PIN_PULLUP);

    LOG_INF("PAW3222 GPIO diagnostic; normal driver disabled for this build");
    LOG_INF("CS=P1.12 CLK=P1.13 SDIO=P1.15 MOTION=P1.14");

    while (true) {
        uint8_t id0 = read_register(0x00);
        uint8_t id1 = read_register(0x01);
        unsigned int motion = nrf_gpio_pin_read(MOTION_PIN);

        LOG_INF("id0=0x%02x expected=0x30 id1=0x%02x motion_raw=%u",
                id0, id1, motion);
        k_sleep(K_SECONDS(2));
    }
}

K_THREAD_DEFINE(trackball_diagnostic_thread, 1024, report_sensor,
                NULL, NULL, NULL, 10, 0, 0);
#endif
