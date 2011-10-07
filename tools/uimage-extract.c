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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <stdint.h>
#include <netinet/in.h>
#include <time.h>

#include "image.h"

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
		fprintf(stderr, "size %u != len %u\n", ntohl(hdr->ih_size), len);
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

write_it(const char *name, void *data, unsigned len)
{
	unsigned i = 0;
	unsigned chunk;
	FILE *fp = fopen(name, "w");

	if (!fp) {
		fprintf(stderr, "Unable to open %s\n", name);
		return;
	}

	fprintf(stderr, "Writing %u bytes to %s\n", len, name);

	while (i < len) {
		chunk = (len - i > 4096) ? 4096 : (len - i);
		fwrite(((char *)data + i), 1, chunk, fp);
		i += chunk;
	}
	fclose(fp);
}

void uimage_check(void *data, unsigned len)
{
	struct image_header *hdr;
	struct image_header *subhdr;
	unsigned i, count, sublen;
	int rc;

	if (rc = uimage_invalid(data, len)) {
		switch(rc) {
			case UIMAGE_INVALID_MAGIC:
				fprintf(stderr, "Invalid Magic\n");
				break;
			case UIMAGE_INVALID_HCRC:
				fprintf(stderr, "Invalid HCRC\n");
				break;
			case UIMAGE_INVALID_DCRC:
				fprintf(stderr, "Invalid DCRC\n");
				break;
			case UIMAGE_INVALID_SIZE:
				fprintf(stderr, "Invalid Size\n");
				break;
			case UIMAGE_INVALID_MULTI_SIZES:
				fprintf(stderr, "Invalid multi-image size(s)\n");
				break;
			default:
				fprintf(stderr, "Invalid\n");
		}
		exit(1);
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
						fprintf(stderr, "Image %d: Invalid Magic\n", i);
						break;
					case UIMAGE_INVALID_HCRC:
						fprintf(stderr, "Image %d: Invalid HCRC\n", i);
						break;
					case UIMAGE_INVALID_DCRC:
						fprintf(stderr, "Image %d: Invalid DCRC\n", i);
						break;
					case UIMAGE_INVALID_SIZE:
						fprintf(stderr, "Image %d: Invalid Size\n", i);
						break;
					case UIMAGE_INVALID_MULTI_SIZES:
						fprintf(stderr, 
								"Image %d: Invalid multi-image size(s)\n", i);
						break;
					default:
						fprintf(stderr, "Image %d: Invalid\n", i);
				}
				exit(1);
			}
			if (uimage_type(subhdr) == IH_TYPE_MULTI) {
				fprintf(stderr, "Image %d: Invalid Multi\n", i);
				exit(1);
			}
		}
	}
}

void uimage_process(struct image_header *hdr)
{
	struct image_header *subhdr;
	unsigned i, num;
	time_t itime;

	if (uimage_type(hdr) == IH_TYPE_MULTI) {
		itime = uimage_time(hdr);
		fprintf(stdout, "Image 0: %s", ctime(&itime));
		fprintf(stdout, "Image 0: MULTI '%s'\n\n", uimage_name(hdr));
	}

	num = uimage_count(hdr);

	for (i = 1; i <= num; i++) {
		subhdr = uimage_image(hdr, i);

		if (i > 1) fprintf(stdout, "\n");

		if (uimage_comp(subhdr) != IH_COMP_NONE) {
			fprintf(stdout, "Image %d: Compression not supported\n", i);
			break;
		}

		if (uimage_arch(subhdr) != IH_CPU_ARM) {
			fprintf(stdout, "Image %d: CPU not supported\n", i);
			break;
		}

		if (uimage_os(subhdr) != IH_OS_LINUX) {
			fprintf(stdout, "Image %d: OS not supported\n", i);
			break;
		}

		itime = uimage_time(subhdr);

		switch (uimage_type(subhdr)) {

			case IH_TYPE_KERNEL:
				fprintf(stdout, "Image %d: %s", i, ctime(&itime));
				fprintf(stdout, "Image %d: KERNEL '%s'\n", i,
						uimage_name(subhdr));
				fprintf(stdout, "Image %d: SIZE: %u\n", i, uimage_size(subhdr));
				fprintf(stdout, "Image %d: LOAD: 0x%08x\n", i, uimage_load(subhdr));
				fprintf(stdout, "Image %d:   EP: 0x%08x\n", i, uimage_ep(subhdr));

				write_it("kernel.img", uimage_data(subhdr),
						uimage_size(subhdr));
				break;
			case IH_TYPE_RAMDISK:
				fprintf(stdout, "Image %d: %s", i, ctime(&itime));
				fprintf(stdout, "Image %d: RAMDISK '%s'\n", i,
					uimage_name(subhdr));
				fprintf(stdout, "Image %d: SIZE: %u\n", i, uimage_size(subhdr));
				write_it("ramdisk.img", uimage_data(subhdr),
						uimage_size(subhdr));
				break;
			default:
				fprintf(stderr, "Image %d: Type not supported\n", i);
		}
	}
}


int main(int argc, char **argv)
{
	long size, pos;
	unsigned chunk;
	char *data;
	FILE *fp;

	if (argc != 2) {
		fprintf(stderr, "No uImage filename given.\n");
		exit(1);
	}
	
	fp = fopen(argv[1], "r");

	if (!fp) {
		fprintf(stderr, "Unable to open uImage file\n");
		exit(1);
	}

	fseek(fp, 0, SEEK_END);

	size = ftell(fp);

	fseek(fp, 0, SEEK_SET);

	data = (char *)malloc(size);

	if (!data) {
		fprintf(stderr, "Unable to malloc\n");
		fclose(fp);
		exit(1);
	}

	pos = 0;

	while (pos < size) {
		chunk = (size - pos > 4096) ? 4096 : (size - pos);
		fread(data + pos, 1, chunk, fp);
		pos += chunk;
	}

	fclose(fp);

	uimage_check(data, size);
	uimage_process((struct image_header*)data);
}

