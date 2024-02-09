/**
 * @file gd32_adc.cpp
 *
 */
/* Copyright (C) 2024 by Arjan van Vught mailto:info@gd32-dmx.org
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <cstdint>

#include "gd32.h"

#define ADC_TEMP_CALIBRATION_VALUE_25		REG16(0x1FF0F7C0)
#define ADC_TEMP_CALIBRATION_VALUE_MINUS40	REG16(0x1FF0F7C2)

static float avg_slope;
static int32_t temperature_value_25;
static int32_t temperature_value_minus40;

static void rcu_config() {
    /* enable ADC clock */
    rcu_periph_clock_enable(RCU_ADC2);
}

static void adc_config() {
    /* reset ADC */
    adc_deinit(ADC2);
    /* ADC clock config */
    adc_clock_config(ADC2, ADC_CLK_SYNC_HCLK_DIV6);
    /* ADC contineous function enable */
    adc_special_function_config(ADC2, ADC_CONTINUOUS_MODE, DISABLE);
    /* ADC scan mode enable */
    adc_special_function_config(ADC2, ADC_SCAN_MODE, ENABLE);
    /* ADC resolution config */
    adc_resolution_config(ADC2, ADC_RESOLUTION_12B);
    /* ADC data alignment config */
    adc_data_alignment_config(ADC2, ADC_DATAALIGN_RIGHT);

    /* ADC channel length config */
    adc_channel_length_config(ADC2, ADC_INSERTED_CHANNEL, 2);

    /* ADC temperature sensor channel config */
    adc_inserted_channel_config(ADC2, 0, ADC_CHANNEL_18, 480);
    /* ADC internal reference voltage channel config */
    adc_inserted_channel_config(ADC2, 1, ADC_CHANNEL_19, 480);

    /* enable ADC temperature channel */
    adc_internal_channel_config(ADC_CHANNEL_INTERNAL_TEMPSENSOR, ENABLE);
    /* enable internal reference voltage channel */
    adc_internal_channel_config(ADC_CHANNEL_INTERNAL_VREFINT, ENABLE);

    /* ADC trigger config */
    adc_external_trigger_config(ADC2, ADC_INSERTED_CHANNEL, EXTERNAL_TRIGGER_DISABLE);

    /* enable ADC interface */
    adc_enable(ADC2);
    /* wait for ADC stability */
    udelay(1000);
    /* ADC calibration mode config */
    adc_calibration_mode_config(ADC2, ADC_CALIBRATION_OFFSET_MISMATCH);
    /* ADC calibration number config */
    adc_calibration_number(ADC2, ADC_CALIBRATION_NUM1);
    /* ADC calibration and reset calibration */
    adc_calibration_enable(ADC2);

    /* ADC software trigger enable */
    adc_software_trigger_enable(ADC2, ADC_INSERTED_CHANNEL);
}

void gd32_adc_init() {
	rcu_config();
	adc_config();

	temperature_value_25 = ADC_TEMP_CALIBRATION_VALUE_25 & 0x0FFF;
	temperature_value_minus40 = ADC_TEMP_CALIBRATION_VALUE_MINUS40 & 0x0FFF;
	avg_slope = -(temperature_value_25 - temperature_value_minus40) / (25.0f + 40.0f);
}

float gd32_adc_gettemp() {
	const auto temperature = (temperature_value_25 - ADC_IDATA0(ADC2)) * 3.3f / 4096 / avg_slope * 1000 + 25;
	adc_software_trigger_enable(ADC2, ADC_INSERTED_CHANNEL);
	return temperature;
}

float gd32_adc_getvref() {
	const auto vref_value = (ADC_IDATA1(ADC2) * 3.3f / 4096);
	adc_software_trigger_enable(ADC2, ADC_INSERTED_CHANNEL);
	return vref_value;
}
