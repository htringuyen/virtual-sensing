#include "zephyr/devicetree.h"
#include "zephyr/sys/util_macro.h"
#include "zephyr/usb/bos.h"
#include "zephyr/usb/usbd.h"
#include <stdint.h>
#include <string.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(sample_usbd_config, CONFIG_SAMPLE_USBD_LOG_LEVEL);

static const char *const blocklist[] = {
	"dfu_dfu",
	NULL,
};

// instantiate a device object
USBD_DEVICE_DEFINE(sample_usbd, DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)), 
				CONFIG_SAMPLE_USBD_VID, CONFIG_SAMPLE_USBD_PID);

// instantiate language desc
USBD_DESC_LANG_DEFINE(sample_lang);

// instatiate manufacturer desc
USBD_DESC_MANUFACTURER_DEFINE(sample_mfr, CONFIG_SAMPLE_USBD_MANUFACTURER);

// instantiate product desc
USBD_DESC_PRODUCT_DEFINE(sample_product, CONFIG_SAMPLE_USBD_PRODUCT);

// instantiate serial number desc
USBD_DESC_SERIAL_NUMBER_DEFINE(sample_sn);

// create usb bit attributes
static const uint8_t attributes = 
					(IS_ENABLED(CONFIG_SAMPLE_USBD_SELF_POWERED) ? USB_SCD_SELF_POWERED : 0)
					|
					(IS_ENABLED(CONFIG_SAMPLE_USBD_REMOTE_WAKEUP) ? USB_SCD_REMOTE_WAKEUP : 0);

// full speed configuration
USBD_DESC_CONFIG_DEFINE(fs_cfg_desc, "FS Configuration");
USBD_CONFIGURATION_DEFINE(sample_fs_config, attributes, CONFIG_SAMPLE_USBD_MAX_POWER, &fs_cfg_desc);

// high speed configuration
USBD_DESC_CONFIG_DEFINE(hs_cfg_desc, "HS Configuration");
USBD_CONFIGURATION_DEFINE(sample_hs_config, attributes, CONFIG_SAMPLE_USBD_MAX_POWER, &hs_cfg_desc);


/*
 * Configuration for USB 2.0
 */
 static const struct usb_bos_capability_lpm bos_cap_lpm = {
	.bLength = sizeof(struct usb_bos_capability_lpm),
	.bDescriptorType = USB_DESC_DEVICE_CAPABILITY,
	.bDevCapabilityType = USB_BOS_CAPABILITY_EXTENSION,
	.bmAttributes = 0UL
 };

 USBD_DESC_BOS_DEFINE(sample_usbext, sizeof(bos_cap_lpm), &bos_cap_lpm);

 static void sample_fix_code_triple(struct usbd_context *uds_ctx,
				   const enum usbd_speed speed)
{
	if (IS_ENABLED(CONFIG_USBD_CDC_ACM_CLASS) ||
	    IS_ENABLED(CONFIG_USBD_CDC_ECM_CLASS) ||
	    IS_ENABLED(CONFIG_USBD_CDC_NCM_CLASS) ||
	    IS_ENABLED(CONFIG_USBD_MIDI2_CLASS) ||
	    IS_ENABLED(CONFIG_USBD_AUDIO2_CLASS)) {
		
		usbd_device_set_code_triple(uds_ctx, speed,
					    USB_BCC_MISCELLANEOUS, 0x02, 0x01);
	} else {
		usbd_device_set_code_triple(uds_ctx, speed, 0, 0, 0);
	}
}


struct usbd_context *sample_usbd_setup_device(usbd_msg_cb_t msg_cb) {
	int err;

	/* add descriptors */
	err = usbd_add_descriptor(&sample_usbd, &sample_lang);
	if (err) {
		LOG_ERR("Failed to initialize language descriptor (%d)", err);
		return NULL;
	}

	err = usbd_add_descriptor(&sample_usbd, &sample_mfr);
	if (err) {
		LOG_ERR("Failed to initialize manufacturer descriptor (%d)", err);
		return NULL;
	}

	err = usbd_add_descriptor(&sample_usbd, &sample_product);
	if (err) {
		LOG_ERR("Failed to initialize product descriptor");
		return NULL;
	}

	err = usbd_add_descriptor(&sample_usbd, &sample_sn);
	if (err) {
		LOG_ERR("Failed to initialize serial number descriptor");
		return NULL;
	}

	if (usbd_caps_speed(&sample_usbd) == USBD_SPEED_HS) {
		err = usbd_add_configuration(&sample_usbd, USBD_SPEED_HS, &sample_hs_config);
		
		if (err) {
			LOG_ERR("Failed to add High-Speed configuration");
			return NULL;
		}

		err = usbd_register_all_classes(&sample_usbd, USBD_SPEED_HS, 1,
						blocklist);
		if (err) {
			LOG_ERR("Failed to add register classes");
			return NULL;
		}



		sample_fix_code_triple(&sample_usbd, USBD_SPEED_HS);
	}

	err = usbd_add_configuration(&sample_usbd, USBD_SPEED_FS, &sample_fs_config);
	if (err) {
		LOG_ERR("Failed to add Full-Speed configuration");
		return NULL;
	}

	// err = usbd_register_class(&sample_usbd, "cdc_acm_0", USBD_SPEED_FS, 1);
	// if (err) {
	// 	LOG_ERR("Failed to register CDC_ACM class");
	// 	return NULL;
	// }

	// err = usbd_register_class(&sample_usbd, "cdc_acm_1", USBD_SPEED_FS, 1);
	// if (err) {
	// 	LOG_ERR("Failed to register CDC_ACM class");
	// 	return NULL;
	// }


	err = usbd_register_all_classes(&sample_usbd, USBD_SPEED_FS, 1, blocklist);
	if (err) {
		LOG_ERR("Failed to add register classes");
		return NULL;
	}

	sample_fix_code_triple(&sample_usbd, USBD_SPEED_FS);

	usbd_self_powered(&sample_usbd, attributes & USB_SCD_SELF_POWERED);

	if (msg_cb != NULL) {
		err = usbd_msg_register_cb(&sample_usbd, msg_cb);
		if (err) {
			LOG_ERR("Failed to register message callback");
			return NULL;
		}
	}

	if (IS_ENABLED(CONFIG_SAMPLE_USBD_20_EXTENSION_DESC)) {
		(void)usbd_device_set_bcd_usb(&sample_usbd, USBD_SPEED_FS, 0x0201);
		(void)usbd_device_set_bcd_usb(&sample_usbd, USBD_SPEED_HS, 0x0201);

		err = usbd_add_descriptor(&sample_usbd, &sample_usbext);
		if (err) {
			LOG_ERR("Failed to add USB 2.0 Extension Descriptor");
			return NULL;
		}
	}
	return &sample_usbd;
}

struct usbd_context *sample_usbd_init_device(usbd_msg_cb_t msg_cb) {
	int err;

	if (sample_usbd_setup_device(msg_cb) == NULL) {
		return NULL;
	}

	err = usbd_init(&sample_usbd);
	if (err) {
		LOG_ERR("Failed to initialize device support");
		return NULL;
	}

	return &sample_usbd;
}





