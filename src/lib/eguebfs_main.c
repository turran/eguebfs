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

#define _GNU_SOURCE

#include <Eguebfs.h>

#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <errno.h>
#include <stdio.h>

#define ERR(...) EINA_LOG_DOM_ERR(eguebfs_log_dom, __VA_ARGS__)
#define WRN(...) EINA_LOG_DOM_WARN(eguebfs_log_dom, __VA_ARGS__)
#define INF(...) EINA_LOG_DOM_INFO(eguebfs_log_dom, __VA_ARGS__)
#define DBG(...) EINA_LOG_DOM_DBG(eguebfs_log_dom, __VA_ARGS__)
#define CRI(...) EINA_LOG_DOM_CRIT(eguebfs_log_dom, __VA_ARGS__)
/*
 * For a XML file like this:
 * <svg>
 *   <g color="red">
 *   </g>
 *   <g>
 *   </g>
 *   <rect/>
 * </svg>
 *
 * The tree should be like:
 * / -> doc
 * /svg@0 -> svg at level 0
 * /svg@0/g@0 -> g at repetition 0
 * /svg@0/g@0/color/ -> color attribute
 * /svg@0/g@0/color/base -> color attribute base value
 * /svg@0/g@0/color/anim -> color attribute anim value
 * /svg@0/g@0/color/style -> color attribute style value
 * /svg@0/g@0/color/final -> color attribute final value
 * /svg@0/g@1 -> g at repetition 1
 * /svg@0/rect@0 -> g at repetition 0
 *
 */

typedef enum _Eguebfs_File_Type
{
	EGUEBFS_FILE_TYPE_NODE,
	EGUEBFS_FILE_TYPE_ATTR_BASE,
	EGUEBFS_FILE_TYPE_ATTR_ANIM,
	EGUEBFS_FILE_TYPE_ATTR_STYLED,
	EGUEBFS_FILE_TYPE_ATTR_FINAL,
} Eguebfs_File_Type;

typedef struct _Eguebfs_File
{
	Eguebfs_File_Type type;
	Egueb_Dom_Node *n;
} Eguebfs_File;

struct _Eguebfs
{
	Eina_Thread thread;
	Egueb_Dom_Node *doc;
	char *mountpoint;
	struct fuse_chan *chan;
	struct fuse *fuse;
};
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
static int _init = 0;
static int eguebfs_log_dom = -1;

static Eina_Bool _eguebfs_name_is_element(const char *p, char **rname, int *count)
{
	const char *child_depth;

	child_depth = strchr(p, '@');
	if (child_depth)
	{
		char *name;

		/* get the count */
		*count = strtoul(child_depth + 1, NULL, 10);
		/* get the real name */
		name = malloc(child_depth - p + 1);
		strncpy(name, p, child_depth - p);
		name[child_depth - p] = '\0';
		*rname = name;

		return EINA_TRUE;
	}
	else
	{
		return EINA_FALSE;
	}
}

static Eina_Bool _eguebfs_file_node_delete(Eguebfs_File *f)
{
	Egueb_Dom_Node_Type type;

	type = egueb_dom_node_type_get(f->n);
	switch (type)
	{
		case EGUEB_DOM_NODE_TYPE_ELEMENT:
		{
			Egueb_Dom_Node *parent;
			parent = egueb_dom_node_parent_get(f->n);
			egueb_dom_node_child_remove(parent,
					egueb_dom_node_ref(f->n), NULL);
			egueb_dom_node_unref(parent);
		}
		break;

		default:
		return EINA_FALSE;
	}
	return EINA_TRUE;
}

static Eina_Bool _eguebfs_file_node_find(Eguebfs_File *f, const char *p)
{
	Egueb_Dom_Node_Type type;

	type = egueb_dom_node_type_get(f->n);
	switch (type)
	{
		/* only topmost element */
		case EGUEB_DOM_NODE_TYPE_DOCUMENT:
		{
			Egueb_Dom_Node *topmost;
			Egueb_Dom_String *name;
			topmost = egueb_dom_document_document_element_get(f->n);
			if (!topmost)
			{
				return EINA_FALSE;
			}
			name = egueb_dom_node_name_get(topmost);
			if (!strcmp(egueb_dom_string_chars_get(name), p))
			{
				egueb_dom_node_unref(f->n);
				f->n = topmost;
			}
			else
			{
				egueb_dom_node_unref(topmost);
				return EINA_FALSE;
			}
		}
		break;

		/* only attributes or child nodes */
		case EGUEB_DOM_NODE_TYPE_ELEMENT:
		{
			char *real_name;
			int depth;

			if (_eguebfs_name_is_element(p, &real_name, &depth))
			{
				Egueb_Dom_Node *child;
				Egueb_Dom_Node *found = NULL;
				Eina_Bool ret = EINA_FALSE;
				int count = 0;

				child = egueb_dom_node_child_first_get(f->n);
				while (child)
				{
					Egueb_Dom_Node *tmp;
					Egueb_Dom_String *name;

					name = egueb_dom_node_name_get(child);
					if (!strcmp(egueb_dom_string_chars_get(name), real_name))
					{
						count++;
						if (count == depth)
						{
							found = child;
							ret = EINA_TRUE;
							break;
						}
					}
					
					tmp = egueb_dom_node_sibling_next_get(child);
					egueb_dom_node_unref(child);
					child = tmp;
				}
				free(real_name);
				egueb_dom_node_unref(f->n);
				f->n = found;
				return ret;
			}
			else
			{
				Egueb_Dom_Node *attr;
				Egueb_Dom_String *attr_name;

				attr_name = egueb_dom_string_new_with_chars(p);
				/* check if it is an attribute */
				attr = egueb_dom_element_attribute_node_get(f->n, attr_name);
				egueb_dom_string_unref(attr_name);
				if (!attr)
				{
					return EINA_FALSE;
				}
				egueb_dom_node_unref(f->n);
				f->n = attr;
			}
		}
		break;

		/* only final, base, anim, styled */
		case EGUEB_DOM_NODE_TYPE_ATTRIBUTE:
		if (!strcmp(p, "base"))
			f->type = EGUEBFS_FILE_TYPE_ATTR_BASE;
		else if (!strcmp(p, "anim") && egueb_dom_attr_is_animatable(f->n))
			f->type = EGUEBFS_FILE_TYPE_ATTR_ANIM;
		else if (!strcmp(p, "styled") && egueb_dom_attr_is_stylable(f->n))
			f->type = EGUEBFS_FILE_TYPE_ATTR_STYLED;
		else if (!strcmp(p, "final"))
			f->type = EGUEBFS_FILE_TYPE_ATTR_FINAL;
		else
			return EINA_FALSE;
		break;

		default:
		return EINA_FALSE;
		break;

	}
	return EINA_TRUE;
}

static void _eguebfs_file_node_list(Eguebfs_File *f, void *buf, fuse_fill_dir_t filler)
{
	Egueb_Dom_Node_Type type;
	type = egueb_dom_node_type_get(f->n);
	switch (type)
	{
		case EGUEB_DOM_NODE_TYPE_DOCUMENT:
		{
			Egueb_Dom_Node *topmost;
			topmost = egueb_dom_document_document_element_get(f->n);
			if (topmost)
			{
				Egueb_Dom_String *name;
				name = egueb_dom_node_name_get(topmost);
				filler(buf, egueb_dom_string_chars_get(name), NULL, 0);
				egueb_dom_string_unref(name);
				egueb_dom_node_unref(topmost);
			}
		}
		break;

		case EGUEB_DOM_NODE_TYPE_ELEMENT:
		{
			Egueb_Dom_Node *child;
			Egueb_Dom_Node_Map_Named *attrs;
			Eina_Hash *repetitions;
			int i;

			/* add every children */
			child = egueb_dom_node_child_first_get(f->n);
			repetitions = eina_hash_string_superfast_new(free);
			while (child)
			{
				Egueb_Dom_Node *tmp;
				Egueb_Dom_String *name;
				char *final_name;
				int *count;

				name = egueb_dom_node_name_get(child);
				count = eina_hash_find(repetitions, egueb_dom_string_chars_get(name));
				if (!count)
				{
					count = malloc(sizeof(int));
					*count = 1;
					eina_hash_add(repetitions, egueb_dom_string_chars_get(name), count);
				}
				else
				{
					*count = *count + 1;
				}
				if (asprintf(&final_name, "%s@%d", egueb_dom_string_chars_get(name), *count) < 0)
					continue;
				filler(buf, final_name, NULL, 0);
				free(final_name);

				/* next child */
				tmp = egueb_dom_node_sibling_next_get(child);
				egueb_dom_node_unref(child);
				child = tmp;
			}
			eina_hash_free(repetitions);
			/* add every attribute */
			attrs = egueb_dom_node_attributes_get(f->n);
			for (i = 0; i < egueb_dom_node_map_named_length(attrs); i++)
			{
				Egueb_Dom_Node *attr;
				Egueb_Dom_String *name;

				attr = egueb_dom_node_map_named_at(attrs, i);
				name = egueb_dom_node_name_get(attr);
				filler(buf, egueb_dom_string_chars_get(name), NULL, 0);
			}
			egueb_dom_node_map_named_unref(attrs);
		}
		break;

		case EGUEB_DOM_NODE_TYPE_ATTRIBUTE:
		{
			filler(buf, "base", NULL, 0);
			filler(buf, "final", NULL, 0);
			if (egueb_dom_attr_is_stylable(f->n))
				filler(buf, "styled", NULL, 0);
			if (egueb_dom_attr_is_animatable(f->n))
				filler(buf, "anim", NULL, 0);
		}
		break;

		default:
		break;
	}
}

static Eina_Bool _eguebfs_file_find(Egueb_Dom_Node *doc, const char *path,
		Eguebfs_File *f)
{
	Eina_Bool ret = EINA_TRUE;
	Eina_Array *split;
	Eina_Iterator *it;
	const char *p;
	char *npath;

	npath = strdup(path);
	f->type = EGUEBFS_FILE_TYPE_NODE;
	f->n = egueb_dom_node_ref(doc);
	split = eina_file_split(npath);
	it = eina_array_iterator_new(split);
	while (eina_iterator_next(it, (void **)&p))
	{
		switch (f->type)
		{
			case EGUEBFS_FILE_TYPE_NODE:
			ret = _eguebfs_file_node_find(f, p);
			if (!ret)
				goto done;
			break;

			case EGUEBFS_FILE_TYPE_ATTR_BASE:
			case EGUEBFS_FILE_TYPE_ATTR_ANIM:
			case EGUEBFS_FILE_TYPE_ATTR_STYLED:
			case EGUEBFS_FILE_TYPE_ATTR_FINAL:
			/* no child files */
			ret = EINA_FALSE;
			break;
		}
	}
done:
	if (!ret)
	{
		egueb_dom_node_unref(f->n);
	}
	eina_iterator_free(it);
	eina_array_free(split);
	free(npath);
	return ret;
}
/*----------------------------------------------------------------------------*
 *                              Thread interface                              *
 *----------------------------------------------------------------------------*/
static void * _eguebfs_thread_main(void *data, Eina_Thread t)
{
	Eguebfs *thiz = data;

	fuse_loop(thiz->fuse);
	return NULL;
}
/*----------------------------------------------------------------------------*
 *                               FUSE interface                               *
 *----------------------------------------------------------------------------*/
static int _eguebfs_readlink(const char *path, char *buf, size_t size)
{
	ERR("readlink %s", path);
	return 0;
}

static int _eguebfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		off_t offset, struct fuse_file_info *fi)
{
	Eguebfs *thiz;
	Eguebfs_File f = { 0 };
	struct fuse_context *ctx;

	ctx = fuse_get_context();
	thiz = ctx->private_data;

	DBG("readdir %s", path);
	if (!_eguebfs_file_find(thiz->doc, path, &f))
		return -ENOENT;

	/* default files */
	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	switch (f.type)
	{
		case EGUEBFS_FILE_TYPE_NODE:
		_eguebfs_file_node_list(&f, buf, filler);
		break;

		default:
		break;
	}
	egueb_dom_node_unref(f.n);

	return 0;
}

static int _eguebfs_getattr(const char *path, struct stat *stbuf)
{
	Eguebfs *thiz;
	Eguebfs_File f = { 0 };
	Egueb_Dom_String *value;
	Eina_Bool fetched;
	struct fuse_context *ctx;
	int ret = 0;

	ctx = fuse_get_context();
	thiz = ctx->private_data;

	DBG("getattr %s", path);
	if (!_eguebfs_file_find(thiz->doc, path, &f))
	{
		WRN("No file '%s' found", path);
		return -ENOENT;
	}

	memset(stbuf, 0, sizeof(struct stat));
	switch (f.type)
	{
		case EGUEBFS_FILE_TYPE_NODE:
		{
			Egueb_Dom_Node_Type type;
			type = egueb_dom_node_type_get(f.n);
			switch (type)
			{
				case EGUEB_DOM_NODE_TYPE_DOCUMENT:
				case EGUEB_DOM_NODE_TYPE_ELEMENT:
				case EGUEB_DOM_NODE_TYPE_ATTRIBUTE:
				stbuf->st_mode = S_IFDIR | 0755;
				stbuf->st_nlink = 2;
				break;

				case EGUEB_DOM_NODE_TYPE_TEXT:
				case EGUEB_DOM_NODE_TYPE_CDATA_SECTION:
				stbuf->st_mode = S_IFREG | 0644;
				stbuf->st_nlink = 1;
				stbuf->st_size = egueb_dom_character_data_length_get(f.n);
				break;

				default:
				ERR("Unsupported node type '%d'", type);
				ret = -ENOENT;
				break;
			}
		}
		break;

		case EGUEBFS_FILE_TYPE_ATTR_BASE:
		fetched = egueb_dom_attr_string_get(f.n, EGUEB_DOM_ATTR_TYPE_BASE, &value);
		stbuf->st_mode = S_IFREG | 0644;
		stbuf->st_nlink = 1;
		if (fetched && egueb_dom_string_is_valid(value))
		{
			const char *content = egueb_dom_string_chars_get(value);
			stbuf->st_size = strlen(content);
			egueb_dom_string_unref(value);
		}
		break;

		case EGUEBFS_FILE_TYPE_ATTR_ANIM:
		fetched = egueb_dom_attr_string_get(f.n, EGUEB_DOM_ATTR_TYPE_ANIMATED, &value);
		stbuf->st_mode = S_IFREG | 0644;
		stbuf->st_nlink = 1;
		if (fetched && egueb_dom_string_is_valid(value))
		{
			const char *content = egueb_dom_string_chars_get(value);
			stbuf->st_size = strlen(content);
			egueb_dom_string_unref(value);
		}
		break;

		case EGUEBFS_FILE_TYPE_ATTR_STYLED:
		fetched = egueb_dom_attr_string_get(f.n, EGUEB_DOM_ATTR_TYPE_STYLED, &value);
		stbuf->st_mode = S_IFREG | 0644;
		stbuf->st_nlink = 1;
		if (fetched && egueb_dom_string_is_valid(value))
		{
			const char *content = egueb_dom_string_chars_get(value);
			stbuf->st_size = strlen(content);
			egueb_dom_string_unref(value);
		}
		break;

		case EGUEBFS_FILE_TYPE_ATTR_FINAL:
		fetched = egueb_dom_attr_final_string_get(f.n, &value);
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		if (fetched && egueb_dom_string_is_valid(value))
		{
			const char *content = egueb_dom_string_chars_get(value);
			stbuf->st_size = strlen(content);
			egueb_dom_string_unref(value);
		}
		break;
	}
	egueb_dom_node_unref(f.n);
	return ret;
}

static int _eguebfs_open(const char *path, struct fuse_file_info *fi)
{
	Eguebfs *thiz;
	Eguebfs_File f = { 0 };
	struct fuse_context *ctx;

	ctx = fuse_get_context();
	thiz = ctx->private_data;

	DBG("open %s", path);
	if (!_eguebfs_file_find(thiz->doc, path, &f))
		return -ENOENT;

	egueb_dom_node_unref(f.n);
	return 0;
}

static int _eguebfs_read(const char *path, char *buf, size_t size, off_t offset,
		struct fuse_file_info *fi)
{
	Eguebfs *thiz;
	Eguebfs_File f = { 0 };
	Egueb_Dom_String *value = NULL;
	Eina_Bool fetched = EINA_FALSE;
	struct fuse_context *ctx;

	ctx = fuse_get_context();
	thiz = ctx->private_data;

	DBG("read %s", path);
	if (!_eguebfs_file_find(thiz->doc, path, &f))
		return -ENOENT;

	switch (f.type)
	{
		case EGUEBFS_FILE_TYPE_NODE:
		{
			Egueb_Dom_Node_Type type;
			type = egueb_dom_node_type_get(f.n);
			switch (type)
			{
				case EGUEB_DOM_NODE_TYPE_TEXT:
				case EGUEB_DOM_NODE_TYPE_CDATA_SECTION:
				value = egueb_dom_character_data_data_get(f.n);
				fetched = EINA_TRUE;
				break;

				default:
				ERR("Unsupported node type '%d'", type);
				break;
			}
		}
		break;

		case EGUEBFS_FILE_TYPE_ATTR_BASE:
		fetched = egueb_dom_attr_string_get(f.n, EGUEB_DOM_ATTR_TYPE_BASE, &value);
		break;

		case EGUEBFS_FILE_TYPE_ATTR_ANIM:
		fetched = egueb_dom_attr_string_get(f.n, EGUEB_DOM_ATTR_TYPE_ANIMATED, &value);
		break;

		case EGUEBFS_FILE_TYPE_ATTR_STYLED:
		fetched = egueb_dom_attr_string_get(f.n, EGUEB_DOM_ATTR_TYPE_STYLED, &value);
		break;

		case EGUEBFS_FILE_TYPE_ATTR_FINAL:
		fetched = egueb_dom_attr_final_string_get(f.n, &value);
		break;
	}

	if (fetched)
	{
		const char *content = egueb_dom_string_chars_get(value);
		size_t len;

		len = strlen(content);
		if (offset < len)
		{
			if (offset + size > len)
				size = len - offset;
			memcpy(buf, content + offset, size);
		} 
		else
		{
			size = 0;
		}
		egueb_dom_string_unref(value);
	}
	else
	{
		size = 0;
	}
	egueb_dom_node_unref(f.n);
	return size;
}

static int _eguebfs_write(const char *path, const char *buf, size_t size, off_t offset,
		struct fuse_file_info *fi)
{
	Eguebfs *thiz;
	Eguebfs_File f = { 0 };
	Egueb_Dom_String *value = NULL;
	Eina_Bool written = EINA_FALSE;
	struct fuse_context *ctx;

	ctx = fuse_get_context();
	thiz = ctx->private_data;

	DBG("write %s", path);
	if (!_eguebfs_file_find(thiz->doc, path, &f))
		return -ENOENT;

	switch (f.type)
	{
		case EGUEBFS_FILE_TYPE_NODE:
		{
			Egueb_Dom_Node_Type type;
			type = egueb_dom_node_type_get(f.n);
			switch (type)
			{
				case EGUEB_DOM_NODE_TYPE_TEXT:
				case EGUEB_DOM_NODE_TYPE_CDATA_SECTION:
				value = egueb_dom_string_new_with_length(buf, size);
				egueb_dom_character_data_data_delete(f.n, 0, -1, NULL);
				egueb_dom_character_data_data_append(f.n, value, NULL);
				written = EINA_TRUE;
				break;

				default:
				ERR("Unsupported node type '%d'", type);
				break;
			}
		}
		break;

		case EGUEBFS_FILE_TYPE_ATTR_FINAL:
		break;

		case EGUEBFS_FILE_TYPE_ATTR_BASE:
		value = egueb_dom_string_new_with_length(buf, size);
		written = egueb_dom_attr_string_set(f.n, EGUEB_DOM_ATTR_TYPE_BASE, value);
		egueb_dom_string_unref(value);
		break;

		case EGUEBFS_FILE_TYPE_ATTR_ANIM:
		value = egueb_dom_string_new_with_length(buf, size);
		written = egueb_dom_attr_string_set(f.n, EGUEB_DOM_ATTR_TYPE_ANIMATED, value);
		egueb_dom_string_unref(value);
		break;

		case EGUEBFS_FILE_TYPE_ATTR_STYLED:
		value = egueb_dom_string_new_with_length(buf, size);
		written = egueb_dom_attr_string_set(f.n, EGUEB_DOM_ATTR_TYPE_STYLED, value);
		egueb_dom_string_unref(value);
		break;
	}

	egueb_dom_node_unref(f.n);
	if (!written)
		return -EINVAL;
	else
		return size;
}

static int _eguebfs_truncate(const char *path, off_t new_length)
{
	Eguebfs *thiz;
	Eguebfs_File f = { 0 };
	struct fuse_context *ctx;
	int ret = -EINVAL;

	ctx = fuse_get_context();
	thiz = ctx->private_data;

	DBG("truncate %s", path);

	if (!_eguebfs_file_find(thiz->doc, path, &f))
		return -ENOENT;

	switch (f.type)
	{
		case EGUEBFS_FILE_TYPE_NODE:
		{
			Egueb_Dom_Node_Type type;

			type = egueb_dom_node_type_get(f.n);
			switch (type)
			{
				case EGUEB_DOM_NODE_TYPE_TEXT:
				case EGUEB_DOM_NODE_TYPE_CDATA_SECTION:
				{
					int length;

					length = egueb_dom_character_data_length_get(f.n);
					ret = 0;
					if (new_length < length)
					{
						egueb_dom_character_data_data_delete(f.n, 0, length - new_length, NULL);
					}
				}
				break;

				default:
				ERR("Unsupported node type '%d'", type);
				break;
			}

		}
		break;

		case EGUEBFS_FILE_TYPE_ATTR_BASE:
		case EGUEBFS_FILE_TYPE_ATTR_ANIM:
		case EGUEBFS_FILE_TYPE_ATTR_STYLED:
		ret = 0;
		break;

		default:
		break;
	}

	egueb_dom_node_unref(f.n);
	return ret;
}

static int _eguebfs_mkdir(const char *path, mode_t m)
{
	Eguebfs *thiz;
	Eguebfs_File f = { 0 };
	struct fuse_context *ctx;
	char *bpath;
	char *chpath;
	char *real_name;
	int depth;
	int ret = -EINVAL;

	ctx = fuse_get_context();
	thiz = ctx->private_data;

	DBG("mkdir %s", path);
	chpath = strrchr(path, '/');
	if (!chpath)
		return -EINVAL;
	bpath = strndup(path, chpath - path);
	/* get the path before the last path entry */
	if (!_eguebfs_file_find(thiz->doc, bpath, &f))
		goto done;

	if (f.type != EGUEBFS_FILE_TYPE_NODE)
		goto no_file;

	if (egueb_dom_node_type_get(f.n) != EGUEB_DOM_NODE_TYPE_ELEMENT)
		goto no_file;

	if (_eguebfs_name_is_element(chpath + 1, &real_name, &depth))
	{
		Egueb_Dom_Node *child;
		int count = 0;

		/* make sure that the child is valid */
		child = egueb_dom_node_child_first_get(f.n);
		while (child)
		{
			Egueb_Dom_Node *tmp;
			Egueb_Dom_String *name;

			name = egueb_dom_node_name_get(child);
			if (!strcmp(egueb_dom_string_chars_get(name), real_name))
				count++;
			tmp = egueb_dom_node_sibling_next_get(child);
			egueb_dom_node_unref(child);
			child = tmp;
		}
		if (depth == count + 1)
		{
			Egueb_Dom_String *name;

			name = egueb_dom_string_new_with_chars(real_name);
			child = egueb_dom_document_element_create(thiz->doc, name, NULL);
			if (child)
			{
				if (egueb_dom_node_child_append(f.n, child, NULL))
					ret = 0;
			}
			egueb_dom_string_unref(name);
		}
		free(real_name);
	}

no_file:
	egueb_dom_node_unref(f.n);
done:
	free(bpath);
	return ret;
}

static int _eguebfs_rmdir(const char *path)
{
	Eguebfs *thiz;
	Eguebfs_File f = { 0 };
	struct fuse_context *ctx;
	int ret = -EINVAL;

	ctx = fuse_get_context();
	thiz = ctx->private_data;

	DBG("rmdir %s", path);
	if (!_eguebfs_file_find(thiz->doc, path, &f))
		return -ENOENT;

	switch (f.type)
	{
		case EGUEBFS_FILE_TYPE_NODE:
		if (_eguebfs_file_node_delete(&f))
			ret = 0;
		break;

		default:
		break;
	}
	egueb_dom_node_unref(f.n);
	return ret;
}

static void * _eguebfs_init(struct fuse_conn_info *conn)
{
	Eguebfs *thiz;
	struct fuse_context *ctx;

	ctx = fuse_get_context();
	thiz = ctx->private_data;

	DBG("init %p", thiz);
	return thiz;
}

static struct fuse_operations eguebfs_ops = {
	.getattr  = _eguebfs_getattr,
	.readlink = _eguebfs_readlink,
	.readdir  = _eguebfs_readdir,
	.open     = _eguebfs_open,
	.read     = _eguebfs_read,
	.write    = _eguebfs_write,
	.truncate = _eguebfs_truncate,
	.init     = _eguebfs_init,
	.rmdir    = _eguebfs_rmdir,
	.mkdir    = _eguebfs_mkdir,
};
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
EAPI void eguebfs_init(void)
{
	if (!_init++)
	{
		eina_init();
		eguebfs_log_dom = eina_log_domain_register("eguebfs", NULL);
	}
}

EAPI void eguebfs_shutdown(void)
{
	if (_init == 1)
	{
		eina_log_domain_unregister(eguebfs_log_dom);
		eina_shutdown();
	}
	_init--;
}

EAPI Eguebfs * eguebfs_mount(Egueb_Dom_Node *doc, const char *to)
{
	Eguebfs *thiz;
	struct fuse_chan *chan;
#if 0
	struct fuse_args args = FUSE_ARGS_INIT(0, NULL);
#endif

	if (!doc)
		return NULL;
	if (!to)
		goto no_chan;

	chan = fuse_mount(to, NULL);
	if (!chan)
		goto no_chan;

#if 0
	fuse_opt_add_arg(&args, "");
	fuse_opt_add_arg(&args, "-odebug");
#endif

	thiz = calloc(1, sizeof(Eguebfs));
	thiz->doc = doc;
	thiz->mountpoint = strdup(to);
	thiz->chan = chan;
#if 0
	thiz->fuse = fuse_new(thiz->chan, &args, &eguebfs_ops, sizeof(eguebfs_ops), thiz);
	fuse_opt_free_args(&args);
#endif
	thiz->fuse = fuse_new(thiz->chan, NULL, &eguebfs_ops, sizeof(eguebfs_ops), thiz);

	/* create the thread and start processing there */
	if (!eina_thread_create(&thiz->thread, EINA_THREAD_NORMAL, -1,
			_eguebfs_thread_main, thiz))
		goto no_thread;
	return thiz;

no_thread:
	fuse_unmount(thiz->mountpoint, thiz->chan);
	fuse_destroy(thiz->fuse);
	free(thiz->mountpoint);
	free(thiz);
no_chan:
	egueb_dom_node_unref(doc);
	return NULL;
}

EAPI void eguebfs_umount(Eguebfs *thiz)
{
	fuse_unmount(thiz->mountpoint, thiz->chan);
	fuse_destroy(thiz->fuse);
	eina_thread_join(thiz->thread);
	egueb_dom_node_unref(thiz->doc);
	free(thiz->mountpoint);
	free(thiz);
}
