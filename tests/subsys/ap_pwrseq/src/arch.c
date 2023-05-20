/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ap_pwrseq/ap_pwrseq_sm.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(ap_pwrseq, LOG_LEVEL_INF);

static int arch_g3_entry(void *data)
{
	LOG_INF("\t%s", __func__);
	return 0;
}

static int arch_g3_run(void *data)
{
	LOG_INF("\t%s", __func__);

	if (ap_pwrseq_sm_is_event_set(data,
		AP_PWRSEQ_EVENT_POWER_STARTUP)) {
		LOG_INF("\t\tTransition");
		return ap_pwrseq_sm_set_state(data, AP_POWER_STATE_S5);
	}

	return 0;
}


static int arch_g3_exit(void *data)
{
	LOG_INF("\t%s", __func__);
	return 0;
}

AP_POWER_ARCH_STATE_DEFINE(AP_POWER_STATE_G3, arch_g3_entry,
			arch_g3_run, arch_g3_exit);


static int arch_s5_entry(void *data)
{
	LOG_INF("\t%s", __func__);
	return 0;
}

static int arch_s5_run(void *data)
{
	LOG_INF("\t%s", __func__);
	return 0;
}


static int arch_s5_exit(void *data)
{
	LOG_INF("\t%s", __func__);
	return 0;
}

AP_POWER_ARCH_STATE_DEFINE(AP_POWER_STATE_S5, arch_s5_entry,
			arch_s5_run, arch_s5_exit);

static int arch_s4_entry(void *data)
{
	LOG_INF("\t%s", __func__);
	return 0;
}

static int arch_s4_run(void *data)
{
	LOG_INF("\t%s", __func__);
	if (ap_pwrseq_sm_is_event_set(data,
		AP_PWRSEQ_EVENT_HOST)) {
		LOG_INF("\t\tTransition");
		return ap_pwrseq_sm_set_state(data, AP_POWER_STATE_S3);
	}
	return 0;
}


static int arch_s4_exit(void *data)
{
	LOG_INF("\t%s", __func__);
	return 0;
}

AP_POWER_ARCH_STATE_DEFINE(AP_POWER_STATE_S4, arch_s4_entry,
			arch_s4_run, arch_s4_exit);

static int arch_s0_entry(void *data)
{
	LOG_INF("\t%s", __func__);
	return 0;
}

static int arch_s0_run(void *data)
{
	LOG_INF("\t%s", __func__);
	if (ap_pwrseq_sm_is_event_set(data,
		AP_PWRSEQ_EVENT_POWER_TIMEOUT)) {
		LOG_INF("\t\tTransition");
		return ap_pwrseq_sm_set_state(data, AP_POWER_STATE_G3);
	}
	return 0;
}


static int arch_s0_exit(void *data)
{
	LOG_INF("\t%s", __func__);
	return 0;
}

AP_POWER_ARCH_STATE_DEFINE(AP_POWER_STATE_S0, arch_s0_entry,
			arch_s0_run, arch_s0_exit);
