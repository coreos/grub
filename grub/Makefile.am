AUTOMAKE_OPTIONS = gnits

EXTRA_PROGRAMS = grub
sbin_PROGRAMS = $(sbingrub)
CLEANFILES = $(EXTRA_PROGRAMS)

INCLUDES = -I$(top_srcdir)/shared_src

grub_SOURCES = main.c asmstub.c
grub_LDADD = boot.o common.o char_io.o cmdline.o disk_io.o gunzip.o \
	fsys_ffs.o fsys_ext2fs.o fsys_fat.o stage2.o

@SHARED_SRC_RULES@
