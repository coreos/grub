sbin_PROGRAMS = grub

AM_CFLAGS = @GRUB_CFLAGS@ -DGRUB_UTIL=1 -DFSYS_EXT2FS=1 -DFSYS_FAT=1 \
	-DFSYS_FFS=1 -fwritable-strings \
	-I$(top_srcdir)/stage2 -I$(top_srcdir)/stage1

grub_SOURCES = main.c asmstub.c getopt.c getopt1.c getopt.h
grub_LDADD = ../stage2/libgrub.a @GRUB_LIBS@
