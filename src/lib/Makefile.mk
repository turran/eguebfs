
lib_LTLIBRARIES += src/lib/libeguebfs.la

installed_headersdir = $(pkgincludedir)-$(VMAJ)
dist_installed_headers_DATA = \
src/lib/Eguebfs.h

src_lib_libeguebfs_la_SOURCES = \
src/lib/eguebfs_main.c

src_lib_libeguebfs_la_CPPFLAGS = \
-I$(top_srcdir)/src/lib \
-DDESCRIPTIONS_DIR=\"$(pkgdatadir)/descriptions\" \
-DEGUEBFS_BUILD \
@EGUEBFS_CFLAGS@

src_lib_libeguebfs_la_LIBADD = \
@EGUEBFS_LIBS@

src_lib_libeguebfs_la_LDFLAGS = -no-undefined -version-info @version_info@
