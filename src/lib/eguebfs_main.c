/* EGUEBFS - FUSE based Egueb filesystem
 * Copyright (C) 2015 - 2015 Jorge Luis Zapata
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.
 * If not, see <http://www.gnu.org/licenses/>.
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif

#include <fuse.h>
#include <errno.h>
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/*----------------------------------------------------------------------------*
 *                               FUSE interface                               *
 *----------------------------------------------------------------------------*/
static int _eguebfs_readlink(const char *path, char *buf, size_t size)
{
	return 0;
}

static int _eguebfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		off_t offset, struct fuse_file_info *fi)
{
	return 0;
}

static int _eguebfs_getattr(const char *path, struct stat *stbuf)
{
	return 0;
}

static int _eguebfs_open(const char *path, struct fuse_file_info *fi)
{
	return 0;
}

static int _eguebfs_read(const char *path, char *buf, size_t size, off_t offset,
		struct fuse_file_info *fi)
{
	return 0;
}

static int _eguebfs_statfs(const char *path, struct statvfs *stbuf)
{
	return 0;
}

static int _eguebfs_rename(const char *orig, const char *dest)
{
	return -EACCES;
}

static void * _eguebfs_init(struct fuse_conn_info *conn)
{
	return NULL;
}

static struct fuse_operations eguebfs_ops = {
	.getattr  = _eguebfs_getattr,
	.readlink = _eguebfs_readlink,
	.readdir  = _eguebfs_readdir,
	.open     = _eguebfs_open,
	.read     = _eguebfs_read,
	.statfs   = _eguebfs_statfs,
	.rename   = _eguebfs_rename,
	.init     = _eguebfs_init,
};
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
EAPI void eguebfs_mount(Egueb_Dom_Node *doc, const char *to)
{
	//fuse_main(argc - 1, argv + 1, &eguebfs_ops, mfs);
}
