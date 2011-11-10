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
#include <lib/atags.h>
#include <string.h>
#include <malloc.h>
#include <debug.h>

char nada[] = "";

unsigned *passed_atags;

void init_passed_atags(unsigned *tags)
{
	passed_atags = tags;
}

void check_atags(unsigned *tags_ptr) {
	struct tag *tagp = (void *)tags_ptr;

	if (tagp->hdr.tag != ATAG_CORE) {
		return;
	}

	while (tagp->hdr.size != 0) {
		if (tagp->hdr.tag == ATAG_CORE) {
			//
		} else if (tagp->hdr.tag == ATAG_MEM) {
			dprintf(INFO, "ATAG_MEM: START: 0x%08x SIZE: 0x%08x\n", tagp->u.mem.start, tagp->u.mem.size);
		} else if (tagp->hdr.tag == ATAG_REVISION) {
			dprintf(INFO, "ATAG_REVISION: 0x%08x\n", tagp->u.revision.rev);
		} else if (tagp->hdr.tag == ATAG_CMDLINE) {
			dprintf(INFO, "ATAG_CMDLINE: '%s'\n", tagp->u.cmdline.cmdline);	
		} else {
			dprintf(INFO, "ATAG Unknown: 0x%08x\n", tagp->hdr.tag);
		}
		dprintf(INFO, "ATAG_SIZE[0x%08x]\n", tagp->hdr.size);
		if (tagp->hdr.size < 2048) {
			tagp = (struct tag*)((unsigned *)tagp + tagp->hdr.size);
		} else {
			dprintf(INFO, "ATAG parse aborted.\n");
			break;
		}

	}
}

char * atags_nduid(unsigned *tags_ptr) {
	struct tag *tagp = (void *)tags_ptr;

	unsigned j, i;

	if (tagp->hdr.tag != ATAG_CORE) {
		return 0;
	}

	while (tagp->hdr.size != 0) {
		if (tagp->hdr.tag == ATAG_CMDLINE) {
			i = 0;
			while (i + 7 < strlen(tagp->u.cmdline.cmdline)) {
				if (strncmp(((char *)(tagp->u.cmdline.cmdline) + i),
						"nduid=", 6) == 0) {
					for (j = i; j < strlen(tagp->u.cmdline.cmdline); j++) {
						if (*((char *)(tagp->u.cmdline.cmdline) + j) == ' ') {
							*((char *)(tagp->u.cmdline.cmdline) + j) = 0;
							return ((char *)(tagp->u.cmdline.cmdline) + i + 6);
						}
					}
				}
				i++;
			}
		}
		if (tagp->hdr.size < 2048) {
			tagp = (struct tag*)((unsigned *)tagp + tagp->hdr.size);
		} else {
			dprintf(INFO, "ATAG parse aborted.\n");
			break;
		}
	}
	return 0;
}

char * atags_get_cmdline_arg(unsigned *tags_ptr, const char *arg)
{
	struct tag *tagp = (void *)tags_ptr;
	unsigned arglen, i;
	char *cl = NULL;
	unsigned cllen;
	unsigned start, end;
	char *retp;

	if (!tagp) tagp = passed_atags;

	if (!tagp) return nada;
	if (!arg) return nada;

	if (tagp->hdr.tag != ATAG_CORE) {
		return nada;
	}

	arglen = strlen(arg);

	if (!arglen) return nada;

	while (tagp->hdr.size != 0) {
		if (tagp->hdr.tag == ATAG_CMDLINE) {
			cl = tagp->u.cmdline.cmdline;
			break;
		}

		if (tagp->hdr.size < 2048) {
			tagp = (struct tag*)((unsigned *)tagp + tagp->hdr.size);
		} else {
			break;
		}
	}

	if (!cl) return nada;

	cllen = strlen(cl);

	i = 0;
	start = 0;
	end = 0;
	while (i + arglen <= cllen) {
		if (strncmp(cl + i, arg, arglen) == 0) {
			if (i + arglen == cllen || cl[i + arglen] == ' ') {
				start = i;
				end = i + arglen - 1;
				break;
			}
			if (cl[i + arglen] != '=') {
				while (i < cllen && cl[i] != ' ') i++;
				while (i < cllen && cl[i] == ' ') i++;
				continue;
			}
			start = i;
			while (i < cllen && cl[i] != ' ') i++;
			end = i - 1;
			break;
		} else {
			if (cl[i] != ' ') {
				while (i < cllen && cl[i] != ' ') i++;
			}
			while (i < cllen && cl[i] == ' ') i++;
		}
	}

	if (end == 0) return nada;

	retp = (char *)malloc(end - start + 3);

	retp[0] = ' ';
	for (i = 0; i <= end - start; i++) {
		retp[i + 1] = cl[start + i];
	}
	retp[i + 1] = '\0';

	return retp;
}
		
void atags_get_ramdisk(unsigned *addr, unsigned *length)
{
	struct tag *tagp = (void *)passed_atags;

	*addr = 0;
	*length = 0;

	if (!tagp || tagp->hdr.tag != ATAG_CORE) {
		return;
	}

	while (tagp->hdr.size != 0) {
		if (tagp->hdr.tag == ATAG_INITRD2) {
			*addr = tagp->u.initrd.start;
			*length = tagp->u.initrd.size;
			break;
		}

		if (tagp->hdr.size < 2048) {
			tagp = (struct tag*)((unsigned *)tagp + tagp->hdr.size);
		} else {
			break;
		}
	}

	return;
}
		
