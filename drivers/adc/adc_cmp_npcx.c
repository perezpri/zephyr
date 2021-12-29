/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/sensor.h>
#include "adc_npcx_threshold.h"
#include "adc_cmp_npcx.h"

#include <logging/log.h>

LOG_MODULE_REGISTER(adc_cmp_npcx, LOG_LEVEL_ERR);

#define ADC_CMP_NPCX_UNDEFINED		(-1)

struct adc_cmp_npcx_data {
	struct k_work work;
	sensor_trigger_handler_t handler;
	const struct device *dev;
};

struct adc_cmp_npcx_config {
	const struct device *adc;
	uint8_t th_sel;
	uint8_t chnsel;
	uint16_t thrval;
	enum adc_cmp_npcx_comparison comparison;
};

#define DT_DRV_COMPAT nuvoton_adc_cmp

#define DRV_CONFIG(dev) ((const struct adc_cmp_npcx_config *)(dev)->config)
#define DRV_DATA(dev) ((struct adc_cmp_npcx_data *)(dev)->data)

static void adc_cmp_npcx_trigger_work_handler(struct k_work *item)
{
	struct adc_cmp_npcx_data *data =
			CONTAINER_OF(item, struct adc_cmp_npcx_data, work);
	struct sensor_trigger trigger = {
		.type = SENSOR_TRIG_THRESHOLD,
		.chan = SENSOR_CHAN_VOLTAGE
	};

	if (data->handler)
		data->handler(data->dev, &trigger);
}

static int adc_cmp_npcx_init(const struct device *dev)
{
	const struct adc_cmp_npcx_config *const config = DRV_CONFIG(dev);
	struct adc_cmp_npcx_data *data = DRV_DATA(dev);
	struct adc_npcx_threshold_control_t control;
	int ret;

	LOG_DBG("Initialize adc cmp threshold selection (%d)", config->th_sel);
	/* Data must keep device reference for worker handler*/
	data->dev = dev;
	k_work_init(&data->work, adc_cmp_npcx_trigger_work_handler);

	/* Set ADC channel selection */
	control.th_sel = config->th_sel;
	control.param = ADC_NPCX_THRESHOLD_PARAM_CHNSEL;
	control.val = (uint32_t)config->chnsel;
	ret = adc_npcx_threshold_ctrl_set_param(config->adc, &control);
	if (ret)
		goto init_error;

	/* Set Worker to enable notifications */
	control.param = ADC_NPCX_THRESHOLD_PARAM_WORK;
	control.val = (uint32_t)&data->work;
	ret = adc_npcx_threshold_ctrl_set_param(config->adc, &control);
	if (ret)
		goto init_error;

	if (config->thrval != ADC_CMP_NPCX_UNDEFINED) {
		control.param = ADC_NPCX_THRESHOLD_PARAM_THVAL;
		control.val = (uint32_t)config->thrval;
		ret = adc_npcx_threshold_ctrl_set_param(config->adc, &control);
		if (ret)
			goto init_error;
	}

	if (config->comparison != ADC_CMP_NPCX_UNDEFINED) {
		control.param = ADC_NPCX_THRESHOLD_PARAM_L_H;
		control.val =
			config->comparison == ADC_CMP_NPCX_COMPARISON_GREATER ?
			ADC_NPCX_THRESHOLD_PARAM_L_H_HIGHER :
			ADC_NPCX_THRESHOLD_PARAM_L_H_LOWER;
		ret = adc_npcx_threshold_ctrl_set_param(config->adc, &control);
	}
init_error:
	if (ret)
		LOG_ERR("Error setting parameter %d - value %d",
			(uint32_t)control.param, control.val);

	return ret;
}

int adc_cmp_npcx_attr_set(const struct device *dev,
			  enum sensor_channel chan,
			  enum sensor_attribute attr,
			  const struct sensor_value *val)
{
	const struct adc_cmp_npcx_config *const config = DRV_CONFIG(dev);
	struct adc_npcx_threshold_control_t control;
	int ret;

	if (chan != SENSOR_CHAN_VOLTAGE)
		return -ENOTSUP;

	switch (attr) {
	case SENSOR_ATTR_LOWER_THRESH:
	case SENSOR_ATTR_UPPER_THRESH:
		/* Set threshold value first */
		control.th_sel = config->th_sel;
		control.param = ADC_NPCX_THRESHOLD_PARAM_THVAL;
		control.val = val->val1;
		ret = adc_npcx_threshold_ctrl_set_param(config->adc, &control);
		if (ret)
			break;

		/* Then set lower or higher threshold */
		control.param = ADC_NPCX_THRESHOLD_PARAM_L_H;
		control.val = attr == SENSOR_ATTR_UPPER_THRESH ?
			ADC_NPCX_THRESHOLD_PARAM_L_H_HIGHER :
			ADC_NPCX_THRESHOLD_PARAM_L_H_LOWER;
		ret = adc_npcx_threshold_ctrl_set_param(config->adc, &control);
		break;

	case SENSOR_ATTR_ALERT:
		control.val = val->val1;
		ret = adc_npcx_threshold_ctrl_enable(config->adc, config->th_sel,
						     !!control.val);
		break;
	default:
		ret = -ENOTSUP;
	}
	return ret;
}

int adc_cmp_npcx_attr_get(const struct device *dev,
			  enum sensor_channel chan,
			  enum sensor_attribute attr,
			  struct sensor_value *val)
{
	return -ENOTSUP;
}

int adc_cmp_npcx_trigger_set(const struct device *dev,
			     const struct sensor_trigger *trig,
			     sensor_trigger_handler_t handler)
{
	const struct adc_cmp_npcx_config *const config = DRV_CONFIG(dev);
	struct adc_cmp_npcx_data *data = DRV_DATA(dev);
	struct adc_npcx_threshold_control_t control;

	if (trig == NULL || handler == NULL)
		return -EINVAL;

	if (trig->type != SENSOR_TRIG_THRESHOLD ||
	    trig->chan != SENSOR_CHAN_VOLTAGE)
		return -ENOTSUP;

	data->handler = handler;

	control.th_sel = config->th_sel;
	control.param = ADC_NPCX_THRESHOLD_PARAM_WORK;
	control.val = (uint32_t)&data->work;
	return adc_npcx_threshold_ctrl_set_param(config->adc, &control);
}

int adc_cmp_npcx_channel_get(const struct device *dev,
			     enum sensor_channel chan,
			     struct sensor_value *val)
{
	const struct adc_cmp_npcx_config *const config = DRV_CONFIG(dev);

	if (chan != SENSOR_CHAN_VOLTAGE)
		return -ENOTSUP;

	if (val == NULL)
		return -EINVAL;

	val->val1 = (uint32_t)&config->adc;
	return 0;
}

static const struct sensor_driver_api adc_cmp_npcx_api = {
	.attr_set = adc_cmp_npcx_attr_set,
	.attr_get = adc_cmp_npcx_attr_get,
	.trigger_set = adc_cmp_npcx_trigger_set,
	.channel_get = adc_cmp_npcx_channel_get,
};

#define NPCX_ADC_CMP_INIT(inst)                                               \
	static struct adc_cmp_npcx_data adc_cmp_npcx_data_##inst;             \
	static const struct adc_cmp_npcx_config adc_cmp_npcx_config_##inst = {\
		.adc = DEVICE_DT_GET(                                         \
			DT_IO_CHANNELS_CTLR(DT_INST(inst, DT_DRV_COMPAT))),   \
		.th_sel = inst,                                               \
		.chnsel = DT_IO_CHANNELS_INPUT(DT_INST(inst, DT_DRV_COMPAT)), \
		.thrval = (uint16_t)COND_CODE_1(DT_NODE_HAS_PROP(             \
			DT_DRV_INST(inst), threshold),                        \
			(DT_PROP(DT_DRV_INST(inst), threshold)),              \
			ADC_CMP_NPCX_UNDEFINED),                              \
		.comparison = (enum adc_cmp_npcx_comparison)COND_CODE_1(      \
			DT_NODE_HAS_PROP(DT_DRV_INST(inst), comparison),      \
			(DT_STRING_TOKEN(DT_DRV_INST(inst), comparison)),     \
			ADC_CMP_NPCX_UNDEFINED),                              \
	};                                                                    \
	DEVICE_DT_INST_DEFINE(inst, adc_cmp_npcx_init, NULL,                  \
			      &adc_cmp_npcx_data_##inst,                      \
			      &adc_cmp_npcx_config_##inst, POST_KERNEL,       \
			      52,                                             \
			      &adc_cmp_npcx_api);
DT_INST_FOREACH_STATUS_OKAY(NPCX_ADC_CMP_INIT)
