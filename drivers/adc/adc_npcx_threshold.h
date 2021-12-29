/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ADC_NPCX_THRESHOLD_H_
#define _ADC_NPCX_THRESHOLD_H_

#include <device.h>

enum adc_npcx_threshold_param_l_h {
	ADC_NPCX_THRESHOLD_PARAM_L_H_HIGHER,
	ADC_NPCX_THRESHOLD_PARAM_L_H_LOWER
};

enum adc_npcx_threshold_control_param {
	ADC_NPCX_THRESHOLD_PARAM_CHNSEL,
	ADC_NPCX_THRESHOLD_PARAM_L_H,
	ADC_NPCX_THRESHOLD_PARAM_THVAL,
	ADC_NPCX_THRESHOLD_PARAM_WORK,
	ADC_NPCX_THRESHOLD_PARAM_MAX,
};

struct adc_npcx_threshold_control_t {
	uint8_t th_sel;
	enum adc_npcx_threshold_control_param param;
	uint32_t val;
};

int adc_npcx_threshold_ctrl_set_param(const struct device *dev,
				      const struct adc_npcx_threshold_control_t
				      *control);

int adc_npcx_threshold_ctrl_enable(const struct device *dev, uint8_t th_sel,
				   const bool enable);

#endif /*_ADC_NPCX_THRESHOLD_H_ */
