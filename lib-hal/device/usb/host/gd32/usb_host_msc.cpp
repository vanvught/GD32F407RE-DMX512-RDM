/**
 * usb_host_msc.cpp
 *
 */
/* Copyright (C) 2023-2024 by Arjan van Vught mailto:info@gd32-dmx.org
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

#ifndef NDEBUG
# undef NDEBUG
#endif

#include <cstdio>
#include <cstdint>

#include "gd32.h"

extern "C" {
#include "usb_host_msc.h"
#include "usbh_core.h"
void console_error(const char *);
}

#include "../lib-hal/ff14b/source/ff.h"
#include "device/usb/host.h"

#if (FF_DEFINED	== 86631)		// R0.14b
 static FATFS fat_fs;
#else
# error Not a recognized/tested FatFs version
#endif

#include "device/usb/host.h"

usbh_user_cb usr_cb = {
    usbh_user_init,
    usbh_user_deinit,
    usbh_user_device_connected,
    usbh_user_device_reset,
    usbh_user_device_disconnected,
    usbh_user_over_current_detected,
    usbh_user_device_speed_detected,
    usbh_user_device_desc_available,
    usbh_user_device_address_assigned,
    usbh_user_configuration_descavailable,
    usbh_user_manufacturer_string,
    usbh_user_product_string,
    usbh_user_serialnum_string,
    usbh_user_enumeration_finish,
    usbh_user_userinput,
    usbh_usr_msc_application,
    usbh_user_device_not_supported,
    usbh_user_unrecovered_error
};

#if defined NODE_SHOWFILE
namespace showfile {
	void usb_ready();
	void usb_disconnected();
}  // namespace showfile
#endif

/* state machine for the USBH_USR_ApplicationState */
#define USBH_USR_FS_MOUNT            0
#define USBH_USR_FS_READY	         1

static uint8_t usbh_usr_application_state = USBH_USR_FS_MOUNT;

static usb::host::Status s_status;
static usb::host::Speed s_speed;
static usb::host::Class s_class;

namespace usb {
namespace host {
Status get_status() {
	return s_status;
}
Speed get_speed() {
	return s_speed;
}
Class get_class() {
	return s_class;
}
}  // namespace host
}  // namespace usb

void usbh_user_init() {
	if (s_status == usb::host::Status::NOT_AVAILABLE) {
		s_status = usb::host::Status::DISCONNECTED;
#ifndef NDEBUG
		puts("USB host library started.");
#endif
	}
}

void usbh_user_deinit() {
	s_status = usb::host::Status::NOT_AVAILABLE;
}

void usbh_user_device_connected() {
	s_status = usb::host::Status::ATTACHED;
	puts("> Device Attached.");
}

void usbh_user_unrecovered_error() {
	s_status = usb::host::Status::UNRECOVERABLE_ERROR;
	puts("> Unrecovered error state.");
}

void usbh_user_device_disconnected() {
	s_status = usb::host::Status::DISCONNECTED;
	puts("> Device Disconnected.");
	usbh_usr_application_state = USBH_USR_FS_MOUNT;
#if defined NODE_SHOWFILE
	showfile::usb_disconnected();
#endif
}

void usbh_user_device_reset() {
	s_status = usb::host::Status::RESET;
	puts("> Reset the USB device.");
}

void usbh_user_device_speed_detected(uint32_t device_speed) {
	if (PORT_SPEED_HIGH == device_speed) {
		s_speed = usb::host::Speed::HIGH;
		puts("> High speed device detected.");
	} else if (PORT_SPEED_FULL == device_speed) {
		s_speed = usb::host::Speed::FULL;
#ifndef NDEBUG
		puts("> Full speed device detected.");
#endif
	} else if (PORT_SPEED_LOW == device_speed) {
		s_speed = usb::host::Speed::LOW;
#ifndef NDEBUG
		puts("> Low speed device detected.");
#endif
	} else {
		s_speed = usb::host::Speed::FAULT;
		puts("> Device Fault.");
	}
}

void usbh_user_device_desc_available(void *device_desc) {
	auto *pDevStr = reinterpret_cast<usb_desc_dev *>(device_desc);

	printf("VID: %04Xh\n", static_cast<uint32_t>(pDevStr->idVendor));
	printf("PID: %04Xh\n", static_cast<uint32_t>(pDevStr->idProduct));
}

void usbh_user_device_address_assigned() {
	puts("usbh_user_device_address_assigned");
}

void usbh_user_configuration_descavailable(usb_desc_config *cfg_desc, usb_desc_itf *itf_desc, usb_desc_ep *ep_desc) {
	auto *id = reinterpret_cast<usb_desc_itf *>(itf_desc);

	if (0x08U == (*id).bInterfaceClass) {
		puts("> Mass storage device connected.");
	} else if (0x03U == (*id).bInterfaceClass) {
		puts("> HID device connected.");
	}
}

void usbh_user_manufacturer_string(void *manufacturer_string) {
    printf("Manufacturer: %s\n", reinterpret_cast<char *>(manufacturer_string));
}

void usbh_user_product_string(void *product_string) {
    printf("Product: %s\n", reinterpret_cast<char *>(product_string));
}

void usbh_user_serialnum_string(void *serial_num_string) {
    printf("Serial Number: %s\n", reinterpret_cast<char *>(serial_num_string));
}

void usbh_user_enumeration_finish() {
	s_status = usb::host::Status::ENUMERATION_COMPLETED;
    puts("> Enumeration completed.");
}

void usbh_user_device_not_supported() {
	s_status = usb::host::Status::DEVICE_NOT_SUPPORTED;
    puts("> Device not supported.");
}

usbh_user_status usbh_user_userinput() {
	puts("usbh_user_userinput");
    return static_cast<usbh_user_status>(1);
}

void usbh_user_over_current_detected() {
    puts("> Overcurrent detected.");
}

int usbh_usr_msc_application() {
	if  (usbh_usr_application_state == USBH_USR_FS_MOUNT) {
		const auto result = f_mount(&fat_fs, (const TCHAR *) "0:/", (BYTE) 0);

		if (result == FR_OK) {
			s_status = usb::host::Status::READY;
			usbh_usr_application_state = USBH_USR_FS_READY;
#if defined NODE_SHOWFILE
			showfile::usb_ready();
#endif
		} else {
			char buffer[32];
			snprintf(buffer, sizeof(buffer) - 1, "f_mount failed! %d\n", (int) result);
			console_error(buffer);
			return -1;
		}
	}

	return 0;
}
