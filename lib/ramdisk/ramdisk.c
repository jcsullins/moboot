#include <string.h>
#include <stdlib.h>
#include <debug.h>
#if WITH_LIB_BIO
#include <lib/bio.h>
#endif
#include <err.h>

static bdev_t bio_dev;
ssize_t ramdisk_bio_read_block(struct bdev *dev, const void *buf, bnum_t block, uint count);
ssize_t ramdisk_bio_write_block(struct bdev *dev, const void *buf, bnum_t block, uint count);

unsigned char *ramdisk_loc;


void ramdisk_init(unsigned loc, unsigned size)
{

	ramdisk_loc = (unsigned char *)loc;

    bio_initialize_bdev(&bio_dev, "/dev/ramdisk", 512, size / 512);

    bio_dev.read_block = &ramdisk_bio_read_block;
    bio_dev.write_block = &ramdisk_bio_write_block;

    bio_register_device(&bio_dev);
}


ssize_t ramdisk_bio_read_block(struct bdev *dev, const void *buf, bnum_t block, uint count)
{
    ASSERT(dev);

	memcpy(buf, ramdisk_loc + block * dev->block_size, count * dev->block_size);
}

ssize_t ramdisk_bio_write_block(struct bdev *dev, const void *buf, bnum_t block, uint count)
{
    return ERR_IO;
}

