#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <hal/nrf_gpio.h>

LOG_MODULE_REGISTER(trackball_diagnostics, LOG_LEVEL_INF);
#ifdef CONFIG_SHIELD_KB_1
#define CS_PIN NRF_GPIO_PIN_MAP(1, 12)
#define CLK_PIN NRF_GPIO_PIN_MAP(1, 13)
#define MOT_PIN NRF_GPIO_PIN_MAP(1, 14)
#define DATA_PIN NRF_GPIO_PIN_MAP(1, 15)

/* Manufacturer sample: clock idle high, address output followed by SDIO input.
 * Deliberately slow clocks provide margin for manual assembly and the FFC. */
static uint8_t read_register(uint8_t address) {
    uint8_t value = 0;
    nrf_gpio_pin_set(CLK_PIN);
    nrf_gpio_cfg_output(DATA_PIN);
    nrf_gpio_pin_clear(CS_PIN);
    k_busy_wait(20);
    for (int bit = 7; bit >= 0; bit--) {
        nrf_gpio_pin_write(DATA_PIN, (address >> bit) & 1);
        k_busy_wait(10);
        nrf_gpio_pin_clear(CLK_PIN);
        k_busy_wait(10);
        nrf_gpio_pin_set(CLK_PIN);
        k_busy_wait(10);
    }
    nrf_gpio_cfg_input(DATA_PIN, NRF_GPIO_PIN_NOPULL);
    k_busy_wait(20);
    for (int bit = 7; bit >= 0; bit--) {
        nrf_gpio_pin_clear(CLK_PIN);
        k_busy_wait(10);
        nrf_gpio_pin_set(CLK_PIN);
        value |= nrf_gpio_pin_read(DATA_PIN) << bit;
        k_busy_wait(10);
    }
    nrf_gpio_pin_set(CS_PIN);
    k_busy_wait(20);
    return value;
}

static void report_sensor(void *a, void *b, void *c) {
    ARG_UNUSED(a); ARG_UNUSED(b); ARG_UNUSED(c);
    k_sleep(K_SECONDS(10));
    nrf_gpio_pin_set(CS_PIN);
    nrf_gpio_cfg_output(CS_PIN);
    nrf_gpio_pin_set(CLK_PIN);
    nrf_gpio_cfg_output(CLK_PIN);
    nrf_gpio_cfg_input(DATA_PIN, NRF_GPIO_PIN_NOPULL);
    nrf_gpio_cfg_input(MOT_PIN, NRF_GPIO_PIN_PULLUP);
    LOG_INF("GPIO-ID diagnostic: hardware SPI and sensor driver disabled");
    LOG_INF("Mode 2, SDIO switches to input; CS=P1.12 CLK=P1.13 DATA=P1.15");
    while (true) {
        unsigned int id0 = read_register(0x00);
        unsigned int id1 = read_register(0x01);
        LOG_INF("gpio_id0=0x%02x expected=0x30 gpio_id1=0x%02x motion_raw=%u",
                id0, id1, nrf_gpio_pin_read(MOT_PIN));
        k_sleep(K_SECONDS(2));
    }
}
K_THREAD_DEFINE(trackball_diagnostic_thread, 1024, report_sensor,
                NULL, NULL, NULL, 10, 0, 0);
#endif
