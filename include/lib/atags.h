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


#ifndef ATAGS_H
#define ATAGS_H

typedef unsigned char __u8;
typedef unsigned char __u16;

#define ATAG_NONE	0x00000000

struct tag_header {
	unsigned size;
	unsigned tag;
};

#define ATAG_CORE	0x54410001

struct tag_core {
	unsigned flags;
	unsigned pagesize;
	unsigned rootdev;
};

#define ATAG_MEM	0x54410002
#define ATAG_MEM_RESERVED	0x5441000A
#define ATAG_MEM_LOW_POWER	0x5441000B
#define ATAG_MEM_OSBL		0x5441000C

struct tag_mem32 {
	unsigned	size;
	unsigned	start;
};

#define ATAG_VIDEOTEXT	0x54410003

struct tag_videotext {
	__u8		x;
	__u8		y;
	__u16		video_page;
	__u8		video_mode;
	__u8		video_cols;
	__u16		video_ega_bx;
	__u8		video_lines;
	__u8		video_isvga;
	__u16		video_points;
};

#define ATAG_RAMDISK	0x54410004

struct tag_ramdisk {
	unsigned flags;
	unsigned size;
	unsigned start;
};

/* virtual addrs */
#define ATAG_INITRD	0x54410005
/* physical addrs */
#define ATAG_INITRD2	0x54420005

struct tag_initrd {
	unsigned start;
	unsigned size;
};

#define ATAG_SERIAL	0x54410006

struct tag_serialnr {
	unsigned low;
	unsigned high;
};

#define ATAG_REVISION	0x54410007

struct tag_revision {
	unsigned rev;
};

#define ATAG_VIDEOLFB	0x54410008

struct tag_videolfb {
	__u16		lfb_width;
	__u16		lfb_height;
	__u16		lfb_depth;
	__u16		lfb_linelength;
	unsigned		lfb_base;
	unsigned		lfb_size;
	__u8		red_size;
	__u8		red_pos;
	__u8		green_size;
	__u8		green_pos;
	__u8		blue_size;
	__u8		blue_pos;
	__u8		rsvd_size;
	__u8		rsvd_pos;
};

#define ATAG_CMDLINE	0x54410009

struct tag_cmdline {
	char	cmdline[1];
};

#define ATAG_ACORN	0x41000101

struct tag_acorn {
	unsigned memc_control_reg;
	unsigned vram_pages;
	__u8 sounddefault;
	__u8 adfsdrives;
};

#define ATAG_MEMCLK	0x41000402

struct tag_memclk {
	unsigned fmemclk;
};



struct tag {
	struct tag_header hdr;
	union {
		struct tag_core		core;
		struct tag_mem32	mem;
		struct tag_videotext	videotext;
		struct tag_ramdisk	ramdisk;
		struct tag_initrd	initrd;
		struct tag_serialnr	serialnr;
		struct tag_revision	revision;
		struct tag_videolfb	videolfb;
		struct tag_cmdline	cmdline;

		struct tag_acorn	acorn;

		struct tag_memclk	memclk;
	} u;
};

extern char * atags_nduid(unsigned *tags_ptr);
extern char * atags_get_cmdline_arg(unsigned *tags_ptr, const char *arg);
extern void init_passed_atags(unsigned *tags);

#endif

