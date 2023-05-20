/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ap_pwrseq/ap_pwrseq_sm.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(ap_pwrseq, LOG_LEVEL_INF);

static int chipset_g3_entry(void *data)
{
	LOG_INF("\t%s", __func__);
	return 0;
}

static int chipset_g3_run(void *data)
{
	LOG_INF("\t%s", __func__);
	return 0;
}


static int chipset_g3_exit(void *data)
{
	LOG_INF("\t%s", __func__);
	return 0;
}

AP_POWER_CHIPSET_STATE_DEFINE(AP_POWER_STATE_G3, chipset_g3_entry,
			chipset_g3_run, chipset_g3_exit);


static int chipset_s5_entry(void *data)
{
	LOG_INF("\t%s", __func__);
	return 0;
}

static int chipset_s5_run(void *data)
{
	LOG_INF("\t%s", __func__);
	if (ap_pwrseq_sm_is_event_set(data,
		AP_PWRSEQ_EVENT_POWER_SIGNAL)) {
		LOG_INF("\t\tTransition");
		return ap_pwrseq_sm_set_state(data, AP_POWER_STATE_S4);
	}
	return 0;
}


static int chipset_s5_exit(void *data)
{
	LOG_INF("\t%s", __func__);
	return 0;
}

AP_POWER_CHIPSET_STATE_DEFINE(AP_POWER_STATE_S5, chipset_s5_entry,
			chipset_s5_run, chipset_s5_exit);

static int chipset_s3_entry(void *data)
{
	LOG_INF("\t%s", __func__);
	return 0;
}

static int chipset_s3_run(void *data)
{
	LOG_INF("\t%s", __func__);
	if (ap_pwrseq_sm_is_event_set(data,
		AP_PWRSEQ_EVENT_HOST)) {
		LOG_INF("\t\tTransition");
		return ap_pwrseq_sm_set_state(data, AP_POWER_STATE_S0);
	}
	return 0;
}


static int chipset_s3_exit(void *data)
{
	LOG_INF("\t%s", __func__);
	return 0;
}

AP_POWER_CHIPSET_STATE_DEFINE(AP_POWER_STATE_S3, chipset_s3_entry,
			chipset_s3_run, chipset_s3_exit);

static int chipset_s0_entry(void *data)
{
	LOG_INF("\t%s", __func__);
	return 0;
}

static int chipset_s0_run(void *data)
{
	LOG_INF("\t%s", __func__);
	return 0;
}


static int chipset_s0_exit(void *data)
{
	LOG_INF("\t%s", __func__);
	return 0;
}

AP_POWER_CHIPSET_STATE_DEFINE(AP_POWER_STATE_S0, chipset_s0_entry,
			chipset_s0_run, chipset_s0_exit);
