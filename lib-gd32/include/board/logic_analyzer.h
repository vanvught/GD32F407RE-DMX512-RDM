/**
 * @file logic_analyzer.h
 *
 */
/* Copyright (C) 2022-2024 by Arjan van Vught mailto:info@gd32-dmx.org
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

#ifndef BOARD_LOGIC_ANALYZER_H_
#define BOARD_LOGIC_ANALYZER_H_

#define LOGIC_ANALYZER_CH0_GPIO_PINx	GPIO_PIN_3
#define LOGIC_ANALYZER_CH1_GPIO_PINx	GPIO_PIN_4
#define LOGIC_ANALYZER_CH2_GPIO_PINx	GPIO_PIN_5
#define LOGIC_ANALYZER_CH3_GPIO_PINx	GPIO_PIN_8
#define LOGIC_ANALYZER_CH4_GPIO_PINx	GPIO_PIN_9
#define LOGIC_ANALYZER_CH5_GPIO_PINx	GPIO_PIN_13
#define LOGIC_ANALYZER_CH6_GPIO_PINx	GPIO_PIN_11
#define LOGIC_ANALYZER_CH7_GPIO_PINx	GPIO_PIN_15

#define LOGIC_ANALYZER_CH0_GPIOx		GPIOB
#define LOGIC_ANALYZER_CH1_GPIOx		GPIOB
#define LOGIC_ANALYZER_CH2_GPIOx		GPIOB
#define LOGIC_ANALYZER_CH3_GPIOx		GPIOC
#define LOGIC_ANALYZER_CH4_GPIOx		GPIOC
#define LOGIC_ANALYZER_CH5_GPIOx		GPIOC
#define LOGIC_ANALYZER_CH6_GPIOx		GPIOA
#define LOGIC_ANALYZER_CH7_GPIOx		GPIOA

#define LOGIC_ANALYZER_CH0_RCU_GPIOx	RCU_GPIOB
#define LOGIC_ANALYZER_CH1_RCU_GPIOx	RCU_GPIOB
#define LOGIC_ANALYZER_CH2_RCU_GPIOx	RCU_GPIOB
#define LOGIC_ANALYZER_CH3_RCU_GPIOx	RCU_GPIOC
#define LOGIC_ANALYZER_CH4_RCU_GPIOx	RCU_GPIOC
#define LOGIC_ANALYZER_CH5_RCU_GPIOx	RCU_GPIOC
#define LOGIC_ANALYZER_CH6_RCU_GPIOx	RCU_GPIOA
#define LOGIC_ANALYZER_CH7_RCU_GPIOx	RCU_GPIOA

#endif /* BOARD_LOGIC_ANALYZER_H_ */
