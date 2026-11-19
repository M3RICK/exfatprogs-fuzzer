#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>

#include "image.h"

extern unsigned int print_level;

static void image_init(struct image *img)
{
	img->fd = -1;
	img->exfat = NULL;
	exfat_init_user_input(&img->ui);
	exfat_init_blk_dev_info(&img->bd);
	print_level = 0; /* library logs off, stderr stays with the sanitizers */
}

static int image_write(struct image *img, const uint8_t *data, size_t size)
{
	img->fd = memfd_create("exfatimg", 0);
	if (img->fd < 0)
		return -1;

	if (write(img->fd, data, size) != (ssize_t)size)
		return -1;

	snprintf(img->path, sizeof(img->path), "/proc/self/fd/%d", img->fd);
	return 0;
}

static int image_parse(struct image *img)
{
	img->ui.dev_name = img->path;
	if (exfat_get_blk_dev_info(&img->ui, &img->bd) != 0)
		return -1;

	img->exfat = exfat_alloc_exfat(&img->bd, NULL, NULL);
	return img->exfat ? 0 : -1;
}

/* Returns 0 once the boot sector is parsed and img->exfat is usable. */
int image_open(struct image *img, const uint8_t *data, size_t size)
{
	image_init(img);
	if (image_write(img, data, size) != 0)
		return -1;

	return image_parse(img);
}

/* Also safe after a failed image_open(). */
void image_close(struct image *img)
{
	if (img->exfat)
		exfat_free_exfat(img->exfat);
	exfat_deinit_blk_dev_info(&img->bd);
	exfat_deinit_user_input(&img->ui);
	if (img->fd >= 0)
		close(img->fd);
}
