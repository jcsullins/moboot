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

#include <debug.h>
#include <arch/arm.h>
#include <uboot/image.h>
#include <endian.h>
#include <zlib.h>

enum UIMAGE_VALIDITY 
{
	UIMAGE_VALID,
	UIMAGE_INVALID_MAGIC,
	UIMAGE_INVALID_HCRC,
	UIMAGE_INVALID_DCRC,
	UIMAGE_INVALID_SIZE,
	UIMAGE_INVALID_MULTI_SIZES
};

uint8_t uimage_type(struct image_header *hdr)
{
	return hdr->ih_type;
}

uint32_t uimage_size(struct image_header *hdr)
{
	return ntohl(hdr->ih_size);
}

uint32_t uimage_time(struct image_header *hdr)
{
	return ntohl(hdr->ih_time);
}

uint32_t uimage_load(struct image_header *hdr)
{
	return ntohl(hdr->ih_load);
}

uint32_t uimage_ep(struct image_header *hdr)
{
	return ntohl(hdr->ih_ep);
}

uint8_t uimage_os(struct image_header *hdr)
{
	return hdr->ih_os;
}

uint8_t uimage_arch(struct image_header *hdr)
{
	return hdr->ih_arch;
}

uint8_t uimage_comp(struct image_header *hdr)
{
	return hdr->ih_comp;
}

uint8_t * uimage_name(struct image_header *hdr)
{
	unsigned i;

	for (i = 0; i < (IH_NMLEN - 1); i++) {
		if (hdr->ih_name[i] == 0) break;
	}
	hdr->ih_name[i] = 0;

	return hdr->ih_name;
}

unsigned uimage_valid_magic(void *data)
{
	struct image_header *hdr = (struct image_header *)data;

	if (ntohl(hdr->ih_magic) != IH_MAGIC) {
		return 0;
	} else {
		return 1;
	}
}

unsigned uimage_valid_hcrc(struct image_header *hdr)
{
	uint32_t hcrc = hdr->ih_hcrc;
	unsigned is_valid = 1;

	hdr->ih_hcrc = 0;
	if (crc32(0, (unsigned char *)hdr, sizeof(struct image_header)) != ntohl(hcrc)) {
		is_valid = 0;
	}
	hdr->ih_hcrc = hcrc;
	return is_valid;
}


unsigned uimage_valid_dcrc(struct image_header *hdr)
{
	unsigned checksum = ntohl(hdr->ih_dcrc);

	/* TODO - check size is within bounds */
	if (crc32(0, ((unsigned char *)hdr + sizeof(struct image_header)),
					ntohl(hdr->ih_size)) != checksum) {
		return 0;
	} else {
		return 1;
	}
}

unsigned uimage_valid_size(struct image_header *hdr, unsigned len)
{
	if (ntohl(hdr->ih_size) != len - sizeof(struct image_header)) {
		dprintf(ALWAYS, "size %u != len %u\n", ntohl(hdr->ih_size), len);
		return 0;
	}

	return 1;
}

unsigned uimage_valid_multi_sizes(struct image_header *hdr, unsigned len)
{
	/* TODO */
	return 1;
}

void * uimage_data(struct image_header *hdr)
{
	return ((char *)hdr + sizeof(struct image_header));
}

unsigned uimage_multi_count(struct image_header *hdr)
{
	uint32_t *ptr = (uint32_t *)((void *)hdr + sizeof(struct image_header));
	int count = 0;

	while (ntohl(*ptr)) {
		count++;
		ptr++;
	}

	return count;
}

unsigned uimage_count(struct image_header *hdr)
{
	if (uimage_type(hdr) == IH_TYPE_MULTI) {
		return uimage_multi_count(hdr);
	} else {
		return 1;
	}
}

struct image_header* uimage_multi_image(struct image_header *hdr,
		unsigned index)
{
	unsigned i;
	unsigned skip = sizeof(struct image_header);

	skip += sizeof(uint32_t) * (uimage_multi_count(hdr) + 1);

	for (i = 0; i < index - 1; i++) {
		skip += ntohl(*(uint32_t *)((char *)hdr + sizeof(struct image_header)
					+ sizeof(uint32_t) * i));
	}

	return ((struct image_header*)((char *)hdr + skip));
}

struct image_header* uimage_image(struct image_header *hdr, unsigned index)
{
	if (uimage_type(hdr) == IH_TYPE_MULTI) {
		return uimage_multi_image(hdr, index);
	} else {
		return hdr;
	}
}

uint32_t uimage_multi_size(struct image_header *hdr, unsigned index)
{
	uint32_t *ptr = (uint32_t *)(hdr + 1);
	ptr += (index - 1);
	return ntohl(*ptr);
}

unsigned uimage_invalid(void *data, unsigned len)
{
	struct image_header *hdr = (struct image_header *)data;

	if (!uimage_valid_magic(data)) {
		return UIMAGE_INVALID_MAGIC;
	}
	if (!uimage_valid_hcrc(hdr)) {
		return UIMAGE_INVALID_HCRC;
	}
	if (!uimage_valid_dcrc(hdr)) {
		return UIMAGE_INVALID_DCRC;
	}
	if (!uimage_valid_size(hdr, len)) {
		return UIMAGE_INVALID_SIZE;
	}
	if (hdr->ih_type == IH_TYPE_MULTI) {
		if (!uimage_valid_multi_sizes(hdr, len)) {
			return UIMAGE_INVALID_MULTI_SIZES;
		}
	}
	return 0;
}

unsigned uimage_check(void *data, unsigned len)
{
	struct image_header *hdr;
	struct image_header *subhdr;
	unsigned i, count, sublen;
	int rc;

	if (rc = uimage_invalid(data, len)) {
		switch(rc) {
			case UIMAGE_INVALID_MAGIC:
				printf("Invalid Magic\n");
				break;
			case UIMAGE_INVALID_HCRC:
				printf("Invalid Header CRC\n");
				break;
			case UIMAGE_INVALID_DCRC:
				printf("Invalid Data CRC\n");
				break;
			case UIMAGE_INVALID_SIZE:
				printf("Invalid Size\n");
				break;
			case UIMAGE_INVALID_MULTI_SIZES:
				printf("Invalid multi-image size(s)\n");
				break;
			default:
				printf("Invalid\n");
		}
		return 1;
	}
	hdr = (struct image_header *)data;
	if (uimage_type(hdr) == IH_TYPE_MULTI) {
		int i;
		int num_images = uimage_multi_count(hdr);

		for (i = 1; i <= num_images; i++) {
			subhdr = uimage_multi_image(hdr, i);
			sublen = uimage_multi_size(hdr, i);
			if (rc = uimage_invalid(subhdr, sublen)) {
				switch(rc) {
					case UIMAGE_INVALID_MAGIC:
						printf("Image %d: Invalid Magic\n", i);
						break;
					case UIMAGE_INVALID_HCRC:
						printf("Image %d: Invalid Header CRC\n", i);
						break;
					case UIMAGE_INVALID_DCRC:
						printf("Image %d: Invalid Data CRC\n", i);
						break;
					case UIMAGE_INVALID_SIZE:
						printf("Image %d: Invalid Size\n", i);
						break;
					case UIMAGE_INVALID_MULTI_SIZES:
						printf("Image %d: Invalid multi-image size(s)\n", i);
						break;
					default:
						printf("Image %d: Invalid\n", i);
				}
				return 1;
			}
			if (uimage_type(subhdr) == IH_TYPE_MULTI) {
				printf("Image %d: Invalid sub-Multi\n", i);
				return 1;
			}
		}
	}
	return 0;
}

unsigned uimage_parse(struct image_header *hdr, 
		unsigned *kernel_load, unsigned *kernel_ep, unsigned *kernel_size,
		unsigned *kernel_addr, unsigned *ramdisk_addr, unsigned *ramdisk_size)
{
	unsigned num, i;
	struct image_header *subhdr;

	*kernel_size = 0;
	*ramdisk_size = 0;

	num = uimage_count(hdr);

	for (i = 1; i <= num; i++) {
		subhdr = uimage_image(hdr, i);

		if (uimage_comp(subhdr) != IH_COMP_NONE) {
			printf("Image %d: Compression not supported\n", i);
			return 1;
		}

		if (uimage_arch(subhdr) != IH_CPU_ARM) {
			printf("Image %d: CPU not supported\n", i);
			return 1;
		}

		if (uimage_os(subhdr) != IH_OS_LINUX) {
			printf("Image %d: OS not supported\n", i);
			return 1;
		}

		switch (uimage_type(subhdr)) {

			case IH_TYPE_KERNEL:
				*kernel_size = uimage_size(subhdr);
				*kernel_load = uimage_load(subhdr);
				*kernel_ep = uimage_ep(subhdr);
				*kernel_addr = uimage_data(subhdr);
				break;
			case IH_TYPE_RAMDISK:
				*ramdisk_size = uimage_size(subhdr);
				*ramdisk_addr = uimage_data(subhdr);
				break;
			default:
				printf("Image %d: Type not supported\n", i);
				return 1;
		}
	}
	return 0;
}

