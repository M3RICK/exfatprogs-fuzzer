/*
 * Root-directory fuzz target for exfatprogs.
 *
 * A lookup in the root forces the library to read and interpret full dentry
 * sets (file 0x85, stream 0xC0, name 0xC1), following the counts and lengths
 * stored in the image.
 */
#include <stdlib.h>

#include "image.h"
#include "exfat_dir.h"

/* The name never matches, so every dentry set in the root gets parsed. */
static void lookup_in_root(struct exfat *exfat)
{
	struct exfat_lookup_filter filter = { 0 };

	if (exfat_lookup_file(exfat, exfat->root, "fuzz", &filter) == 0)
		free(filter.out.dentry_set);
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	struct image img;

	if (image_open(&img, data, size) == 0)
		lookup_in_root(img.exfat);
	image_close(&img);
	return 0;
}
