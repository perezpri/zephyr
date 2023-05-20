/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ap_pwrseq/ap_pwrseq_sm.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(ap_pwrseq, LOG_LEVEL_INF);

static int app_g3_entry(void *data)
{
	LOG_INF("\t%s", __func__);
	return 0;
}

static int app_g3_run(void *data)
{
	LOG_INF("\t%s", __func__);
	return 0;
}


static int app_g3_exit(void *data)
{
	LOG_INF("\t%s", __func__);
	return 0;
}

AP_POWER_APP_STATE_DEFINE(AP_POWER_STATE_G3, app_g3_entry,
			app_g3_run, app_g3_exit);


static int app_s5_entry(void *data)
{
	LOG_INF("\t%s", __func__);
	return 0;
}

static int app_s5_run(void *data)
{
	LOG_INF("\t%s", __func__);
	return 0;
}


static int app_s5_exit(void *data)
{
	LOG_INF("\t%s", __func__);
	return 0;
}

AP_POWER_APP_STATE_DEFINE(AP_POWER_STATE_S5, app_s5_entry,
			app_s5_run, app_s5_exit);

static int app_s3_entry(void *data)
{
	LOG_INF("\t%s", __func__);
	return 0;
}

static int app_s3_run(void *data)
{
	LOG_INF("\t%s", __func__);
	return 0;
}


static int app_s3_exit(void *data)
{
	LOG_INF("\t%s", __func__);
	return 0;
}

AP_POWER_APP_STATE_DEFINE(AP_POWER_STATE_S3, app_s3_entry,
			app_s3_run, app_s3_exit);

static int app_s0_entry(void *data)
{
	LOG_INF("\t%s", __func__);
	return 0;
}

static int app_s0_run(void *data)
{
	LOG_INF("\t%s", __func__);
	return 0;
}


static int app_s0_exit(void *data)
{
	LOG_INF("\t%s", __func__);
	return 0;
}

AP_POWER_APP_STATE_DEFINE(AP_POWER_STATE_S0, app_s0_entry,
			app_s0_run, app_s0_exit);
