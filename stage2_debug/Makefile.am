AUTOMAKE_OPTIONS = gnits

pkglibdir = $(exec_prefix)/lib/$(PACKAGE)
pkglib_DATA = $(stage2debug)
EXTRA_DATA = stage2_debug
CLEANFILES = stage2_debug
MOSTLYCLEANFILES = stage2_debug.exec

# We can't use builtins.
COMPILE = $(CC) -fno-builtin -DDEBUG=1 $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS)
INCLUDES = -I$(top_srcdir)/shared_src

# asm.o absolutely needs to come first!
stage2_debug_exec_LDADD = asm.o boot.o common.o char_io.o cmdline.o disk_io.o \
	gunzip.o fsys_ffs.o fsys_ext2fs.o fsys_fat.o stage2.o

EXTRA_PROGRAMS = stage2_debug.exec
stage2_debug_exec_SOURCES = dummy.c # needed to drag in C Makefile variables
DISTFILES = $(DIST_COMMON)

stage2_debug.exec: $(stage2_debug_exec_LDADD)
	$(LD) -N -Ttext 8000 -o stage2_debug.exec $(stage2_debug_exec_LDADD)

stage2_debug: stage2_debug.exec
	$(OBJCOPY) -O binary stage2_debug.exec stage2_debug

@SHARED_SRC_RULES@
