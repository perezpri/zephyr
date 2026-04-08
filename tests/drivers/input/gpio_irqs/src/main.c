/*
 * Copyright 2024 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/drivers/gpio_irqs.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define DEBOUNCE_MS DT_PROP(DT_NODELABEL(debounce_irq), debounce_ms)

/* Device references for each child node */
static const struct device *const basic_dev =
	DEVICE_DT_GET(DT_NODELABEL(basic_irq));
static const struct device *const debounce_dev =
	DEVICE_DT_GET(DT_NODELABEL(debounce_irq));
static const struct device *const fire_init_dev =
	DEVICE_DT_GET(DT_NODELABEL(fire_init_irq));
static const struct device *const falling_dev =
	DEVICE_DT_GET(DT_NODELABEL(falling_irq));

/* GPIO specs used to drive pin state via the emulator */
static const struct gpio_dt_spec basic_gpio =
	GPIO_DT_SPEC_GET(DT_NODELABEL(basic_irq), gpios);
static const struct gpio_dt_spec debounce_gpio =
	GPIO_DT_SPEC_GET(DT_NODELABEL(debounce_irq), gpios);
static const struct gpio_dt_spec falling_gpio =
	GPIO_DT_SPEC_GET(DT_NODELABEL(falling_irq), gpios);

/* Callback hit counters */
static int basic_cb_count;
static int debounce_cb_count;
static int fire_init_cb_count;
static int falling_cb_count;

static void basic_handler(const struct device *dev, void *user_data)
{
	basic_cb_count++;
}
GPIO_IRQS_CALLBACK_DEFINE(DEVICE_DT_GET(DT_NODELABEL(basic_irq)),
			  basic_handler, NULL);

static void debounce_handler(const struct device *dev, void *user_data)
{
	debounce_cb_count++;
}
GPIO_IRQS_CALLBACK_DEFINE(DEVICE_DT_GET(DT_NODELABEL(debounce_irq)),
			  debounce_handler, NULL);

static void fire_init_handler(const struct device *dev, void *user_data)
{
	fire_init_cb_count++;
}
GPIO_IRQS_CALLBACK_DEFINE(DEVICE_DT_GET(DT_NODELABEL(fire_init_irq)),
			  fire_init_handler, NULL);

static void falling_handler(const struct device *dev, void *user_data)
{
	falling_cb_count++;
}
GPIO_IRQS_CALLBACK_DEFINE(DEVICE_DT_GET(DT_NODELABEL(falling_irq)),
			  falling_handler, NULL);

/**
 * Reset counters and bring pins to a known LOW state before each test.
 * basic_irq interrupt is disabled so pin transitions don't bleed across tests.
 */
static void gpio_irqs_before(void *fixture)
{
	ARG_UNUSED(fixture);

	basic_cb_count = 0;
	debounce_cb_count = 0;

	gpio_irqs_disable(basic_dev);
	gpio_emul_input_set(basic_gpio.port, basic_gpio.pin, 0);
	gpio_emul_input_set(debounce_gpio.port, debounce_gpio.pin, 0);
	gpio_irqs_disable(falling_dev);
	gpio_emul_input_set(falling_gpio.port, falling_gpio.pin, 0);
	falling_cb_count = 0;
}

ZTEST_SUITE(gpio_irqs, NULL, NULL, gpio_irqs_before, NULL, NULL);

/**
 * @brief Verify all device instances are ready after boot.
 */
ZTEST(gpio_irqs, test_devices_ready)
{
	zassert_true(device_is_ready(basic_dev));
	zassert_true(device_is_ready(debounce_dev));
	zassert_true(device_is_ready(fire_init_dev));
}

/**
 * @brief Verify that enabling the interrupt causes the callback to fire on
 * both rising and falling edges.
 */
ZTEST(gpio_irqs, test_enable_triggers_interrupt)
{
	zassert_ok(gpio_irqs_enable(basic_dev));

	zassert_ok(gpio_emul_input_set(basic_gpio.port, basic_gpio.pin, 1));
	k_sleep(K_MSEC(1));
	zassert_equal(basic_cb_count, 1);

	zassert_ok(gpio_emul_input_set(basic_gpio.port, basic_gpio.pin, 0));
	k_sleep(K_MSEC(1));
	zassert_equal(basic_cb_count, 2);
}

/**
 * @brief Verify that disabling the interrupt stops callbacks, and that
 * re-enabling resumes them.
 */
ZTEST(gpio_irqs, test_disable_stops_interrupt)
{
	zassert_ok(gpio_irqs_enable(basic_dev));

	zassert_ok(gpio_emul_input_set(basic_gpio.port, basic_gpio.pin, 1));
	k_sleep(K_MSEC(1));
	zassert_equal(basic_cb_count, 1);

	zassert_ok(gpio_irqs_disable(basic_dev));
	zassert_ok(gpio_emul_input_set(basic_gpio.port, basic_gpio.pin, 0));
	k_sleep(K_MSEC(1));
	zassert_equal(basic_cb_count, 1, "Callback fired after disable");

	/* Re-enable and verify interrupts resume */
	zassert_ok(gpio_irqs_enable(basic_dev));
	zassert_ok(gpio_emul_input_set(basic_gpio.port, basic_gpio.pin, 1));
	k_sleep(K_MSEC(1));
	zassert_equal(basic_cb_count, 2);
}

/**
 * @brief Verify that gpio_irqs_configure returns -ENOSYS since runtime
 * flag reconfiguration is not yet implemented.
 */
ZTEST(gpio_irqs, test_configure_returns_enosys)
{
	zassert_equal(gpio_irqs_configure(basic_dev, GPIO_INT_EDGE_RISING),
		      -ENOSYS);
}

/**
 * @brief Verify that gpio_irqs_pin_get reflects the current physical pin level.
 */
ZTEST(gpio_irqs, test_pin_get)
{
	zassert_ok(gpio_emul_input_set(basic_gpio.port, basic_gpio.pin, 1));
	zassert_equal(gpio_irqs_pin_get(basic_dev), 1);

	zassert_ok(gpio_emul_input_set(basic_gpio.port, basic_gpio.pin, 0));
	zassert_equal(gpio_irqs_pin_get(basic_dev), 0);
}

/**
 * @brief Verify that a debounced interrupt does not fire before the debounce
 * interval has elapsed, but does fire once the signal is stable.
 */
ZTEST(gpio_irqs, test_debounce_fires_after_interval)
{
	/* Trigger rising edge - interrupt fires, debounce work is scheduled */
	zassert_ok(gpio_emul_input_set(debounce_gpio.port,
				       debounce_gpio.pin, 1));

	/* Callback must not fire before debounce interval */
	k_sleep(K_MSEC(DEBOUNCE_MS / 2));
	zassert_equal(debounce_cb_count, 0,
		      "Callback fired before debounce interval");

	/* Callback must fire once the signal has been stable long enough */
	k_sleep(K_MSEC(DEBOUNCE_MS));
	zassert_equal(debounce_cb_count, 1,
		      "Callback did not fire after debounce interval");
}

/**
 * @brief Verify that a signal that bounces within the debounce window does
 * not trigger the callback (pin state at debounce expiry differs from the
 * state captured at interrupt time).
 */
ZTEST(gpio_irqs, test_debounce_cancelled_on_bounce)
{
	/* Rising edge: driver captures pin_state=HIGH, schedules debounce */
	zassert_ok(gpio_emul_input_set(debounce_gpio.port,
				       debounce_gpio.pin, 1));
	k_sleep(K_MSEC(DEBOUNCE_MS / 4));

	/* Pin bounces LOW before debounce expires */
	zassert_ok(gpio_emul_input_set(debounce_gpio.port,
				       debounce_gpio.pin, 0));

	/* At debounce expiry pin=LOW but pin_state=HIGH -> no match, no callback */
	k_sleep(K_MSEC(DEBOUNCE_MS * 2));
	zassert_equal(debounce_cb_count, 0,
		      "Callback fired despite signal bounce");
}

/**
 * @brief Verify that a falling-edge IRQ fires only on the falling edge and
 * not on the rising edge.
 */
ZTEST(gpio_irqs, test_falling_edge_only)
{
	zassert_ok(gpio_irqs_enable(falling_dev));

	/* Rising edge - must NOT trigger callback */
	zassert_ok(gpio_emul_input_set(falling_gpio.port, falling_gpio.pin, 1));
	k_sleep(K_MSEC(1));
	zassert_equal(falling_cb_count, 0,
		      "Callback fired on rising edge with EDGE_FALLING config");

	/* Falling edge - must trigger callback */
	zassert_ok(gpio_emul_input_set(falling_gpio.port, falling_gpio.pin, 0));
	k_sleep(K_MSEC(1));
	zassert_equal(falling_cb_count, 1,
		      "Callback did not fire on falling edge");
	zassert_equal(fire_init_cb_count, 1,
		      "fire-on-init callback did not fire at boot");
}
