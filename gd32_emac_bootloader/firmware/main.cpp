/**
 * @file main.cpp
 *
 */
/* Copyright (C) 2022-2026 by Arjan van Vught mailto:info@gd32-dmx.org
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

#include <cstdio>
#include <cstdint>

#include "board.h"
#include "watchdog.h"
#include "network.h"
#include "display.h"
#include "board_statusled.h"
#include "remoteconfig.h"
#include "firmware/firmwareversion.h"
#include "software_version.h"
#include "flashcodeinstall.h"
#include "configstore.h"
#include "firmware.h"
#include "gd32.h"

namespace board {
void RebootHandler() {}
} // namespace hal

int main() {
    rcu_periph_clock_enable(KEY_BOOTLOADER_TFTP_RCU_GPIOx);
#if defined(GD32F4XX) || defined(GD32H7XX)
    rcu_periph_clock_enable(RCU_PMU);
#if defined(GD32F4XX)
    pmu_backup_ldo_config(PMU_BLDOON_ON);
#endif
    rcu_periph_clock_enable(RCU_BKPSRAM);
    pmu_backup_write_enable();
    gpio_mode_set(KEY_BOOTLOADER_TFTP_GPIOx, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, KEY_BOOTLOADER_TFTP_GPIO_PINx);
#else
    rcu_periph_clock_enable(RCU_AF);
    rcu_periph_clock_enable(KEY_BOOTLOADER_TFTP_RCU_GPIOx);
    if constexpr (KEY_BOOTLOADER_TFTP_GPIOx == GPIOA) {
        if constexpr ((KEY_BOOTLOADER_TFTP_GPIO_PINx == GPIO_PIN_13) || (KEY_BOOTLOADER_TFTP_GPIO_PINx == GPIO_PIN_14)) {
            gpio_pin_remap_config(GPIO_SWJ_DISABLE_REMAP, ENABLE);
        }
    }
    gpio_init(KEY_BOOTLOADER_TFTP_GPIOx, GPIO_MODE_IPU, GPIO_OSPEED_50MHZ, KEY_BOOTLOADER_TFTP_GPIO_PINx);
#endif

    const auto kIsNotRemote = (bkp_data_read(BKP_DATA_1) != 0xA5A5);
    const auto kIsNotKey = (gpio_input_bit_get(KEY_BOOTLOADER_TFTP_GPIOx, KEY_BOOTLOADER_TFTP_GPIO_PINx));
    const uint32_t* reset_p = reinterpret_cast<uint32_t*>(FLASH_BASE + OFFSET_UIMAGE + 4);
    const auto kIsEmpty = (*reset_p == UINT32_MAX);

    if (kIsNotRemote && kIsNotKey && !kIsEmpty) {
        // https://developer.arm.com/documentation/ka001423/1-0
        // 1. Disable interrupt response.
        __disable_irq();
        // 2. Disable all enabled interrupts in NVIC.
                for (auto& reg : NVIC->ICER) {
            reg = 0xFFFFFFFF;
        }
        /* 3. Disable all enabled peripherals which might generate interrupt requests.
         *  Clear all pending interrupt flags in those peripherals.
         *  This part is device-dependent, and you can write it by referring to device datasheet.
         */

        /* Clear all pending interrupt requests in NVIC. */
                for (auto& reg : NVIC->ICPR) {
            reg = 0xFFFFFFFF;
        }
        // 4. Disable SysTick and clear its exception pending bit.
        SysTick->CTRL = 0;
        SCB->ICSR |= SCB_ICSR_PENDSTCLR_Msk;
        // 5. Load the vector table address of user application code in to VTOR.
        SCB->VTOR = FLASH_BASE + OFFSET_UIMAGE;
        // 6. Use the MSP as the current SP.
        // Set the MSP with the value from the vector table used by the application.
        __set_MSP((reinterpret_cast<unsigned int*>((SCB->VTOR))[0]));
        // In thread mode, enable privileged access and use the MSP as the current SP.
        __set_CONTROL(0);
        // 7. Enable interrupts.
        __enable_irq();
        // 8. Call the reset handler
        asm volatile("bx %0;" : : "r"(*reset_p));
    }

    board::Init();
    Display display(4);
    ConfigStore config_store;
    network::Init();
    FirmwareVersion fw(kSoftwareVersion, __DATE__, __TIME__);
    FlashCodeInstall flashcode_install;

    printf("Remote=%c, Key=%c, Empty=%c\n", kIsNotRemote ? 'N' : 'Y', kIsNotKey ? 'N' : 'Y', kIsEmpty ? 'Y' : 'N');
    fw.Print("Bootloader TFTP Server");

    RemoteConfig remote_config(remoteconfig::Output::CONFIG);

    display.Printf(3, "Bootloader TFTP Srvr");

    board::statusled::SetMode(board::statusled::Mode::kFast);
    watchdog::Init();

    while (1) {
        watchdog::Feed();
        network::Run();
        board::Run();
    }
}
