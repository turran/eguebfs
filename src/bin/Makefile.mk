bin_PROGRAMS = \
src/bin/eguebfs

src_bin_eguebfs_LDADD = \
$(top_builddir)/src/lib/libeguebfs.la \
@EGUEBFS_BIN_LIBS@

src_bin_eguebfs_CPPFLAGS = \
-I$(top_srcdir)/src/lib \
@EGUEBFS_BIN_CFLAGS@

src_bin_eguebfs_SOURCES = \
src/bin/eguebfs.c
