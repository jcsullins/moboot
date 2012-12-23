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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <app.h>
#include <debug.h>
#include <platform.h>
#include <kernel/thread.h>
#include <target/gpiokeys.h>
#include <target/restart.h>
#include <dev/fbcon.h>
#include <dev/display.h>
#include <sys/types.h>
#include <lib/fs.h>
#include <lib/tga.h>
#include <lib/gfxconsole.h>
#include <lib/atags.h>
#include <lib/ramdisk.h>
#include <lib/bootlinux.h>

typedef struct menu_entry
{
	char *title;
	unsigned type;
	char *arg;
	char *name;
} menu_entry_t;

unsigned num_menu_entries;
menu_entry_t **entries;

gfx_surface *display_surface;
gfx_surface *splash_surface;
gfx_surface *background_surface;
gfx_surface *tile_surface;

unsigned gfx_trans;

#define BOOT_FS 1
#define BOOT_REBOOT 2
#define BOOT_RECOVER 3
#define BOOT_SHUTDOWN 4
#define BOOT_DFU 5

void boot_splash()
{
	unsigned splash_pos_x, splash_pos_y;

	if (splash_surface) {
		if (splash_surface->width > display_surface->width ||
				splash_surface->height > display_surface->height) {
			printf("ERROR: Splash image will not fit on display!\n");
			return;
		}
		gfxconsole_clear();
		splash_pos_x = (display_surface->width - splash_surface->width) / 2;
		splash_pos_y = (display_surface->height - splash_surface->height) / 2;
		gfx_surface_blend(display_surface, splash_surface, splash_pos_x,
							splash_pos_y);
	}
}

void show_background()
{
	unsigned pos_x, pos_y;

	if (background_surface) {
		if (background_surface->width > display_surface->width ||
				background_surface->height > display_surface->height) {
			printf("ERROR: Background image will not fit on display!\n");
			return;
		}
		gfxconsole_clear();
		pos_x = (display_surface->width - background_surface->width) / 2;
		pos_y = (display_surface->height - background_surface->height) / 2;
		gfx_surface_blend(display_surface, background_surface, pos_x,
							pos_y);
	}
}

int moboot_menu(unsigned x, unsigned y, menu_entry_t **entries, unsigned init, unsigned total, unsigned timeout)
{
	unsigned curpos, i, max, keys, is_pressed, has_timeout, countdown;
	time_t tick;
	unsigned wiggle_count = 0;

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
			gfxconsole_setpos(x, y + i);
			if (i == curpos) {
				if (tile_surface) {
					gfxconsole_set_colors(0x00000000, 0x000000ff);
					printf("%s", entries[i]->title);
					gfxconsole_set_colors(0x00000000, 0x00000000);
				} else {
					gfxconsole_set_colors(0x000000ff, 0x00000000);
					printf("%s", entries[i]->title);
					gfxconsole_set_colors(0x00000000, 0x000000ff);
				}
			} else {
				if (tile_surface) {
					gfxconsole_set_colors(0x00000000, 0x00000000);
					printf("%s", entries[i]->title);
				} else {
					gfxconsole_set_colors(0x00000000, 0x000000ff);
					printf("%s", entries[i]->title);
				}
			}
		}
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
				gfxconsole_setpos(x, y + total + 1);
				if (tile_surface) {
					gfxconsole_set_colors(0x00000000, 0x000000ff);
				} else {
					gfxconsole_set_colors(0x00000000, 0x00ff0000);
				}
				printf("timeout in %d     ", countdown);
				if (tile_surface) {
					gfxconsole_set_colors(0x00000000, 0x00000000);
				} else {
					gfxconsole_set_colors(0x00000000, 0x000000ff);
				}
			}


			thread_sleep(10);
			keys = gpiokeys_poll(KEY_ALL);
			if (has_timeout && keys) {
				gfxconsole_setpos(x, y + total + 1);
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

void set_menu_entry(char *title, unsigned type, char *arg, char *name)
{
	menu_entry_t *entryp = (menu_entry_t *)malloc(sizeof(menu_entry_t));

	entryp->title = title;
	entryp->type = type;
	entryp->arg = arg;
	entryp->name = name;

	entries[num_menu_entries++] = entryp;
}

void moboot_init(const struct app_descriptor *app)
{
	int rv, keys_pressed;
	unsigned act;
	menu_entry_t *real_entries[32];

	char *ptr;
	int rc;
	char path[256];
	char *newpath;
	char *newtitle;
	char *newname;

	unsigned xoff, yoff;

	unsigned ramdisk_mounted, ramdisk_start, ramdisk_size;

	unsigned i, j;

	unsigned default_menu_entry = 0;
	unsigned next_menu_entry = 0;

	char default_image[256];
	char next_image[256];

	unsigned default_timeout;
	unsigned counted_images;
	unsigned use_next;
	ssize_t splash_sz;
	void *splash_ptr = NULL;
	ssize_t background_sz;
	void *background_ptr;
	ssize_t tile_sz;
	void *tile_ptr;
	char splash_path[256];

	unsigned boot_flags;

	ssize_t rdmsgsz;
	char *rdmsg;

	keys_pressed = 0;

	display_surface = NULL;

	entries = real_entries;

	gfx_trans = 0;

	gfxconsole_clear();
	gfxconsole_setpos(0,0);

	ramdisk_mounted = 0;
	atags_get_ramdisk(&ramdisk_start, &ramdisk_size);
	if (ramdisk_size && ramdisk_start) {
		ramdisk_init((void*)ramdisk_start, ramdisk_size);
		if (fs_mount("/ramdisk", "/dev/ramdisk")) {
			printf("Ramdisk start=%08x size=%08x\n", ramdisk_start, ramdisk_size);
			printf("Unable to mount /ramdisk\n");
			printf("Press SELECT to continue\n");
			gpiokeys_wait_select();
		} else {
			ramdisk_mounted = 1;
		}
	}


    if (fs_mount("/boot", "/dev/mmcblk0p13")) {
		printf("\nUnable to mount /boot, exiting.\n");
		while (1) {
			thread_sleep(20);
		}
	}

	default_timeout = 5;
	if ((rv = fs_load_file("/boot/moboot.timeout", &default_image, 256)) > 0) {
		default_image[rv] = 0;
		if (default_image[rv - 1] == '\n') default_image[rv - 1] = 0;
		default_timeout = atoui(default_image);
	}
	default_image[0] = 0;
	rv = fs_load_file("/boot/moboot.default", &default_image, 256);
	if (rv > 0) {
		default_image[rv] = 0;
		if (default_image[rv - 1] == '\n') default_image[rv - 1] = 0;
	}
	use_next = 0;
	next_image[0] = 0;
	rv = fs_load_file("/boot/moboot.next", &next_image, 256);
	if (rv > 0) {
		next_image[rv] = 0;
		if (next_image[rv - 1] == '\n') next_image[rv - 1] = 0;
	}

	tile_sz = fs_load_file_mem("/boot/moboot.background.tga", &tile_ptr);

	background_surface = NULL;
	tile_surface = NULL;

	if (tile_sz > 0) {
		tile_surface = tga_decode(tile_ptr, tile_sz,
									GFX_FORMAT_RGB_x888);
		struct display_info disp_info;
		if (!display_surface) {
			display_get_info(&disp_info);
			display_surface = gfx_create_surface_from_display(&disp_info);
		}
		background_surface = gfx_create_surface(NULL,
								display_surface->width,
								display_surface->height,
								display_surface->stride,
								display_surface->format);
		for (i = 0; i < display_surface->width; i += tile_surface->width) {
			for (j = 0; j < display_surface->height;
							j += tile_surface->height) {
				gfx_surface_blend(background_surface,
									tile_surface,
									i, j);
			}
		}
	}


	num_menu_entries = 0;

	i = 0;
	counted_images = 0;
	while ((rc = fs_dirent("/boot", i, &ptr)) > 0) {
		sprintf(path, "/boot/%s", ptr);
		if (strncmp("uImage.", ptr, 7) == 0) {
			if (strncmp("uImage.moboot", ptr, 13) != 0) {
				newtitle = malloc(strlen(ptr) - 7 + 5 + 1);
				sprintf(newtitle, "boot %s", ptr + 7);
				newpath = malloc(strlen(ptr) + 6 + 1);
				sprintf(newpath, "/boot/%s", ptr);
				newname = malloc(strlen(ptr) - 7 + 1);
				sprintf(newname, "%s", ptr + 7);
				if (strcmp(default_image, ptr + 7) == 0) {
					default_menu_entry = num_menu_entries;
				}
				if (strcmp(next_image, ptr + 7) == 0) {
					next_menu_entry = num_menu_entries;
					use_next = 1;
				}
				set_menu_entry(newtitle, BOOT_FS, newpath, newname);
				counted_images++;
			}
		}
		free(ptr);
		i++;
	}
	if (rc < 0) {
		dprintf(SPEW, "/boot dirList ERROR rc = %d\n", rc);
	}
	i = 0;
	while ((rc = fs_dirent("/ramdisk/boot", i, &ptr)) > 0) {
		sprintf(path, "/ramdisk/boot/%s", ptr);
		if (strncmp("uImage.", ptr, 7) == 0) {
			if (strncmp("uImage.moboot", ptr, 13) != 0) {
				newtitle = malloc(strlen(ptr) - 7 + 5 + 1);
				sprintf(newtitle, "boot %s", ptr + 7);
				newpath = malloc(strlen(ptr) + 14 + 1);
				sprintf(newpath, "/ramdisk/boot/%s", ptr);
				newname = malloc(strlen(ptr) - 7 + 1);
				sprintf(newname, "%s", ptr + 7);
				if (strcmp(default_image, ptr + 7) == 0) {
					default_menu_entry = num_menu_entries;
				}
				if (strcmp(next_image, ptr + 7) == 0) {
					next_menu_entry = num_menu_entries;
					use_next = 1;
				}
				set_menu_entry(newtitle, BOOT_FS, newpath, newname);
				counted_images++;
			}
		}
		free(ptr);
		i++;
	}
	if (rc < 0) {
		dprintf(SPEW, "/ramdisk/boot dirList ERROR rc = %d\n", rc);
	}
	
	if (counted_images == 0) {
		set_menu_entry("boot", BOOT_FS, "/boot/uImage-2.6.35-palm-tenderloin", "default");
	}


	if (gpiokeys_poll(KEY_ALL)) {
		keys_pressed = 1;
		printf("\nPlease release key(s)...");
		while (1) {
			thread_sleep(20);
			if (!gpiokeys_poll(KEY_ALL)) {
				break;
			}
		}
	}

	gfx_trans = 0;
	if (tile_surface) {
		gfx_trans = 1;
	}


	set_menu_entry("boot webOS Recovery", BOOT_RECOVER, "", "recover");
	set_menu_entry("reboot", BOOT_REBOOT, "", "reboot");
	// set_menu_entry("DFU", BOOT_DFU, "", "");
	set_menu_entry("shutdown", BOOT_SHUTDOWN, "", "shutdown");

	xoff = (gfxconsole_getwidth() - 16 ) / 2;
	if (num_menu_entries < 10) {
		yoff = (gfxconsole_getheight() - 12) / 2;
	} else {
		yoff = (gfxconsole_getheight() - (num_menu_entries + 4)) / 2;
	}

#if 0
	tgasz = fs_load_file_mem("/boot/moboot.tga", &tgaptr);

	tga_surface = NULL;

	if (tgasz > 0) {
		tga_surface = tga_decode(tgaptr, tgasz, GFX_FORMAT_RGB_x888);
		struct display_info disp_info;
		if (!display_surface) {
			display_get_info(&disp_info);
			display_surface = gfx_create_surface_from_display(&disp_info);
		}
	}
#endif

	while (1) {
		gfxconsole_clear();

		show_background();

		if (background_surface) {
			gfxconsole_setbackground(background_surface);
		}

		gfxconsole_settrans(gfx_trans);

		gfxconsole_setpos(xoff,yoff);
		if (gfx_trans) {
			gfxconsole_set_colors(0xffffffff, 0x00000000);
			printf("moboot %s", MOBOOT_VERSION);
			gfxconsole_setpos(xoff,yoff);
			gfxconsole_set_colors(0x00000000, 0x00000000);
		} else {
			gfxconsole_set_colors(0x00000000, 0xffffffff);
			printf("moboot %s", MOBOOT_VERSION);
			gfxconsole_set_colors(0x00000000, 0x000000ff);
		}

		if (!use_next || keys_pressed) {
			act = moboot_menu(xoff, yoff + 2, entries, default_menu_entry, num_menu_entries, keys_pressed ? 0 : default_timeout);
		} else {
			act = next_menu_entry;
			use_next = 0;
		}

		keys_pressed = 1;

		gfxconsole_setpos(xoff, yoff + 2 + num_menu_entries + 2);

		boot_flags = BOOTLINUX_NOFLAGS;

		switch (entries[act]->type) {
			case BOOT_RECOVER:
				reboot_device(RESTART_REASON_RECOVER);
				break;
			case BOOT_REBOOT:
				reboot_device(RESTART_REASON_REBOOT);
				break;
			case BOOT_DFU:
				reboot_device(RESTART_REASON_DFU);
				break;
			case BOOT_SHUTDOWN:
				reboot_device(RESTART_REASON_SHUTDOWN);
				break;
			case BOOT_FS:
				gfxconsole_clear();
				gfxconsole_settrans(gfx_trans);
				show_background();
				gfxconsole_setpos(0,0);

				if (gfx_trans) {
					gfxconsole_set_colors(0x00000000, 0x00000000);
				} else {
					gfxconsole_set_colors(0x00000000, 0x000000ff);
				}

				printf("Selected: '%s'\n\n", entries[act]->title);
				printf("Loading '%s'... ", entries[act]->arg);
				if ((rv = fs_load_file(entries[act]->arg,
							(void *)SCRATCH_ADDR, 
							SCRATCH_SIZE * 1024 * 1024)) < 0) {
					printf("FAILED\n");
				} else {
					printf("OK\n");

					/* check for verbose boot */
					sprintf(splash_path, "/boot/moboot.verbose.%s", 
								entries[act]->name);

					splash_sz = fs_load_file_mem(splash_path, &splash_ptr);

					if (splash_sz > 0) {
						if (strncmp(splash_ptr, "yes", 3) == 0) {
							boot_flags |= BOOTLINUX_VERBOSE;
						}
					}

					/* check for sercon boot */
					sprintf(splash_path, "/boot/moboot.sercon.%s", 
								entries[act]->name);

					splash_sz = fs_load_file_mem(splash_path, &splash_ptr);

					if (splash_sz > 0) {
						if (strncmp(splash_ptr, "yes", 3) == 0) {
							boot_flags |= BOOTLINUX_SERCON;
						}
					}

					if (splash_ptr) free(splash_ptr);

					/* check for splash image */
					sprintf(splash_path, "/boot/moboot.splash.%s.tga", 
								entries[act]->name);

					splash_sz = fs_load_file_mem(splash_path, &splash_ptr);

					splash_surface = NULL;

					if (splash_sz > 0) {
						splash_surface = tga_decode(splash_ptr, splash_sz,
													GFX_FORMAT_RGB_x888);
						struct display_info disp_info;
						if (!display_surface) {
							display_get_info(&disp_info);
							display_surface = gfx_create_surface_from_display(
													&disp_info);
						}
					}

					/* do it to it! */
					bootlinux_uimage_mem((void *)SCRATCH_ADDR, rv, boot_splash,
							boot_flags);
				}

				gfxconsole_set_colors(0x00000000, 0x00ff0000);
				printf("\n\nBOOT FAILED!\n\nPress SELECT to continue\n");
				gfxconsole_set_colors(0x00000000, 0x000000ff);
				gpiokeys_wait_select();
				break;
		}
	}
}


#if 0
		char *ptr;
		int rc;
		unsigned i = 0;
		char path[256];
		filecookie fc;
		struct file_stat fstat;
		while ((rc = fs_dirent("/boot", i, &ptr)) > 0) {
			sprintf(path, "/boot/%s", ptr);
			fs_open_file(path, &fc);
			fs_stat_file(fc, &fstat);
			dprintf(SPEW, "Mode: %06hx ATIME: %08x CTIME: %08x MTIME: %08x NAME: '%s'\n",
					fstat.mode, fstat.atime, fstat.ctime, fstat.mtime, ptr);
			fs_close_file(fc);
			free(ptr);
			i++;
		}
		if (rc < 0) {
			dprintf(SPEW, "rc = %d\n", rc);
		}
#endif

APP_START(moboot)
	.init = moboot_init,
APP_END

