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

#include <target/gpio.h>
#include <target/gpiokeys.h>
#include <kernel/thread.h>

int gpiokeys_poll(unsigned keylist) {

	int keys_result = 0;

#ifndef IS_TOUCHPAD_3G
	if (keylist & KEY_UP) {
		if (apq_gpio_get(KEY_UP_GPIO) == 0) {
			keys_result += KEY_UP;
		}
	}

	if (keylist & KEY_DOWN) {
		if (apq_gpio_get(KEY_DOWN_GPIO) == 0) {
			keys_result += KEY_DOWN;
		}
	}
#else
	if (keylist & KEY_UP) {
		if (!pm8058_gpio_get(5)) {
			keys_result += KEY_UP;
		}
	}

	if (keylist & KEY_DOWN) {
		if (!pm8058_gpio_get(6)) {
			keys_result += KEY_DOWN;
		}
	}
#endif

	if (keylist & KEY_SELECT) {
		if (apq_gpio_get(KEY_SELECT_GPIO) == 0) {
			keys_result += KEY_SELECT;
		}
	}

	return keys_result;
}


void gpiokeys_wait_select()
{
	unsigned keys, is_pressed;

	while (gpiokeys_poll(KEY_ALL)) {
		thread_sleep(20);
	}

	is_pressed = gpiokeys_poll(KEY_ALL);
	while (1) {
		thread_sleep(20);
		keys = gpiokeys_poll(KEY_ALL);
		if (is_pressed && !keys) {
			if (is_pressed & KEY_SELECT) {
				return;
			}
		}
		is_pressed = keys;
	}
}

bool usbhost_poll()
{
	return(pm8058_gpio_get(36));
}

