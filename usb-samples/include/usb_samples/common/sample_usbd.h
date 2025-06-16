#ifndef ZEPHYR_VTSS_USB_COMMON_SAMPLE_USBD_H
#define ZEPHYR_VTSS_USB_COMMON_SAMPLE_USBD_H

#include <zephyr/usb/usbd.h>

/*
* Setup and init usb device
*/
struct usbd_context *sample_usbd_init_device(usbd_msg_cb_t msg_cb);

/*
* Setup the usb device, initialization will be done by user
*/
struct usbd_context *sample_usbd_setup_device(usbd_msg_cb_t msg_cb);

#endif