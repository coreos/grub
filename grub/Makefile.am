sbin_PROGRAMS = grub

AM_CFLAGS = $(GRUB_CFLAGS) -DGRUB_UTIL=1 -DFSYS_EXT2FS=1 -DFSYS_FAT=1 \
	-DFSYS_FFS=1 -DFSYS_MINIX=1 -DSUPPORT_SERIAL=1 \
	-fwritable-strings -I$(top_srcdir)/stage2 \
	-I$(top_srcdir)/stage1 -I$(top_srcdir)/lib

grub_SOURCES = main.c asmstub.c
grub_LDADD = ../stage2/libgrub.a  ../lib/libcommon.a $(GRUB_LIBS)
