/*
 * Boot-sector fuzz target for exfatprogs.
 *
 * Stops right after the boot-sector parse. No directory is read, so the file
 * entry checksum never rejects a mutated input and every one of them reaches
 * the geometry arithmetic in read_boot_sect().
 */
#include "image.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	struct image img;

	image_open(&img, data, size);
	image_close(&img);
	return 0;
}
