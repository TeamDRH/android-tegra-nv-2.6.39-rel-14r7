/*
 * arch/arm/mach-tegra/include/mach/csi.h
 *
 * Copyright (C) 2010-2011 NVIDIA Corporation.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __MACH_TEGRA_CSI_H
#define __MACH_TEGRA_CSI_H

/*** Input interfaces ***/
#define CSI_VI_INPUT_STREAM_CONTROL_0 0x200
/* Always set to zero on Tegra2? */
#define VIP_SF_GEN(x) (((x) & 0x1) << 7)

#define CSI_HOST_INPUT_STREAM_CONTROL_0 0x202

#define CSI_INPUT_STREAM_A_CONTROL_0 0x204

#define CSI_INPUT_STREAM_B_CONTROL_0 0x20f

/*** PP A registers ***/
#define CSI_PIXEL_STREAM_A_CONTROL0_0 0x206
#define CSI_PPA_PAD_FRAME(x) (((x) & 0x3) << 28)
#define CSI_PPA_HEADER_EC_ENABLE(x) (((x) & 0x1) << 27)
#define CSI_PPA_PAD_SHORT_LINE(x) (((x) & 0x3) << 24)
#define CSI_PPA_EMBEDDED_DATA_OPTIONS(x) (((x) & 0x3) << 20)
#define CSI_PPA_OUTPUT_FORMAT_OPTIONS(x) (((x) & 0xf) << 16)
#define OUTPUT_FORMAT_ARBITRARY 0
#define OUTPUT_FORMAT_PIXEL 1
#define OUTPUT_FORMAT_PIXEL_REP 2
#define OUTPUT_FORMAT_STORE 3
#define CSI_PPA_VIRTUAL_CHANNEL_ID(x) (((x) & 0x3) << 14)
#define CSI_PPA_DATA_TYPE(x) (((x) & 0x3f) << 8)
#define DATA_TYPE_YUV420_8 24
#define DATA_TYPE_YUV420_10 25
#define DATA_TYPE_LED_YUV420_8 26
#define DATA_TYPE_YUV420CSPS_8 27
#define DATA_TYPE_YUV420CSPS_10 28
#define DATA_TYPE_YUV422_8 29
#define DATA_TYPE_YUV422_10 30
#define DATA_TYPE_RGB444 32
#define DATA_TYPE_RGB555 33
#define DATA_TYPE_RGB565 34
#define DATA_TYPE_RGB666 35
#define DATA_TYPE_RGB888 36
#define DATA_TYPE_RAW6 40
#define DATA_TYPE_RAW7 41
#define DATA_TYPE_RAW8 42
#define DATA_TYPE_RAW10 43
#define DATA_TYPE_RAW12 44
#define DATA_TYPE_RAW14 45
#define CSI_PPA_CRC_CHECK(x) (((x) & 0x1) << 7)
#define CSI_PPA_WORD_COUNT_SELECT(x) (((x) & 0x1) << 6)
#define CSI_PPA_DATA_IDENTIFIER(x) (((x) & 0x1) << 5)
#define CSI_PPA_PACKET_HEADER(x) (((x) & 0x1) << 4)
#define CSI_PPA_STREAM_SOURCE(x) (((x) & 0x7) << 0)
#define STREAM_SOURCE_CSI_A 0
#define STREAM_SOURCE_CSI_B 1
#define STREAM_SOURCE_VI_PORT 6
#define STREAM_SOURCE_HOST 7

#define CSI_PIXEL_STREAM_A_CONTROL1_0 0x207

#define CSI_PIXEL_STREAM_A_WORD_COUNT_0 0x208

#define CSI_PIXEL_STREAM_A_GAP_0 0x209

#define CSI_PIXEL_STREAM_PPA_COMMAND_0 0x20a
#define CSI_PPA_START_MARKER_FRAME_MAX(x) (((x) & 0xf) << 12)
#define CSI_PPA_START_MARKER_FRAME_MIN(x) (((x) & 0xf) << 8)
#define CSI_PPA_VSYNC_START_MARKER(x) (((x) & 0x1) << 4)
#define CSI_PPA_SINGLE_SHOT(x) (((x) & 0x1) << 2)
#define CSI_PPA_ENABLE(x) (((x) & 0x3) << 0)
#define CSI_ENABLE 1
#define CSI_DISABLE 2
#define CSI_RST 3

/*** PP B registers ***/
#define CSI_PIXEL_STREAM_B_CONTROL0_0 0x211

#define CSI_PIXEL_STREAM_B_CONTROL1_0 0x212

#define CSI_PIXEL_STREAM_B_WORD_COUNT_0 0x213

#define CSI_PIXEL_STREAM_B_GAP_0 0x214

#define CSI_PIXEL_STREAM_PPB_COMMAND_0 0x215

/*** CIL ***/
#define CSI_PHY_CIL_COMMAND_0 0x21a
#define CSI_B_PHY_CIL_ENABLE(x) (((x) & 0x3) << 16)
#define CSI_A_PHY_CIL_ENABLE(x) (((x) & 0x3) << 0)

#define CSI_PHY_CILA_CONTROL0_0 0x21b

#define CSI_PHY_CILB_CONTROL0_0 0x21c

#define CSI_CSI_PIXEL_PARSER_STATUS_0 0x21e

#define CSI_CSI_CIL_STATUS_0 0x21f

#define CSI_CSI_PIXEL_PARSER_INTERRUPT_MASK_0 0x220

#define CSI_CSI_CIL_INTERRUPT_MASK_0 0x221

#define CSI_CSI_READONLY_STATUS_0 0x220

#define CSI_ESCAPE_MODE_COMMAND_0 0x223

#define CSI_ESCAPE_MODE_DATA_0 0x224

#define CSI_CILA_PAD_CONFIG0_0 0x225

#define CSI_CILA_PAD_CONFIG1_0 0x226

#define CSI_CILB_PAD_CONFIG0_0 0x227

#define CSI_CILB_PAD_CONFIG1_0 0x228

#define CSI_CIL_PAD_CONFIG0_0 0x229

#define CSI_CILA_MIPI_CAL_CONFIG_0 0x22a
#define  MIPI_CAL_TERMOSA(x)		(((x) & 0x1f) << 0)

#define CSI_CILB_MIPI_CAL_CONFIG_0 0x22b
#define  MIPI_CAL_TERMOSB(x)		(((x) & 0x1f) << 0)

#define CSI_CIL_MIPI_CAL_STATUS_0 0x22c

#define CSI_DSI_MIPI_CAL_CONFIG	0x234
#define  MIPI_CAL_HSPDOSD(x)		(((x) & 0x1f) << 16)
#define  MIPI_CAL_HSPUOSD(x)		(((x) & 0x1f) << 8)

#define CSI_MIPIBIAS_PAD_CONFIG	0x235
#define  PAD_DRIV_DN_REF(x)		(((x) & 0x7) << 16)
#define  PAD_DRIV_UP_REF(x)		(((x) & 0x7) << 8)

int tegra_vi_csi_enable_clocks(void);
void tegra_vi_csi_disable_clocks(void);

int tegra_vi_csi_readl(u32 offset, u32 *val);
int tegra_vi_csi_writel(u32 value, u32 offset);

#endif
