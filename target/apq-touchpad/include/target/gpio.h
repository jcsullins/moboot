/*
 * Copyright (c) 2011, James Sullins
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the 
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED 
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _GPIO_H
#define _GPIO_H

#define BIT(nr)			(1UL << (nr))

#define NR_MSM_GPIOS 173
#define ARCH_NR_GPIOS 512

#define PM8058_GPIO_BASE			NR_MSM_GPIOS
#define PM8058_GPIO_PM_TO_SYS(pm_gpio)		(pm_gpio + PM8058_GPIO_BASE)

#define MSM_TLMM_BASE 0x00800000
#define KEY_UP_GPIO				103
#define KEY_DOWN_GPIO			104
#define KEY_SELECT_GPIO			40
#define KEY_UP_GPIO_3G			(PM8058_GPIO_PM_TO_SYS(6-1))
#define KEY_DOWN_GPIO_3G			(PM8058_GPIO_PM_TO_SYS(7-1))

#define GPIO_CONFIG(gpio)         (MSM_TLMM_BASE + 0x1000 + (0x10 * (gpio)))
#define GPIO_IN_OUT(gpio)         (MSM_TLMM_BASE + 0x1004 + (0x10 * (gpio)))

#define GPIO_IN_BIT		0
#define GPIO_OUT_BIT	1
#define GPIO_OE_BIT		9

static inline void apq_set_gpio_bits(unsigned n, void *addr)
{
	writel(readl(addr) | n, addr);
}

static inline void apq_clear_gpio_bits(unsigned n, void *addr)
{
	writel(readl(addr) & ~n, addr);
}

extern int apq_gpio_get(unsigned gpio);
extern void apq_gpio_set(unsigned gpio, int val);
extern void apq_gpio_direction_input(unsigned gpio);

#endif
