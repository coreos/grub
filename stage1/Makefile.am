AUTOMAKE_OPTIONS = gnits

pkglibdir = $(exec_prefix)/lib/$(PACKAGE)
pkglib_DATA = stage1
CLEANFILES = stage1

# We can't use builtins or standard includes.
COMPILE = $(CC) -fno-builtin -nostdinc $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS)

noinst_PROGRAMS = stage1.exec
stage1_exec_SOURCES = stage1.S

stage1.exec: stage1.o
	$(LD) -N -Ttext 7C00 -o $@ $<

stage1: stage1.exec
	$(OBJCOPY) -O binary $< $@
