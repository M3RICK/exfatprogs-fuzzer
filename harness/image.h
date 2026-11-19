/*
 * A fuzzer input seen by exfatprogs as a block device.
 *
 * The bytes are written to a memfd that the library opens by path, like any
 * device, before parsing the boot sector. Every fuzz iteration is bracketed
 * by image_open() and image_close().
 */
#ifndef IMAGE_H
#define IMAGE_H

#include <stddef.h>
#include <stdint.h>

#include "exfat_ondisk.h"
#include "libexfat.h"
#include "exfat_fs.h"

struct image {
	int fd;
	char path[32];
	struct exfat_user_input ui;
	struct exfat_blk_dev bd;
	struct exfat *exfat;
};

int image_open(struct image *img, const uint8_t *data, size_t size);
void image_close(struct image *img);

#endif /* IMAGE_H */
