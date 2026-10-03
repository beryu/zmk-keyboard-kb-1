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
static bool hold_cs;
static unsigned int edge_us = 10;
static unsigned int turnaround_us = 20;

static void reset_bus(void) {
    nrf_gpio_pin_clear(CLK_PIN);
    k_busy_wait(1000);
    nrf_gpio_pin_set(CLK_PIN);
    k_busy_wait(2000);
}

static uint8_t read_register(uint8_t address) {
    uint8_t value = 0;
    nrf_gpio_pin_set(CLK_PIN);
    nrf_gpio_cfg_output(DATA_PIN);
    nrf_gpio_pin_clear(CS_PIN);
    k_busy_wait(20);
    for (int bit = 7; bit >= 0; bit--) {
        nrf_gpio_pin_write(DATA_PIN, (address >> bit) & 1);
        k_busy_wait(edge_us);
        nrf_gpio_pin_clear(CLK_PIN);
        k_busy_wait(edge_us);
        nrf_gpio_pin_set(CLK_PIN);
        k_busy_wait(edge_us);
    }
    nrf_gpio_cfg_input(DATA_PIN, NRF_GPIO_PIN_NOPULL);
    k_busy_wait(turnaround_us);
    for (int bit = 7; bit >= 0; bit--) {
        nrf_gpio_pin_clear(CLK_PIN);
        k_busy_wait(edge_us);
        nrf_gpio_pin_set(CLK_PIN);
        k_busy_wait(edge_us);
        value |= nrf_gpio_pin_read(DATA_PIN) << bit;
        k_busy_wait(edge_us);
    }
    if (!hold_cs) { nrf_gpio_pin_set(CS_PIN); }
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
    LOG_INF("Timing diagnostic v3: per-read CS, delayed data sampling");
    static const unsigned int edges[] = { 1, 10, 50 };
    static const unsigned int turns[] = { 5, 100 };
    hold_cs = false;
    while (true) {
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 2; j++) {
                edge_us = edges[i];
                turnaround_us = turns[j];
                nrf_gpio_pin_set(CS_PIN);
                reset_bus();
                unsigned int valid = 0;
                unsigned int id0 = 0, id1 = 0;
                for (int attempt = 0; attempt < 10; attempt++) {
                    id0 = read_register(0x00);
                    id1 = read_register(0x01);
                    if (id0 == 0x30 && id1 == 0x02) { valid++; }
                    k_sleep(K_MSEC(10));
                }
                LOG_INF("edge_us=%u turnaround_us=%u valid=%u/10 last_id0=0x%02x last_id1=0x%02x",
                        edge_us, turnaround_us, valid, id0, id1);
            }
        }
        k_sleep(K_SECONDS(2));
    }
}
K_THREAD_DEFINE(trackball_diagnostic_thread, 1024, report_sensor,
                NULL, NULL, NULL, 10, 0, 0);
#endif
