/**
 * usb_host_msc.h
 *
 */
/* Copyright (C) 2023 by Arjan van Vught mailto:info@gd32-dmx.org
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

#ifndef USB_HOST_MSC_H_
#define USB_HOST_MSC_H_

#include "usbh_core.h"
#include "usb_conf.h"

/* function declarations */
/* user operation for host-mode initialization */
void usbh_user_init(void);
/* deinitialize user state and associated variables */
void usbh_user_deinit(void);
/* user operation for device attached */
void usbh_user_device_connected(void);
/* user operation for reset USB Device */
void usbh_user_device_reset(void);
/* user operation for device disconnect event */
void usbh_user_device_disconnected(void);
/* user operation for device over current detection event */
void usbh_user_over_current_detected(void);
/* user operation for detecting device speed */
void usbh_user_device_speed_detected(uint32_t DeviceSpeed);
/* user operation when device descriptor is available */
void usbh_user_device_desc_available(void *);
/* usb device is successfully assigned the address  */
void usbh_user_device_address_assigned(void);
/* user operation when configuration descriptor is available */
void usbh_user_configuration_descavailable(usb_desc_config *cfgDesc, usb_desc_itf *itfDesc, usb_desc_ep *epDesc);
/* user operation when manufacturer string exists */
void usbh_user_manufacturer_string(void *);
/* user operation when product string exists */
void usbh_user_product_string(void *);
/* user operation when serialNum string exists */
void usbh_user_serialnum_string(void *);
/* user response request is displayed to ask for application jump to class */
void usbh_user_enumeration_finish(void);
/* user action for application state entry */
usbh_user_status usbh_user_userinput(void);
/* user operation when device is not supported */
void usbh_user_device_not_supported(void);
/* user operation when unrecovered error happens */
void usbh_user_unrecovered_error(void);
/* demo application for mass storage */
int usbh_usr_msc_application(void);

#endif /* USB_HOST_MSC_H_ */
