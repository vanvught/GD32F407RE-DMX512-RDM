/**
 * @file gd32_gpio.h
 *
 */
/* Copyright (C) 2021-2025 by Arjan van Vught mailto:info@gd32-dmx.org
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

#ifndef GD32_GPIO_H_
#define GD32_GPIO_H_

#include <stdint.h>
#include <assert.h>

#include "gd32.h"

#if defined(GD32F10X) || defined(GD32F20X) || defined(GD32F30X)
#define GPIO_FSEL_OUTPUT GPIO_MODE_OUT_PP
#define GPIO_FSEL_INPUT GPIO_MODE_IPU
#define GPIO_PULL_UP GPIO_MODE_IPU
#define GPIO_PULL_DOWN GPIO_MODE_IPD
#define GPIO_PULL_DISABLE GPIO_MODE_IN_FLOATING
#define GPIO_INT_CFG_NEG_EDGE EXTI_TRIG_FALLING
#define GPIO_INT_CFG_BOTH EXTI_TRIG_BOTH
#elif defined(GD32F4XX) || defined(GD32H7XX)
#define GPIO_FSEL_OUTPUT GPIO_MODE_OUTPUT
#define GPIO_FSEL_INPUT GPIO_MODE_INPUT
#define GPIO_PULL_UP GPIO_PUPD_PULLUP
#define GPIO_PULL_DOWN GPIO_PUPD_PULLDOWN
#define GPIO_PULL_DISABLE GPIO_PUPD_NONE
#endif

#if defined(GD32F4XX)
#define GPIOx_OCTL_OFFSET 0x14U;
#define GPIOx_BOP_OFFSET 0x18U;
#define GPIOx_BC_OFFSET 0x28U;
#else
#define GPIOx_BOP_OFFSET 0x10U;
#define GPIOx_BC_OFFSET 0x14U;
#endif

#ifdef __cplusplus
inline void Gd32GpioFsel(uint32_t gpio_periph, uint32_t pin, uint32_t fsel)
{
    switch (gpio_periph)
    {
        case GPIOA:
            rcu_periph_clock_enable(RCU_GPIOA);
            break;
        case GPIOB:
            rcu_periph_clock_enable(RCU_GPIOB);
            break;
        case GPIOC:
            rcu_periph_clock_enable(RCU_GPIOC);
            break;
        case GPIOD:
            rcu_periph_clock_enable(RCU_GPIOD);
            break;
        case GPIOE:
            rcu_periph_clock_enable(RCU_GPIOE);
            break;
        case GPIOF:
            rcu_periph_clock_enable(RCU_GPIOF);
            break;
        case GPIOG:
            rcu_periph_clock_enable(RCU_GPIOG);
            break;
#if !(defined(GD32F10X) || defined(GD32F30X))
        case GPIOH:
            rcu_periph_clock_enable(RCU_GPIOH);
            break;
#if !defined(GD32H7XX)
        case GPIOI:
            rcu_periph_clock_enable(RCU_GPIOI);
            break;
#endif
#endif
#if defined(GD32H7XX)
        case GPIOJ:
            rcu_periph_clock_enable(RCU_GPIOJ);
            break;
        case GPIOK:
            rcu_periph_clock_enable(RCU_GPIOK);
            break;
#endif
        default:
            break;
    }

#if defined(GD32F10X) || defined(GD32F20X) || defined(GD32F30X)
    if (gpio_periph == GPIOA)
    {
        if ((pin == GPIO_PIN_13) || (pin == GPIO_PIN_14))
        {
            rcu_periph_clock_enable(RCU_AF);
            gpio_pin_remap_config(GPIO_SWJ_DISABLE_REMAP, ENABLE);
        }
    }

    gpio_init(gpio_periph, fsel, GPIO_OSPEED_50MHZ, pin);
#elif defined(GD32F4XX) || defined(GD32H7XX)
    if (fsel == GPIO_FSEL_OUTPUT)
    {
        gpio_mode_set(gpio_periph, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, pin);
        gpio_output_options_set(gpio_periph, GPIO_OTYPE_PP, GPIO_OSPEED, pin);
    }
    else
    {
        gpio_mode_set(gpio_periph, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, pin);
    }
#else
#error MCU not defined
#endif
}

#if !defined(GD32H7XX)
inline void Gd32GpioIntCfg(uint32_t gpio, uint32_t trig_type)
{
    const uint32_t kLinex = BIT(GD32_GPIO_TO_NUMBER(gpio));

    switch (trig_type)
    {
        case EXTI_TRIG_RISING:
            EXTI_RTEN |= kLinex;
            EXTI_FTEN &= ~kLinex;
            break;
        case EXTI_TRIG_FALLING:
            EXTI_RTEN &= ~kLinex;
            EXTI_FTEN |= kLinex;
            break;
        case EXTI_TRIG_BOTH:
            EXTI_RTEN |= kLinex;
            EXTI_FTEN |= kLinex;
            break;
        default:
            break;
    }

    const auto kOutputPort = GD32_GPIO_TO_PORT(gpio);
    const auto kOutputPin = GD32_GPIO_TO_NUMBER(gpio);

#if defined(GD32F10X) || defined(GD32F20X) || defined(GD32F30X)
    gpio_exti_source_select(kOutputPort, kOutputPin);
#elif defined(GD32F4XX)
    syscfg_exti_line_config(kOutputPort, kOutputPin);
#endif
}
#endif

inline uint32_t Gd32GpioToPeriph(uint32_t gpio)
{
    switch ((GD32_Port_TypeDef)GD32_GPIO_TO_PORT(gpio))
    {
        case GD32_GPIO_PORTA:
        case GD32_GPIO_PORTB:
        case GD32_GPIO_PORTC:
        case GD32_GPIO_PORTD:
        case GD32_GPIO_PORTE:
        case GD32_GPIO_PORTF:
        case GD32_GPIO_PORTG:
            return GPIOA + (GD32_GPIO_TO_PORT(gpio) * 0x400);
            break;
#if !(defined(GD32F10X) || defined(GD32F30X))
        case GD32_GPIO_PORTH:
            return GPIOH;
            break;
#if !defined(GD32H7XX)
        case GD32_GPIO_PORTI:
            return GPIOI;
            break;
#endif
#endif
#if defined(GD32H7XX)
        case GD32_GPIO_PORTJ:
            return GPIOJ;
            break;
        case GD32_GPIO_PORTK:
            return GPIOK;
            break;
#endif
        default:
            assert(0);
            return 0;
            break;
    }
}

inline void Gd32GpioFsel(uint32_t gpio, uint32_t fsel)
{
    const uint32_t kGpioPeriph = Gd32GpioToPeriph(gpio);
    const uint32_t kPin = BIT(GD32_GPIO_TO_NUMBER(gpio));

    Gd32GpioFsel(kGpioPeriph, kPin, fsel);
}

inline void Gd32GpioClr(uint32_t gpio)
{
    const uint32_t kGpioPeriph = Gd32GpioToPeriph(gpio);
    const uint32_t kPin = BIT(GD32_GPIO_TO_NUMBER(gpio));

    GPIO_BC(kGpioPeriph) = kPin;
}

inline void Gd32GpioSet(uint32_t gpio)
{
    const uint32_t kGpioPeriph = Gd32GpioToPeriph(gpio);
    const uint32_t kPin = BIT(GD32_GPIO_TO_NUMBER(gpio));

    GPIO_BOP(kGpioPeriph) = kPin;
}

inline void Gd32GpioWrite(uint32_t gpio, uint32_t level)
{
    if (level == 0)
    {
        Gd32GpioClr(gpio);
    }
    else
    {
        Gd32GpioSet(gpio);
    }
}

inline uint32_t Gd32GpioLev(uint32_t gpio)
{
    const uint32_t kGpioPeriph = Gd32GpioToPeriph(gpio);
    const uint32_t kPin = BIT(GD32_GPIO_TO_NUMBER(gpio));

    return static_cast<uint32_t>(0 != (GPIO_ISTAT(kGpioPeriph) & kPin));
}

inline void Gd32GpioSetPud(uint32_t gpio, uint32_t pud)
{
    const uint32_t gpio_periph = Gd32GpioToPeriph(gpio);
    const uint32_t pin = BIT(GD32_GPIO_TO_NUMBER(gpio));

#if defined(GD32F10X) || defined(GD32F20X) || defined(GD32F30X)
    gpio_init(gpio_periph, pud, GPIO_OSPEED_50MHZ, pin);
#elif defined(GD32F4XX) || defined(GD32H7XX)
    if (pud == GPIO_PULL_UP)
    {
        gpio_mode_set(gpio_periph, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, pin);
    }
    else if (pud == GPIO_PULL_DOWN)
    {
        gpio_mode_set(gpio_periph, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, pin);
    }
    else
    {
        gpio_mode_set(gpio_periph, GPIO_MODE_INPUT, GPIO_PUPD_NONE, pin);
    }
#endif
}

#if defined(GD32F4XX) || defined(GD32H7XX)
template <uint32_t gpio_periph, uint32_t mode, uint32_t pull_up_down, uint32_t pin> inline void Gd32GpioModeSet()
{
    static_assert(pin != 0, "pin cannot be zero");
    static_assert(pin == (1U << __builtin_ctz(pin)), "Only single pin values are allowed");

    uint32_t ctl = GPIO_CTL(gpio_periph);
    uint32_t pupd = GPIO_PUD(gpio_periph);

    constexpr auto i = 31U - __builtin_clz(pin);

    /* clear the specified pin mode bits */
    ctl &= ~GPIO_MODE_MASK(i);
    /* set the specified pin mode bits */
    ctl |= GPIO_MODE_SET(i, mode);

    /* clear the specified pin pupd bits */
    pupd &= ~GPIO_PUPD_MASK(i);
    /* set the specified pin pupd bits */
    pupd |= GPIO_PUPD_SET(i, pull_up_down);

    GPIO_CTL(gpio_periph) = ctl;
    GPIO_PUD(gpio_periph) = pupd;
}

template <uint32_t gpio_periph, uint32_t alt_func_num, uint32_t pin> inline void Gd32GpioAfSet()
{
    static_assert(pin != 0, "pin cannot be zero");
    static_assert(pin == (1U << __builtin_ctz(pin)), "Only single pin values are allowed");

    constexpr uint32_t kPin = 31U - __builtin_clz(pin);

    auto afrl = GPIO_AFSEL0(gpio_periph);
    auto afrh = GPIO_AFSEL1(gpio_periph);

    if constexpr (kPin < 8U)
    {
        /* clear the specified pin alternate function bits */
        afrl &= ~GPIO_AFR_MASK(kPin);
        afrl |= GPIO_AFR_SET(kPin, alt_func_num);
    }
    else if constexpr (kPin < 16U)
    {
        /* clear the specified pin alternate function bits */
        afrh &= ~GPIO_AFR_MASK(kPin - 8U);
        afrh |= GPIO_AFR_SET(kPin - 8U, alt_func_num);
    }

    GPIO_AFSEL0(gpio_periph) = afrl;
    GPIO_AFSEL1(gpio_periph) = afrh;
}
#else
template <uint32_t gpio_periph, uint32_t mode, uint32_t pin, uint32_t speed = GPIO_OSPEED_50MHZ> inline void gd32_gpio_init()
{
    /* GPIO mode configuration */
    auto temp_mode = (mode & 0x0F);

    /* GPIO speed configuration */
    if constexpr ((0x00U) != (mode & (0x10U)))
    {
        /* output mode max speed: 10MHz, 2MHz, 50MHz */
        temp_mode |= speed;
    }

    constexpr uint32_t kPinPos = 31U - __builtin_clz(pin);

    if constexpr (kPinPos < 8U)
    {
        uint32_t reg = GPIO_CTL0(gpio_periph);
        /* clear the specified pin mode bits */
        reg &= ~GPIO_MODE_MASK(kPinPos);
        /* set the specified pin mode bits */
        reg |= GPIO_MODE_SET(kPinPos, temp_mode);

        /* set IPD or IPU */
        if constexpr (GPIO_MODE_IPD == mode)
        {
            /* reset the corresponding OCTL bit */
            GPIO_BC(gpio_periph) = (1U << kPinPos);
        }
        else
        {
            /* set the corresponding OCTL bit */
            if constexpr (GPIO_MODE_IPU == mode)
            {
                GPIO_BOP(gpio_periph) = (1U << kPinPos);
            }
        }
        /* set GPIO_CTL0 register */
        GPIO_CTL0(gpio_periph) = reg;
    }
    else
    {
        /* configure the eight high port pins with GPIO_CTL1 */
        constexpr uint32_t kHighPinPos = kPinPos - 8U;
        uint32_t reg = GPIO_CTL1(gpio_periph);
        /* clear the specified pin mode bits */
        reg &= ~GPIO_MODE_MASK(kHighPinPos);
        /* set the specified pin mode bits */
        reg |= GPIO_MODE_SET(kHighPinPos, temp_mode);

        /* set IPD or IPU */
        if constexpr (GPIO_MODE_IPD == mode)
        {
            /* reset the corresponding OCTL bit */
            GPIO_BC(gpio_periph) = (1U << kPinPos);
        }
        else
        {
            /* set the corresponding OCTL bit */
            if (GPIO_MODE_IPU == mode)
            {
                GPIO_BOP(gpio_periph) = (1U << kPinPos);
            }
        }
        /* set GPIO_CTL1 register */
        GPIO_CTL1(gpio_periph) = reg;
    }
}
#endif
#endif

#endif // GD32_GPIO_H_
