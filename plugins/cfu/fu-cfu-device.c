/*
 * Copyright (C) 2021 Richard Hughes <richard@hughsie.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#include "config.h"

#include <fwupdplugin.h>

#include "fu-cfu-device.h"
#include "fu-cfu-module.h"

struct _FuCfuDevice {
	FuHidDevice parent_instance;
	guint8 protocol_version;
};

G_DEFINE_TYPE(FuCfuDevice, fu_cfu_device, FU_TYPE_HID_DEVICE)

#define FU_CFU_DEVICE_TIMEOUT 5000 /* ms */
#define FU_CFU_FEATURE_SIZE   60   /* bytes */

#define FU_CFU_CMD_GET_FIRMWARE_VERSION 0x00

static void
fu_cfu_device_to_string(FuDevice *device, guint idt, GString *str)
{
	FuCfuDevice *self = FU_CFU_DEVICE(device);

	/* FuUdevDevice->to_string */
	FU_DEVICE_CLASS(fu_cfu_device_parent_class)->to_string(device, idt, str);

	fu_common_string_append_kx(str, idt, "ProtocolVersion", self->protocol_version);
}

static gboolean
fu_cfu_module_write_firmware(FuDevice *device,
			     FuFirmware *firmware,
			     FuProgress *progress,
			     FwupdInstallFlags flags,
			     GError **error)
{
	g_set_error(error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED, "not supported");
	return FALSE;
}

static gboolean
fu_cfu_device_setup(FuDevice *device, GError **error)
{
	FuCfuDevice *self = FU_CFU_DEVICE(device);
	guint8 buf[FU_CFU_FEATURE_SIZE] = {0};
	guint8 component_cnt = 0;
	guint8 tmp = 0;
	gsize offset = 0;
	g_autoptr(GHashTable) modules_by_cid = NULL;

	/* FuUsbDevice->setup */
	if (!FU_DEVICE_CLASS(fu_cfu_device_parent_class)->setup(device, error))
		return FALSE;

	/* get version */
	if (!fu_hid_device_get_report(FU_HID_DEVICE(device),
				      FU_CFU_CMD_GET_FIRMWARE_VERSION,
				      buf,
				      sizeof(buf),
				      FU_CFU_DEVICE_TIMEOUT,
				      FU_HID_DEVICE_FLAG_IS_FEATURE,
				      error)) {
		return FALSE;
	}
	if (!fu_common_read_uint8_safe(buf, sizeof(buf), 0x0, &component_cnt, error))
		return FALSE;
	if (!fu_common_read_uint8_safe(buf, sizeof(buf), 0x3, &tmp, error))
		return FALSE;
	self->protocol_version = tmp & 0b1111;

	/* keep track of all modules so we can work out which are dual bank */
	modules_by_cid = g_hash_table_new(g_int_hash, g_int_equal);

	/* read each component module version */
	offset += 4;
	for (guint i = 0; i < component_cnt; i++) {
		g_autoptr(FuCfuModule) module = fu_cfu_module_new(device);
		FuCfuModule *module_tmp;

		if (!fu_cfu_module_setup(module, buf, sizeof(buf), offset, error))
			return FALSE;
		fu_device_add_child(device, FU_DEVICE(module));

		/* same module already exists, so mark both as being dual bank */
		module_tmp =
		    g_hash_table_lookup(modules_by_cid,
					GINT_TO_POINTER(fu_cfu_module_get_component_id(module)));
		if (module_tmp != NULL) {
			fu_device_add_flag(FU_DEVICE(module), FWUPD_DEVICE_FLAG_DUAL_IMAGE);
			fu_device_add_flag(FU_DEVICE(module_tmp), FWUPD_DEVICE_FLAG_DUAL_IMAGE);
		} else {
			g_hash_table_insert(modules_by_cid,
					    GINT_TO_POINTER(fu_cfu_module_get_component_id(module)),
					    module);
		}

		/* done */
		offset += 0x8;
	}

	/* success */
	return TRUE;
}

static void
fu_cfu_device_init(FuCfuDevice *self)
{
	fu_device_add_flag(FU_DEVICE(self), FWUPD_DEVICE_FLAG_USABLE_DURING_UPDATE);
}

static void
fu_cfu_device_class_init(FuCfuDeviceClass *klass)
{
	FuDeviceClass *klass_device = FU_DEVICE_CLASS(klass);
	klass_device->setup = fu_cfu_device_setup;
	klass_device->to_string = fu_cfu_device_to_string;
	klass_device->write_firmware = fu_cfu_module_write_firmware;
}
