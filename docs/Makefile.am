info_TEXINFOS = multiboot.texi
EXAMPLES = boot.S kernel.c multiboot.h
multiboot_TEXINFOS = boot.S.texi kernel.c.texi multiboot.h.texi
SRC2TEXI = src2texi
noinst_SCRIPTS = $(SRC2TEXI)
EXTRA_PROGRAMS = kernel

# The example kernel is built if you specify --enable-example-kernel.
if BUILD_EXAMPLE_KERNEL
noinst_PROGRAMS = kernel
kernel_SOURCES = $(EXAMPLES)
kernel_CFLAGS = -fno-builtin -nostdinc -O -g -Wall \
	-imacros $(top_builddir)/config.h
kernel_LDFLAGS = -nostdlib -Wl,-N -Wl,-Ttext -Wl,100000 -Wl,--build-id=none

boot.o: multiboot.h
endif

EXTRA_DIST = $(man_MANS) $(noinst_SCRIPTS) \
	$(EXAMPLES) $(multiboot_TEXINFOS)
CLEANFILES = $(noinst_PROGRAMS)

# Cancel the rule %.texi -> %. This rule may confuse make to determine
# the dependecies.
.texi:

%.c.texi: %.c $(srcdir)/$(SRC2TEXI)
	$(SHELL) $(srcdir)/$(SRC2TEXI) $(srcdir) $< $@

%.h.texi: %.h $(srcdir)/$(SRC2TEXI)
	$(SHELL) $(srcdir)/$(SRC2TEXI) $(srcdir) $< $@

%.S.texi: %.S $(srcdir)/$(SRC2TEXI)
	$(SHELL) $(srcdir)/$(SRC2TEXI) $(srcdir) $< $@
