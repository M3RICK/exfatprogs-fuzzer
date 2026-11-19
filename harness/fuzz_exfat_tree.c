/*
 * Recursive directory fuzz target for exfatprogs.
 *
 * Descends into every subdirectory through the start cluster stored in its
 * stream entry, which exercises subdirectory cluster chains and nested
 * dentry sets. Depth and entries per directory are capped to keep every
 * input fast.
 */
#include "image.h"
#include "exfat_dir.h"

#define MAX_DEPTH	8
#define MAX_DENTRIES	200000

static void walk_dir(struct exfat *exfat, struct exfat_inode *dir, int depth);

/* Builds the inode described by a file entry and its stream entry. */
static struct exfat_inode *inode_from_set(struct exfat_dentry *file,
					  struct exfat_dentry *stream)
{
	struct exfat_inode *inode;

	inode = exfat_alloc_inode(le16_to_cpu(file->dentry.file.attr));
	if (!inode)
		return NULL;

	inode->first_clus = le32_to_cpu(stream->dentry.stream.start_clu);
	inode->size = le64_to_cpu(stream->dentry.stream.size);
	return inode;
}

/* Recurses into the file set under the iterator when it is a directory. */
static void visit_file_set(struct exfat *exfat, struct exfat_de_iter *iter,
			   struct exfat_dentry *file, int depth)
{
	struct exfat_dentry *stream;
	struct exfat_inode *child;

	if (exfat_de_iter_get(iter, 1, &stream) != 0 ||
	    stream->type != EXFAT_STREAM)
		return;

	child = inode_from_set(file, stream);
	if (!child)
		return;

	if (child->attr & ATTR_SUBDIR)
		walk_dir(exfat, child, depth + 1);
	exfat_free_inode(child);
}

static void walk_entries(struct exfat *exfat, struct exfat_de_iter *iter,
			 int depth)
{
	struct exfat_dentry *de;
	int seen = 0;

	while (seen++ < MAX_DENTRIES && exfat_de_iter_get(iter, 0, &de) == 0) {
		if (de->type != EXFAT_FILE) {
			exfat_de_iter_advance(iter, 1);
			continue;
		}

		visit_file_set(exfat, iter, de, depth);
		exfat_de_iter_advance(iter, de->dentry.file.num_ext + 1);
	}
}

static void walk_dir(struct exfat *exfat, struct exfat_inode *dir, int depth)
{
	struct exfat_de_iter iter;
	struct buffer_desc *buffer;

	if (depth > MAX_DEPTH)
		return;

	buffer = exfat_alloc_buffer(exfat);
	if (!buffer)
		return;

	if (exfat_de_iter_init(&iter, exfat, dir, buffer) == 0)
		walk_entries(exfat, &iter, depth);
	exfat_free_buffer(exfat, buffer);
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	struct image img;

	if (image_open(&img, data, size) == 0)
		walk_dir(img.exfat, img.exfat->root, 0);
	image_close(&img);
	return 0;
}
