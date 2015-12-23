bin_PROGRAMS = \
src/bin/eguebfs

src_bin_eguebfs_LDADD = \
$(top_builddir)/src/lib/libender.la \
@ENDER_LIBS@

src_bin_eguebfs_CPPFLAGS = \
-I$(top_srcdir)/src/lib \
@ENDER_CFLAGS@

src_bin_eguebfs_SOURCES = \
src/bin/eguebfs.c
