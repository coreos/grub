sbin_PROGRAMS = grub
CLEANFILES = $(EXTRA_PROGRAMS)

COMPILE = $(CC) -DGRUB_UTIL=1 -fwritable-strings $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS)
INCLUDES = -I$(top_srcdir)/shared_src

grub_SOURCES = main.c asmstub.c
grub_LDADD = boot.o common.o char_io.o cmdline.o disk_io.o gunzip.o \
	fsys_ffs.o fsys_ext2fs.o fsys_fat.o stage2.o

# FIXME: Automake hackery.
DEP_FILES = .deps/main.P .deps/asmstub.P $(wildcard .deps/*.P)

@SHARED_SRC_RULES@
