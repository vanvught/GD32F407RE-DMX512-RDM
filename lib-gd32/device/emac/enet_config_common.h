/**
 * enet_config_common.h
 *
 */
/* Copyright (C) 2022 by Arjan van Vught mailto:info@gd32-dmx.org
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

#ifndef ENET_CONFIG_COMMON_H_
#define ENET_CONFIG_COMMON_H_

#define LINK_CHECK_GPIO_CLK				RCU_GPIOB
#define LINK_CHECK_GPIO_PORT			GPIOB
#define LINK_CHECK_GPIO_PIN 			GPIO_PIN_0
#define LINK_CHECK_EXTI_LINE			EXTI_0
#define LINK_CHECK_EXTI_IRQn			EXTI0_IRQn
#define LINK_CHECK_IRQ_HANDLE			EXTI0_IRQHandler
#if !defined (GD32F4XX)
# define LINK_CHECK_EXTI_CLK			RCU_AF
# define LINK_CHECK_EXTI_PORT_SOURCE	GPIO_PORT_SOURCE_GPIOB
# define LINK_CHECK_EXTI_PIN_SOURCE		GPIO_PIN_SOURCE_0
# define LINK_CHECK_EXTI_SOURCE_CONFIG	gpio_exti_source_select
# define LINK_CHECK_GPIO_CONFIG			gpio_init(LINK_CHECK_GPIO_PORT, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, LINK_CHECK_GPIO_PIN);
#else
# define LINK_CHECK_EXTI_CLK			RCU_SYSCFG
# define LINK_CHECK_EXTI_PORT_SOURCE	EXTI_SOURCE_GPIOB
# define LINK_CHECK_EXTI_PIN_SOURCE		EXTI_SOURCE_PIN0
# define LINK_CHECK_EXTI_SOURCE_CONFIG	syscfg_exti_line_config
# define LINK_CHECK_GPIO_CONFIG			gpio_mode_set(LINK_CHECK_GPIO_PORT, GPIO_MODE_INPUT, GPIO_PUPD_NONE, LINK_CHECK_GPIO_PIN);
#endif

#endif /* ENET_CONFIG_COMMON_H_ */
