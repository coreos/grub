AUTOMAKE_OPTIONS = gnits

pkglibdir = $(exec_prefix)/lib/$(PACKAGE)
pkglib_DATA = stage2
CLEANFILES = stage2
MOSTLYCLEANFILES = stage2.exec

# We can't use builtins.
COMPILE = $(CC) -fno-builtin $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS)
INCLUDES = -I$(top_srcdir)/shared_src

# asm.o absolutely needs to come first!
stage2_exec_LDADD = asm.o boot.o common.o char_io.o cmdline.o disk_io.o \
	gunzip.o fsys_ffs.o fsys_ext2fs.o fsys_fat.o stage2.o

noinst_PROGRAMS = stage2.exec
stage2_exec_SOURCES = dummy.c # needed to drag in C Makefile variables
DISTFILES = $(DIST_COMMON)

stage2.exec: $(stage2_exec_LDADD)
	$(LD) -N -Ttext 8000 -o stage2.exec $(stage2_exec_LDADD)

stage2: stage2.exec
	$(OBJCOPY) -O binary stage2.exec stage2

@SHARED_SRC_RULES@
