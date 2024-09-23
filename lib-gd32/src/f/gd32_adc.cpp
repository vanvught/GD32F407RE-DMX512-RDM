/**
 * @file gd32_adc.cpp
 *
 */
/* Copyright (C) 2021-2023 by Arjan van Vught mailto:info@gd32-dmx.org
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

void gd32_adc_init() {
	rcu_periph_clock_enable(RCU_ADC0);
#if !defined (GD32F4XX)
	rcu_adc_clock_config(RCU_CKADC_CKAPB2_DIV12);

	/* ADC SCAN function enable */
	adc_special_function_config(ADC0, ADC_SCAN_MODE, ENABLE);
	/* ADC trigger config */
	adc_external_trigger_source_config(ADC0, ADC_INSERTED_CHANNEL, ADC0_1_2_EXTTRIG_INSERTED_NONE);
	/* ADC data alignment config */
	adc_data_alignment_config(ADC0, ADC_DATAALIGN_RIGHT);
	/* ADC mode config */
	adc_mode_config(ADC_MODE_FREE);
	/* ADC channel length config */
	adc_channel_length_config(ADC0, ADC_INSERTED_CHANNEL, 2);

	/* ADC temperature sensor channel config */
	adc_inserted_channel_config(ADC0, 0, ADC_CHANNEL_16, ADC_SAMPLETIME_239POINT5);
	/* ADC internal reference voltage channel config */
	adc_inserted_channel_config(ADC0, 1, ADC_CHANNEL_17, ADC_SAMPLETIME_239POINT5);

	/* ADC external trigger enable */
	adc_external_trigger_config(ADC0, ADC_INSERTED_CHANNEL, ENABLE);

	/* ADC temperature and Vrefint enable */
	adc_tempsensor_vrefint_enable();
#else
    adc_clock_config(ADC_ADCCK_PCLK2_DIV6);

    /* ADC channel length config */
    adc_channel_length_config(ADC0,ADC_INSERTED_CHANNEL,3);

    /* ADC temperature sensor channel config */
    adc_inserted_channel_config(ADC0,0,ADC_CHANNEL_16,ADC_SAMPLETIME_144);
    /* ADC internal reference voltage channel config */
    adc_inserted_channel_config(ADC0,1,ADC_CHANNEL_17,ADC_SAMPLETIME_144);
    /* ADC 1/4 voltate of external battery config */
    adc_inserted_channel_config(ADC0,2,ADC_CHANNEL_18,ADC_SAMPLETIME_144);

    /* ADC external trigger disable */
    adc_external_trigger_config(ADC0,ADC_INSERTED_CHANNEL,EXTERNAL_TRIGGER_DISABLE);
    /* ADC data alignment config */
    adc_data_alignment_config(ADC0,ADC_DATAALIGN_RIGHT);
    /* ADC SCAN function enable */
    adc_special_function_config(ADC0,ADC_SCAN_MODE,ENABLE);
    /* ADC Vbat channel enable */
    adc_channel_16_to_18(ADC_VBAT_CHANNEL_SWITCH,ENABLE);
    /* ADC temperature and Vref enable */
    adc_channel_16_to_18(ADC_TEMP_VREF_CHANNEL_SWITCH,ENABLE);
#endif
	/* enable ADC interface */
	adc_enable(ADC0);
	udelay(1000);
	/* ADC calibration and reset calibration */
	adc_calibration_enable(ADC0);
    /* ADC software trigger enable */
    adc_software_trigger_enable(ADC0, ADC_INSERTED_CHANNEL);
}

float gd32_adc_gettemp() {
	const float temperature = (1.43f - ADC_IDATA0(ADC0) * 3.3f / 4096) * 1000 / 4.3f + 25;
	adc_software_trigger_enable(ADC0, ADC_INSERTED_CHANNEL);
	return temperature;
}

float gd32_adc_getvref() {
	const float vref_value = (ADC_IDATA1(ADC0) * 3.3f / 4096);
	adc_software_trigger_enable(ADC0, ADC_INSERTED_CHANNEL);
	return vref_value;
}

#if defined (GD32F4XX)
float gd32_adc_getvbat() {
	const float vref_value =  (ADC_IDATA2(ADC0) * 3.3f / 4096) * 4;
	adc_software_trigger_enable(ADC0, ADC_INSERTED_CHANNEL);
	return vref_value;
}
#endif
