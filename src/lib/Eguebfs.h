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

#ifndef _EGUEBFS_H
#define _EGUEBFS_H

#include <Eina.h>
#include <Egueb_Dom.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "eguebfs_build.h"

typedef struct _Eguebfs Eguebfs;

EAPI void eguebfs_init(void);
EAPI void eguebfs_shutdown(void);

EAPI Eguebfs * eguebfs_mount(Egueb_Dom_Node *doc, const char *to);
EAPI void eguebfs_umount(Eguebfs *thiz);

#ifdef __cplusplus
}
#endif

#endif /*_EGUEBFS_H*/

