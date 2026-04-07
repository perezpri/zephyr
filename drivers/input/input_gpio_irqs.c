/*
 * Copyright 2022 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#define DT_DRV_COMPAT gpio_irqs

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio_irqs.h>
#include <zephyr/dt-bindings/input/gpio-irqs.h>

LOG_MODULE_REGISTER(gpio_irqs, LOG_LEVEL_ERR);

struct gpio_irqs_data {
	struct gpio_callback gpio_cb;
	const struct gpio_irqs_callback *irq_cb;
	struct k_work_delayable dworker;
	uint8_t pin_state;
};

struct gpio_irqs_config {
	const struct gpio_dt_spec gpio;
	gpio_flags_t flags;
	uint32_t debounce_ms;
	bool enable_on_init;
	bool fire_on_init;
};

static void gpio_irqs_fire_callback(const struct gpio_irqs_callback *irq_cb)
{
	if (irq_cb && irq_cb->handler) {
		irq_cb->handler(irq_cb->dev, irq_cb->user_data);
	}
}

static void gpio_irqs_work(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct gpio_irqs_data *data =
		CONTAINER_OF(dwork, struct gpio_irqs_data, dworker);
	const struct gpio_irqs_callback *irq_cb = data->irq_cb;
	const struct gpio_irqs_config *cfg = irq_cb->dev->config;

	if (data->pin_state == gpio_pin_get_dt(&cfg->gpio)) {
		gpio_irqs_fire_callback(irq_cb);
	}
}

static void gpio_irqs_cb(const struct device *dev, struct gpio_callback *cb,
				 uint32_t pins)
{
	struct gpio_irqs_data *data =
		CONTAINER_OF(cb, struct gpio_irqs_data, gpio_cb);
	const struct gpio_irqs_callback *irq_cb = data->irq_cb;
	const struct gpio_irqs_config *cfg = irq_cb->dev->config;

	if (cfg->debounce_ms > 0) {
		data->pin_state = gpio_pin_get_dt(&cfg->gpio);
		k_work_reschedule(&data->dworker, K_MSEC(cfg->debounce_ms));
		return;
	}

	gpio_irqs_fire_callback(irq_cb);
}

/**
 * @brief Enable GPIO interrupt for this device instance.
 *
 * Applies the interrupt flags from the devicetree configuration to the
 * underlying GPIO pin, arming it for interrupt detection.
 *
 * @param dev  gpio_irqs device instance.
 *
 * @retval 0        On success.
 * @retval -errno   Negative error code from the GPIO driver on failure.
 */
static int gpio_irqs_impl_enable(const struct device *dev)
{
	const struct gpio_irqs_config *cfg = dev->config;
	int configure_flags = GPIO_INT_ENABLE | GPIO_INT_EDGE;

	if (cfg->flags & GPIO_IRQ_EDGE_RISING) {
		configure_flags |= GPIO_INT_EDGE_RISING;
	}
	if (cfg->flags & GPIO_IRQ_EDGE_FALLING) {
		configure_flags |= GPIO_INT_EDGE_FALLING;
	}

	return gpio_pin_interrupt_configure_dt(&cfg->gpio, configure_flags);
}

/**
 * @brief Disable GPIO interrupt for this device instance.
 *
 * Masks the interrupt on the underlying GPIO pin.  No callbacks will be
 * invoked until @ref gpio_irqs_impl_enable is called again.
 *
 * @param dev  gpio_irqs device instance.
 *
 * @retval 0        On success.
 * @retval -errno   Negative error code from the GPIO driver on failure.
 */
static int gpio_irqs_impl_disable(const struct device *dev)
{
	const struct gpio_irqs_config *cfg = dev->config;

	return gpio_pin_interrupt_configure_dt(&cfg->gpio, GPIO_INT_DISABLE);
}

/**
 * @brief Reconfigure the GPIO interrupt flags at runtime.
 *
 * @param dev    gpio_irqs device instance.
 * @param flags  New GPIO interrupt configuration flags.
 *
 * @retval -ENOSYS  Runtime reconfiguration is not yet implemented.
 */
static int gpio_irqs_impl_configure(const struct device *dev,
					gpio_flags_t flags)
{
	/* TODO: apply flags to the underlying GPIO */
	return -ENOSYS;
}

/**
 * @brief Read the current logical level of the GPIO pin.
 *
 * @param dev  gpio_irqs device instance.
 *
 * @retval 1        Pin is logically high.
 * @retval 0        Pin is logically low.
 * @retval -errno   Negative error code from the GPIO driver on failure.
 */
static int gpio_irqs_impl_pin_get(const struct device *dev)
{
	const struct gpio_irqs_config *cfg = dev->config;

	return gpio_pin_get_dt(&cfg->gpio);
}

static const struct gpio_irqs_driver_api gpio_irqs_api = {
	.enable = gpio_irqs_impl_enable,
	.disable = gpio_irqs_impl_disable,
	.configure = gpio_irqs_impl_configure,
	.pin_get = gpio_irqs_impl_pin_get,
};

/**
 * @brief Initialise a gpio_irqs device instance.
 *
 * Configures the GPIO pin as input, registers the internal GPIO callback,
 * optionally sets up debounce work, binds the first matching
 * @ref gpio_irqs_callback from the iterable section, and optionally fires
 * the callback and/or enables the interrupt immediately.
 *
 * @param dev  gpio_irqs device instance.
 *
 * @retval 0        On success.
 * @retval -ENODEV  If the underlying GPIO device is not ready.
 * @retval -errno   Negative error code on other failures.
 */
static int gpio_irqs_init(const struct device *dev)
{
	const struct gpio_irqs_config *cfg = dev->config;
	struct gpio_irqs_data *data = dev->data;
	int rv;

	if (!gpio_is_ready_dt(&cfg->gpio)) {
		LOG_ERR("%s: GPIO device not ready", dev->name);
		return -ENODEV;
	}

	rv = gpio_pin_configure_dt(&cfg->gpio, GPIO_INPUT);
	if (rv < 0) {
		LOG_ERR("%s: Unable to configure GPIO", dev->name);
		return rv;
	}

	gpio_init_callback(&data->gpio_cb, gpio_irqs_cb,
			   BIT(cfg->gpio.pin));

	rv = gpio_add_callback(cfg->gpio.port, &data->gpio_cb);
	if (rv < 0) {
		LOG_ERR("%s: Unable to add callback", dev->name);
		return rv;
	}

	if (cfg->debounce_ms) {
		k_work_init_delayable(&data->dworker, gpio_irqs_work);
	}

	STRUCT_SECTION_FOREACH(gpio_irqs_callback, cb)
	{
		if (cb->dev == dev) {
			data->irq_cb = cb;
		}
	}

	if (cfg->fire_on_init) {
		gpio_irqs_fire_callback(data->irq_cb);
	}

	if (cfg->enable_on_init) {
		gpio_irqs_impl_enable(dev);
	}

	return 0;
}

/* Create one device instance per child node of gpio-irqs */
#define GPIO_IRQS_CHILD_DEFINE(node_id)                                    \
	static struct gpio_irqs_data gpio_irqs_data_##node_id;         \
	static const struct gpio_irqs_config gpio_irqs_cfg_##node_id = \
		{                                                              \
			.gpio = GPIO_DT_SPEC_GET(node_id, gpios),              \
			.flags = DT_PROP_OR(node_id, flags, 0),                \
			.debounce_ms = DT_PROP(node_id, debounce_ms),          \
			.enable_on_init = DT_PROP(node_id, enable_on_init),    \
			.fire_on_init = DT_PROP(node_id, fire_on_init),        \
	};                                                                     \
	DEVICE_DT_DEFINE(node_id, gpio_irqs_init, NULL,                    \
			 &gpio_irqs_data_##node_id,                        \
			 &gpio_irqs_cfg_##node_id, POST_KERNEL,            \
			 CONFIG_INPUT_GPIO_IRQS_INIT_PRIORITY,          \
			 &gpio_irqs_api);

#define GPIO_IRQS_NODE_DEF(inst)                                           \
	DT_FOREACH_CHILD_STATUS_OKAY(DT_DRV_INST(inst), GPIO_IRQS_CHILD_DEFINE)

DT_INST_FOREACH_STATUS_OKAY(GPIO_IRQS_NODE_DEF)
