/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ap_pwrseq/ap_pwrseq.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

static int entry_counters[AP_POWER_STATE_COUNT];
static int exit_counters[AP_POWER_STATE_COUNT];

#define ENTRY_MASK	(BIT(AP_POWER_STATE_S4) | \
			 BIT(AP_POWER_STATE_S5) | \
			 BIT(AP_POWER_STATE_G3))

#define EXIT_MASK	(BIT(AP_POWER_STATE_S0) | \
			 BIT(AP_POWER_STATE_S3) | \
			 BIT(AP_POWER_STATE_G3))

static int g3_s0_entry;
static int g3_s0_exit;

static void entry_callback(const struct device *dev, enum ap_pwrseq_state entry,
		enum ap_pwrseq_state exit)
{
	if (entry == AP_POWER_STATE_G3 && exit == AP_POWER_STATE_S0) {
		g3_s0_entry++;
	}

	entry_counters[entry]++;
	switch(entry) {
		case AP_POWER_STATE_S0:
		case AP_POWER_STATE_S4:
		case AP_POWER_STATE_S5:
		case AP_POWER_STATE_G3:
			break;

		default:
			zassert_unreachable("Unexpected entry callback from "
				 "state %s", ap_pwrseq_get_state_str(entry));
	}
}

static void exit_callback(const struct device *dev, enum ap_pwrseq_state entry,
		enum ap_pwrseq_state exit)
{
	if (entry == AP_POWER_STATE_G3 && exit == AP_POWER_STATE_S0) {
		g3_s0_exit++;
	}

	exit_counters[exit]++;
	switch(exit) {
		case AP_POWER_STATE_S0:
		case AP_POWER_STATE_S3:
		case AP_POWER_STATE_G3:
			break;

		default:
			zassert_unreachable("Unexpected exit callback from "
				 "state %s", ap_pwrseq_get_state_str(exit));
	}
}

static void check_counters(int *counters, uint32_t mask, char *type)
{
	for (int i = AP_POWER_STATE_G3; i < AP_POWER_STATE_COUNT; i++) {
		if (mask & BIT(i)) {
			zassert_equal(1, counters[i], "Unexpected %s counter %s",
				type, ap_pwrseq_get_state_str(i));
		} else {
			zassert_equal(0, counters[i], "Unexpected %s counter %s",
				type, ap_pwrseq_get_state_str(i));
		}
	}
}

static void check_entry_counters(void)
{
	check_counters(entry_counters, ENTRY_MASK, "entry");
}

static void check_exit_counters(void)
{
	check_counters(exit_counters, EXIT_MASK, "exit");
}

ZTEST_USER(ap_pwrseq, test_execute)
{
	static struct ap_pwrseq_state_callback entry_state_cb = {
		.cb = entry_callback,
		.states_bit_mask = ENTRY_MASK,
	};
	static struct ap_pwrseq_state_callback exit_state_cb = {
		.cb = exit_callback,
		.states_bit_mask = EXIT_MASK,
	};
	const struct device * dev = ap_pwrseq_get_instance();

	ap_pwrseq_register_state_entry_callback(dev, &entry_state_cb);
	ap_pwrseq_register_state_exit_callback(dev, &exit_state_cb);
	ap_pwrseq_start(dev, AP_POWER_STATE_G3);
	k_msleep(50);
	ap_pwrseq_post_event(dev, AP_PWRSEQ_EVENT_POWER_STARTUP);
	k_msleep(50);
	ap_pwrseq_post_event(dev, AP_PWRSEQ_EVENT_POWER_SIGNAL);
	k_msleep(50);
	ap_pwrseq_post_event(dev, AP_PWRSEQ_EVENT_HOST);
	k_msleep(100);
	ap_pwrseq_post_event(dev, AP_PWRSEQ_EVENT_POWER_TIMEOUT);
	k_msleep(50);

	check_entry_counters();
	check_exit_counters();
	zassert_equal(1, g3_s0_entry, "Test failed");
	zassert_equal(1, g3_s0_exit, "Test failed");
}

ZTEST_SUITE(ap_pwrseq, NULL, NULL, NULL, NULL, NULL);
