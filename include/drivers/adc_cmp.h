#ifndef _ADC_CMP_H_
#define _ADC_CMP_H_

#include <device.h>

enum threshold_comparison {
	THRESHOLD_LESS_OR_EQUAL,
	THRESHOLD_GREATER,
};

typedef void (*threshold_comparator_cb_t)(const struct device *dev,
					  void *data);

struct threshold_comparator_config {
	/* Determines relation between measured value and assertion threshold */
	enum threshold_comparison comparison;

	/*
	 * Raw value present on ADC as threshold assert value
	 * (adc_raw_to_millivolts may be used to get value in mV)
	 */
	int32_t raw_threshold;

	/* Callback function to be invoked when threashold is asserted */
	threshold_comparator_cb_t callback;

	/* User data passed with callback function */
	void *callback_data;
};

typedef int (*threshold_cmp_api_setup)(const struct device *dev,
				       const struct threshold_comparator_config
				       *config);

typedef int (*threshold_cmp_api_enable)(const struct device *dev, bool enable);

typedef const struct device *(*threshold_cmp_api_get_adc)(const struct device *dev);

struct threshold_comparator_api {
	threshold_cmp_api_setup    setup;
	threshold_cmp_api_enable   enable;
	threshold_cmp_api_get_adc  get_adc;
};

int threshold_comparator_setup(const struct device *dev,
			       const struct threshold_comparator_config *config);

int threshold_comparator_enable(const struct device *dev, bool enable);

const struct device *threshold_comparator_get_adc(const struct device *dev);

#endif /* _ADC_CMP_H_ */
