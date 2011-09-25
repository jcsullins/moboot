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

#include <app.h>
#include <debug.h>
#include <target/gpiokeys.h>
#include <target/restart.h>
#include <dev/fbcon.h>
#include <sys/types.h>


int moboot_menu(unsigned x, unsigned y, char **entries, unsigned init, unsigned total, unsigned timeout)
{
	unsigned curpos, i, max, keys, is_pressed, has_timeout, countdown;
	time_t tick;

	max = total - 1;
	curpos = init;

	if (timeout) {
		has_timeout = 1;
		countdown = timeout;
		tick = current_time() + 1000;
	} else {
		has_timeout = 0;
	}

	while (1) {
		for (i = 0; i < total; i++) {
			if (i == curpos) {
				fbcon_set_colors(0, 0, 255, 0, 0, 0);
			} else {
				fbcon_set_colors(0, 0, 0, 0, 0, 255);
			}
			fbcon_setpos(x, y + i);
			printf("%s", entries[i]);
		}
		fbcon_set_colors(0, 0, 0, 0, 0, 255);
		is_pressed = gpiokeys_poll(KEY_ALL);
		while (1) {
			if (has_timeout) {
				if (current_time() > tick) {
					tick += 1000;
					countdown--;
					if (!countdown) {
						return curpos;
					}
				}
				fbcon_setpos(x, y + total + 1);
				printf("timeout in %d     ", countdown);
			}
			thread_sleep(20);
			keys = gpiokeys_poll(KEY_ALL);
			if (has_timeout && keys) {
				fbcon_setpos(x, y + total + 1);
				printf("                         ");
				has_timeout = 0;
			}
			if (is_pressed && !keys) {
				if (is_pressed & KEY_SELECT) {
					return curpos;
				}
				if (is_pressed & KEY_UP) {
					if (curpos == 0) {
						curpos = max;
					} else {
						curpos--;
					}
				}
				if (is_pressed & KEY_DOWN) {
					if (curpos == max) {
						curpos = 0;
					} else {
						curpos++;
					}
				}
				break;
			} else if (keys) {
				is_pressed = keys;
			}
		}
	}
}

void moboot_init(const struct app_descriptor *app)
{
	int rv, keys_pressed;
	unsigned act;
	char *entries[10];

	keys_pressed = 0;

	if (gpiokeys_poll(KEY_ALL)) {
		keys_pressed = 1;
		printf("\nPlease release key(s)...\n");
		while (1) {
			thread_sleep(20);
			if (!gpiokeys_poll(KEY_ALL)) {
				break;
			}
		}
	}

	fbcon_clear();
	fbcon_setpos(0,0);
	printf("welcome to moboot\n");

	entries[0] = "Recover";
	entries[1] = "Reboot";
	entries[2] = "Shutdown";

	act = moboot_menu(0, 2, entries, 0, 3, keys_pressed ? 0 : 9);

	fbcon_setpos(0, 2 + 3 + 2); /* y_start + num_entries + 2 */

	if (act == 0) {
		reboot_device(RESTART_REASON_RECOVER);
	} else if (act == 1) {
		reboot_device(RESTART_REASON_REBOOT);
	} else if (act == 2) {
		reboot_device(RESTART_REASON_SHUTDOWN);
	}

}

APP_START(moboot)
	.init = moboot_init,
APP_END

