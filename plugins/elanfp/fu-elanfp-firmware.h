/*
 * Copyright (C) 2021 Michael Cheng <michael.cheng@emc.com.tw>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#pragma once

#include <fwupdplugin.h>

#define FU_TYPE_ELANFP_FIRMWARE (fu_elanfp_firmware_get_type())
G_DECLARE_FINAL_TYPE(FuElanfpFirmware, fu_elanfp_firmware, FU, ELANFP_FIRMWARE, FuFirmware)

#define FW_SET_ID_OFFER_A   "offer_A"
#define FW_SET_ID_OFFER_B   "offer_B"
#define FW_SET_ID_PAYLOAD_A "payload_A"
#define FW_SET_ID_PAYLOAD_B "payload_B"

typedef struct _S2F_FILE {
	guint8 Tag[2][2];
	const guint8 *pOffer[2];
	const guint8 *pPayload[2];
	gsize OfferLength[2];
	gsize PayloadLength[2];
} S2F_FILE;

typedef struct _PAYLOAD_HEADER {
	guint32 Address;
	guint8 Length;
} PAYLOAD_HEADER;

FuFirmware *
fu_elanfp_firmware_new(void);
