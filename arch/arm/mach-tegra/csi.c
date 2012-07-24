/*
 * arch/arm/mach-tegra/csi.c
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

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>

#include <mach/iomap.h>
#include <mach/csi.h>

#include "clock.h"

static struct clk *vi_clk;
static struct clk *csi_clk;

int tegra_vi_csi_enable_clocks(void)
{
	if (vi_clk == NULL) {
		vi_clk = tegra_get_clock_by_name("vi");
		if (IS_ERR_OR_NULL(vi_clk)) {
			pr_err("vi: can't get vi clock\n");
			return -EINVAL;
		}
	}
	clk_enable(vi_clk);

	if (csi_clk == NULL) {
		csi_clk = tegra_get_clock_by_name("csi");
		if (IS_ERR_OR_NULL(csi_clk)) {
			pr_err("csi: can't get csi clock\n");
			return -EINVAL;
		}
	}
	clk_enable(csi_clk);

	return 0;
}

void tegra_vi_csi_disable_clocks(void)
{
	clk_disable(csi_clk);
	clk_disable(vi_clk);
}

int tegra_vi_csi_writel(u32 val, u32 offset)
{
	int ret;

	ret = tegra_vi_csi_enable_clocks();
	if(ret)
	  return ret;
	writel(val, IO_TO_VIRT(TEGRA_VI_BASE) + offset * 4);
	tegra_vi_csi_disable_clocks();
	return 0;
}

int tegra_vi_csi_readl(u32 offset, u32 *val)
{
	int ret;

	ret = tegra_vi_csi_enable_clocks();
	if(ret)
	  return ret;

	*val = readl(IO_TO_VIRT(TEGRA_VI_BASE) + offset * 4);

	tegra_vi_csi_disable_clocks();

	return 0;
}
