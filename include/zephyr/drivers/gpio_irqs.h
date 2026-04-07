/*
 * Copyright 2022 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_GPIO_IRQS_H_
#define ZEPHYR_INCLUDE_DRIVERS_GPIO_IRQS_H_

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/slist.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Callback handler type for GPIO IRQ events.
 *
 * @param dev       Pointer to the gpio_irqs device that triggered the event.
 * @param data      User-supplied data pointer registered with the callback.
 */
typedef void (*gpio_irqs_callback_handler)(const struct device *dev, void *data);

/**
 * @brief GPIO IRQ callback descriptor.
 *
 * Statically allocated and linked into the iterable section so the driver can
 * find all registered callbacks at initialisation time.
 */
struct gpio_irqs_callback {
	/** gpio_irqs device this callback is bound to. */
	const struct device *dev;
	/** Function invoked when the GPIO IRQ fires. */
	gpio_irqs_callback_handler handler;
	/** Cached pin state used for debounce validation. */
	uint8_t pin_state;
	/** Opaque pointer forwarded to @p handler. */
	void *user_data;
};

/**
 * @brief Define a named GPIO IRQ callback entry.
 *
 * Places a @ref gpio_irqs_callback into the iterable section so that the
 * driver can discover it automatically during initialisation.
 *
 * @param _dev        gpio_irqs device the callback is associated with.
 * @param _handler    Function of type @ref gpio_irqs_callback_handler to call.
 * @param _user_data  Opaque pointer forwarded to @p _handler.
 * @param name        Unique C identifier suffix for the generated symbol.
 */
#define GPIO_IRQS_CALLBACK_DEFINE_NAMED(_dev, _handler, _user_data, name) \
	static const STRUCT_SECTION_ITERABLE(gpio_irqs_callback,              \
					     _gpio_irqs_callback_##name) = { \
		.dev = _dev,                                                \
		.handler = _handler,                                        \
		.user_data = _user_data,                                    \
	}

/**
 * @brief Define a GPIO IRQ callback entry, using the handler name as suffix.
 *
 * Convenience wrapper around @ref GPIO_IRQS_CALLBACK_DEFINE_NAMED that uses
 * @p _callback as both the handler and the symbol suffix.
 *
 * @param _dev        gpio_irqs device the callback is associated with.
 * @param _callback   Function of type @ref gpio_irqs_callback_handler to call.
 * @param _user_data  Opaque pointer forwarded to @p _callback.
 */
#define GPIO_IRQS_CALLBACK_DEFINE(_dev, _callback, _user_data)             \
	GPIO_IRQS_CALLBACK_DEFINE_NAMED(_dev, _callback, _user_data, _callback)

typedef int (*gpio_irqs_enable_fn)(const struct device *dev);
typedef int (*gpio_irqs_disable_fn)(const struct device *dev);
typedef int (*gpio_irqs_pin_get_fn)(const struct device *dev);
typedef int (*gpio_irqs_configure_fn)(const struct device *dev, gpio_flags_t flags);

__subsystem struct gpio_irqs_driver_api {
	gpio_irqs_enable_fn enable;
	gpio_irqs_disable_fn disable;
	gpio_irqs_configure_fn configure;
	gpio_irqs_pin_get_fn pin_get;
};

/**
 * @brief Enable interrupt detection on the GPIO IRQ device.
 *
 * Configures the underlying GPIO pin to generate interrupts according to the
 * flags set in the devicetree node.
 *
 * @param dev  gpio_irqs device instance.
 *
 * @retval 0        On success.
 * @retval -errno   Negative error code from the GPIO driver on failure.
 */
static inline int gpio_irqs_enable(const struct device *dev)
{
	const struct gpio_irqs_driver_api *api =
		(const struct gpio_irqs_driver_api *)dev->api;

	return api->enable(dev);
}

/**
 * @brief Disable interrupt detection on the GPIO IRQ device.
 *
 * Masks the interrupt on the underlying GPIO pin so no further callbacks are
 * fired until @ref gpio_irqs_enable is called again.
 *
 * @param dev  gpio_irqs device instance.
 *
 * @retval 0        On success.
 * @retval -errno   Negative error code from the GPIO driver on failure.
 */
static inline int gpio_irqs_disable(const struct device *dev)
{
	const struct gpio_irqs_driver_api *api =
		(const struct gpio_irqs_driver_api *)dev->api;

	return api->disable(dev);
}

/**
 * @brief Reconfigure the GPIO interrupt flags at runtime.
 *
 * @param dev    gpio_irqs device instance.
 * @param flags  GPIO interrupt configuration flags (see @ref gpio_flags_t).
 *
 * @retval 0        On success.
 * @retval -ENOSYS  If runtime reconfiguration is not supported.
 * @retval -errno   Negative error code on other failures.
 */
static inline int gpio_irqs_configure(const struct device *dev,
				     gpio_flags_t flags)
{
	const struct gpio_irqs_driver_api *api =
		(const struct gpio_irqs_driver_api *)dev->api;

	return api->configure(dev, flags);
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
static inline int gpio_irqs_pin_get(const struct device *dev)
{
	const struct gpio_irqs_driver_api *api =
		(const struct gpio_irqs_driver_api *)dev->api;

	return api->pin_get(dev);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_GPIO_IRQS_H_ */
