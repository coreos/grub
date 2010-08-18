AUTOMAKE_OPTIONS = subdir-objects

CC=$(TARGET_CC)
CPP=$(TARGET_CC)
CCAS=$(TARGET_CC)

EXTRA_DIST =
EXTRA_DIST += modules.def genmoddep.awk
EXTRA_DIST += genmodsrc.sh gensymlist.sh genemuinit.sh genemuinitheader.sh
EXTRA_DIST += genfslist.sh gencmdlist.sh genvideolist.sh genhandlerlist.sh
EXTRA_DIST += genpartmaplist.sh genparttoollist.sh genterminallist.sh
EXTRA_DIST += lib/libgcrypt_wrap/cipher_wrap.h
EXTRA_DIST += lib/libgcrypt/cipher/rijndael-tables.h
EXTRA_DIST += lib/libgcrypt/cipher/bithelp.h
EXTRA_DIST += lib/libgcrypt/cipher/rmd.h
EXTRA_DIST += lib/libgcrypt/cipher/hash-common.h
EXTRA_DIST += lib/libgcrypt/cipher/camellia.h
EXTRA_DIST += lib/posix_wrap/localcharset.h
EXTRA_DIST += lib/posix_wrap/ctype.h
EXTRA_DIST += lib/posix_wrap/limits.h
EXTRA_DIST += lib/posix_wrap/stdio.h
EXTRA_DIST += lib/posix_wrap/sys/types.h
EXTRA_DIST += lib/posix_wrap/unistd.h
EXTRA_DIST += lib/posix_wrap/locale.h
EXTRA_DIST += lib/posix_wrap/wchar.h
EXTRA_DIST += lib/posix_wrap/string.h
EXTRA_DIST += lib/posix_wrap/langinfo.h
EXTRA_DIST += lib/posix_wrap/wctype.h
EXTRA_DIST += lib/posix_wrap/stdint.h
EXTRA_DIST += lib/posix_wrap/stdlib.h
EXTRA_DIST += lib/posix_wrap/assert.h
EXTRA_DIST += lib/posix_wrap/errno.h
EXTRA_DIST += lib/libgcrypt-grub/cipher/types.h
EXTRA_DIST += lib/libgcrypt-grub/cipher/cipher.h
EXTRA_DIST += lib/libgcrypt-grub/cipher/rijndael-tables.h
EXTRA_DIST += lib/libgcrypt-grub/cipher/memory.h
EXTRA_DIST += lib/libgcrypt-grub/cipher/bithelp.h
EXTRA_DIST += lib/libgcrypt-grub/cipher/g10lib.h
EXTRA_DIST += lib/libgcrypt-grub/cipher/rmd.h
EXTRA_DIST += lib/libgcrypt-grub/cipher/hash-common.h
EXTRA_DIST += lib/libgcrypt-grub/cipher/camellia.h
EXTRA_DIST += efiemu/runtime/config.h
EXTRA_DIST += gnulib/getopt.h
EXTRA_DIST += gnulib/argp-version-etc.h
EXTRA_DIST += gnulib/fnmatch.h
EXTRA_DIST += gnulib/error.h
EXTRA_DIST += gnulib/argp-namefrob.h
EXTRA_DIST += gnulib/argp.h
EXTRA_DIST += gnulib/argp-fmtstream.h
EXTRA_DIST += gnulib/gettext.h
EXTRA_DIST += gnulib/regex_internal.h
EXTRA_DIST += gnulib/progname.h
EXTRA_DIST += gnulib/regex.h
EXTRA_DIST += gnulib/alloca.h
EXTRA_DIST += gnulib/getopt_int.h

grubconfdir = $(sysconfdir)/grub.d
platformdir = $(pkglibrootdir)/$(target_cpu)-$(platform)

# to calm down automake
BUILT_SOURCES =
CLEANFILES =
COMMAND_FILES =
DEF_FILES =
FS_FILES =
HANDLER_FILES =
IMG_FILES =
MOD_FILES =
MODULE_FILES =
PARTMAP_FILES =
PARTTOOL_FILES =
TERMINAL_FILES =
TESTS =
UND_FILES =
VIDEO_FILES =
bin_PROGRAMS =
bin_SCRIPTS =
check_PROGRAMS =
check_SCRIPTS =
grubconf_DATA =
grubconf_SCRIPTS =
man_MANS =
noinst_DATA =
noinst_LIBRARIES =
noinst_PROGRAMS =
pkglib_SCRIPTS =
platform_DATA =
sbin_PROGRAMS =
sbin_SCRIPTS =
KERNEL_HEADER_FILES =

# Platform specific options
if COND_i386_pc
  CFLAGS_PLATFORM = -mrtd -mregparm=3
endif
if COND_i386_efi
  LDFLAGS_PLATFORM = -melf_i386
endif
if COND_x86_64_efi
  LDFLAGS_PLATFORM = -melf_x86_64
endif
if COND_i386_qemu
  CFLAGS_PLATFORM = -mrtd -mregparm=3
endif
if COND_i386_coreboot
  CFLAGS_PLATFORM = -mrtd -mregparm=3
endif
if COND_i386_ieee1275
  CFLAGS_PLATFORM = -mrtd -mregparm=3
endif
if COND_mips_yeeloong
  CFLAGS_PLATFORM = -march=mips3 -mexplicit-relocs -mflush-func=grub_cpu_flush_cache
  CCASFLAGS_PLATFORM = -march=mips3
endif
if COND_sparc64_ieee1275
  CFLAGS_PLATFORM = -mno-app-regs
  LDFLAGS_PLATFORM = -melf64_sparc -mno-relax
endif

CPPFLAGS_GRUB = -DGRUB_FILE=\"`basename $<`\"
CPPFLAGS_GRUB += -I$(builddir) -I$(srcdir) -I$(top_builddir) -I$(top_srcdir)
CPPFLAGS_GRUB += -I$(top_srcdir)/include
CPPFLAGS_GRUB += -I$(top_builddir)/include
CCASFLAGS_GRUB = -DASM_FILE=1

CFLAGS_GCRY = -Wno-error -Wno-missing-field-initializers
CPPFLAGS_GCRY = -I$(top_srcdir)/grub-core/lib/libgcrypt_wrap

CFLAGS_GNULIB = -Wno-undef -Wno-sign-compare -Wno-unused -D_GL_UNUSED="__attribute__ ((unused))"
CPPFLAGS_GNULIB = -I$(top_srcdir)/grub-core/gnulib

CFLAGS_MKISOFS = -Wno-all -Werror
CPPFLAGS_MKISOFS = -D_FILE_OFFSET_BITS=64 -I$(top_srcdir)/util/mkisofs/include

CFLAGS_POSIX = -fno-builtin
CPPFLAGS_POSIX = -I$(top_srcdir)/grub-core/lib/posix_wrap

CPPFLAGS_EFIEMU = -I$(top_srcdir)/grub-core/efiemu/runtime

LDADD_KERNEL = -lgcc
CFLAGS_KERNEL = $(TARGET_CFLAGS) $(CFLAGS_CPU) $(CFLAGS_PLATFORM) -ffreestanding
LDFLAGS_KERNEL = $(TARGET_LDFLAGS) $(LDFLAGS_CPU) $(LDFLAGS_PLATFORM) -nostdlib -Wl,-N -static-libgcc
CPPFLAGS_KERNEL = $(TARGET_CPPFLAGS) $(CPPFLAGS_CPU) $(CPPFLAGS_PLATFORM)
CCASFLAGS_KERNEL = $(TARGET_CCASFLAGS) $(CCASFLAGS_CPU) $(CCASFLAGS_PLATFORM)

CFLAGS_MODULE = $(TARGET_CFLAGS) $(CFLAGS_CPU) $(CFLAGS_PLATFORM) -ffreestanding
LDFLAGS_MODULE = $(TARGET_LDFLAGS) $(LDFLAGS_CPU) $(LDFLAGS_PLATFORM) -nostdlib -Wl,-N,-r,-d
CPPFLAGS_MODULE = $(TARGET_CPPFLAGS) $(CPPFLAGS_CPU) $(CPPFLAGS_PLATFORM)
CCASFLAGS_MODULE = $(TARGET_CCASFLAGS) $(CCASFLAGS_CPU) $(CCASFLAGS_PLATFORM)

CFLAGS_IMAGE = $(TARGET_CFLAGS) $(CFLAGS_CPU) $(CFLAGS_PLATFORM) -fno-builtin
LDFLAGS_IMAGE = $(TARGET_LDFLAGS) $(LDFLAGS_CPU) $(LDFLAGS_PLATFORM) -nostdlib -Wl,-N,-S
CPPFLAGS_IMAGE = $(TARGET_CPPFLAGS) $(CPPFLAGS_CPU) $(CPPFLAGS_PLATFORM)
CCASFLAGS_IMAGE = $(TARGET_CCASFLAGS) $(CCASFLAGS_CPU) $(CCASFLAGS_PLATFORM)

CFLAGS_LIBRARY = $(TARGET_CFLAGS) $(CFLAGS_CPU) $(CFLAGS_PLATFORM) -fno-builtin
CPPFLAGS_LIBRARY = $(TARGET_CPPFLAGS) $(CPPFLAGS_CPU) $(CPPFLAGS_PLATFORM)
CCASFLAGS_LIBRARY = $(TARGET_CCASFLAGS) $(CCASFLAGS_CPU) $(CCASFLAGS_PLATFORM)

CFLAGS_PROGRAM = $(TARGET_CFLAGS) $(CFLAGS_CPU) $(CFLAGS_PLATFORM)
LDFLAGS_PROGRAM = $(TARGET_LDFLAGS) $(LDFLAGS_CPU) $(LDFLAGS_PLATFORM)
CPPFLAGS_PROGRAM = $(TARGET_CPPFLAGS) $(CPPFLAGS_CPU) $(CPPFLAGS_PLATFORM)
CCASFLAGS_PROGRAM = $(TARGET_CCASFLAGS) $(CCASFLAGS_CPU) $(CCASFLAGS_PLATFORM)

AM_CFLAGS =
AM_LDFLAGS =
AM_CPPFLAGS = $(CPPFLAGS_GRUB)
AM_CCASFLAGS = $(CCASFLAGS_GRUB)

# gentrigtables
gentrigtables: gentrigtables.c
	$(BUILD_CC) -o $@ -I$(top_srcdir)/include $(CPPFLAGS) -lm $<
CLEANFILES += gentrigtables

# trigtables.c
trigtables.c: gentrigtables.c $(top_srcdir)/configure.ac
	$(MAKE) $(AM_MAKEFLAGS) gentrigtables
	$(builddir)/gentrigtables > $@
CLEANFILES += trigtables.c

# XXX Use Automake's LEX & YACC support
grub_script.tab.h: script/parser.y
	$(YACC) -d -p grub_script_yy -b grub_script $<
grub_script.tab.c: grub_script.tab.h
CLEANFILES += grub_script.tab.c grub_script.tab.h

# For the lexer.
grub_script.yy.h: script/yylex.l
	$(LEX) -o grub_script.yy.c --header-file=grub_script.yy.h $<
grub_script.yy.c: grub_script.yy.h
CLEANFILES += grub_script.yy.c grub_script.yy.h

include $(srcdir)/modules.am

KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/cache.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/command.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/device.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/disk.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/dl.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/elf.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/elfload.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/env.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/env_private.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/err.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/file.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/fs.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/i18n.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/kernel.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/list.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/misc.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/mm.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/net.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/parser.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/partition.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/reader.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/symbol.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/term.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/time.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/types.h

if COND_i386_pc
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/boot.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/loader.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/msdos_partition.h
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/machine/biosdisk.h
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/machine/boot.h
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/machine/console.h
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/machine/memory.h
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/machine/loader.h
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/machine/vga.h
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/machine/vbe.h
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/machine/kernel.h
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/machine/pxe.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/i386/pit.h
endif

if COND_i386_efi
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/boot.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/loader.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/msdos_partition.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/efi/efi.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/efi/time.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/efi/disk.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/i386/pit.h
endif

if COND_i386_coreboot
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/boot.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/loader.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/msdos_partition.h
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/machine/boot.h
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/machine/console.h
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/machine/init.h
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/machine/memory.h
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/machine/loader.h
endif

if COND_i386_multiboot
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/boot.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/loader.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/msdos_partition.h
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/machine/boot.h
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/machine/console.h
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/machine/init.h
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/machine/memory.h
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/machine/loader.h
endif

if COND_i386_qemu
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/boot.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/loader.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/msdos_partition.h
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/machine/boot.h
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/machine/console.h
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/machine/init.h
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/machine/memory.h
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/machine/loader.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/pci.h
endif

if COND_i386_ieee1275
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/loader.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/msdos_partition.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/ieee1275/ieee1275.h
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/machine/loader.h
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/machine/memory.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/terminfo.h
endif

if COND_x86_64_efi
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/boot.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/loader.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/msdos_partition.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/efi/efi.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/efi/time.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/efi/disk.h
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/machine/loader.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/i386/pit.h
endif

if COND_mips_yeeloong
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/boot.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/loader.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/msdos_partition.h
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/machine/kernel.h
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/machine/memory.h
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/cpu/cache.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/bitmap.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/video.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/gfxterm.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/font.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/bitmap_scale.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/bufio.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/pci.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/libgcc.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/cs5536.h
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/machine/pci.h
endif

if COND_powerpc_ieee1275
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/boot.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/loader.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/msdos_partition.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/ieee1275/ieee1275.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/libgcc.h
endif

if COND_sparc64_ieee1275
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/boot.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/loader.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/msdos_partition.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/libgcc.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/ieee1275/ieee1275.h
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/machine/kernel.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/sparc64/ieee1275/ieee1275.h
endif

if COND_emu
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/cpu/time.h
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/cpu/types.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/gzio.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/menu.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/datetime.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/emu/misc.h
if COND_GRUB_EMU_SDL
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/sdl.h
endif
if COND_GRUB_EMU_USB
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/libusb.h
endif
if COND_GRUB_EMU_PCI
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/libpciaccess.h
endif
endif

symlist.h: $(top_builddir)/config.h $(KERNEL_HEADER_FILES)
	@list='$^'; \
	for p in $$list; do \
	  echo "#include <$$p>" >> $@ || (rm -f $@; exit 1); \
	done
CLEANFILES += symlist.h
BUILT_SOURCES += symlist.h

symlist.c: symlist.h gensymlist.sh
	$(TARGET_CPP) $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS_KERNEL) $(CPPFLAGS) -DGRUB_SYMBOL_GENERATOR=1 symlist.h > symlist.p || (rm -f symlist.p; exit 1)
	cat symlist.p | /bin/sh $(srcdir)/gensymlist.sh $(top_builddir)/config.h $(KERNEL_HEADER_FILES) >$@ || (rm -f $@; exit 1)
	rm -f symlist.p
CLEANFILES += symlist.c
BUILT_SOURCES += symlist.c

noinst_DATA += kernel_syms.lst
kernel_syms.lst: $(KERNEL_HEADER_FILES) $(top_builddir)/config.h
	$(TARGET_CPP) $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS_KERNEL) $(CPPFLAGS) $(CFLAGS) -DGRUB_SYMBOL_GENERATOR=1 $^ >kernel_syms.input
	if grep "^#define HAVE_ASM_USCORE" $(top_builddir)/config.h; then u="_"; else u=""; fi; \
	cat kernel_syms.input | grep -v '^#' | sed -n \
	  -e '/EXPORT_FUNC *([a-zA-Z0-9_]*)/{s/.*EXPORT_FUNC *(\([a-zA-Z0-9_]*\)).*/'"$$u"'\1 kernel/;p;}' \
	  -e '/EXPORT_VAR *([a-zA-Z0-9_]*)/{s/.*EXPORT_VAR *(\([a-zA-Z0-9_]*\)).*/'"$$u"'\1 kernel/;p;}' \
	  | sort -u >$@
	rm -f kernel_syms.input
CLEANFILES += kernel_syms.lst

if COND_emu
kern/emu/grub_emu-main.$(OBJEXT):grub_emu_init.h
grub_emu-grub_emu_init.$(OBJEXT):grub_emu_init.h
kern/emu/grub_emu_dyn-main.$(OBJEXT):grub_emu_init.h
grub_emu_dyn-grub_emu_init.$(OBJEXT):grub_emu_init.h

grub_emu_init.h: genemuinitheader.sh $(MOD_FILES)
	rm -f $@; echo $(MOD_FILES) | sh $(srcdir)/genemuinitheader.sh $(NM) > $@
CLEANFILES += grub_emu_init.h

grub_emu_init.c: grub_emu_init.h genemuinit.sh $(MOD_FILES)
	rm -f $@; echo $(MOD_FILES) | sh $(srcdir)/genemuinit.sh $(NM) > $@
CLEANFILES += grub_emu_init.c
endif

# .lst files
platform_DATA += moddep.lst
platform_DATA += fs.lst
platform_DATA += command.lst
platform_DATA += partmap.lst
platform_DATA += handler.lst
platform_DATA += terminal.lst
platform_DATA += parttool.lst
platform_DATA += video.lst
platform_DATA += crypto.lst
CLEANFILES += moddep.lst
CLEANFILES += handler.lst
CLEANFILES += terminal.lst
CLEANFILES += parttool.lst
CLEANFILES += video.lst
CLEANFILES += crypto.lst

fs.lst: $(FS_FILES)
	cat $^ /dev/null | sort | uniq > $@
CLEANFILES += fs.lst

command.lst: $(COMMAND_FILES)
	cat $^ /dev/null | sort | uniq > $@
CLEANFILES += command.lst

partmap.lst: $(PARTMAP_FILES)
	cat $^ /dev/null | sort | uniq > $@
CLEANFILES += partmap.lst

handler.lst: $(HANDLER_FILES)
	cat $^ /dev/null | sort | uniq > $@
CLEANFILES += handler.lst

terminal.lst: $(TERMINAL_FILES)
	cat $^ /dev/null | sort | uniq > $@
CLEANFILES += terminal.lst

parttool.lst: $(PARTTOOL_FILES)
	cat $^ /dev/null | sort | uniq > $@
CLEANFILES += parttool.lst

video.lst: $(VIDEO_FILES)
	cat $^ /dev/null | sort | uniq > $@
CLEANFILES += video.lst

# but, crypto.lst is simply copied
crypto.lst: $(srcdir)/lib/libgcrypt-grub/cipher/crypto.lst
	cp $^ $@
CLEANFILES += crypto.lst

# generate global module dependencies list
moddep.lst: kernel_syms.lst genmoddep.awk $(DEF_FILES) $(UND_FILES)
	cat $(DEF_FILES) kernel_syms.lst /dev/null \
	  | $(AWK) -f $(srcdir)/genmoddep.awk $(UND_FILES) > $@ \
	  || (rm -f $@; exit 1)

if COND_i386_pc
if COND_ENABLE_EFIEMU
efiemu32.o: efiemu/runtime/efiemu.c $(TARGET_OBJ2ELF)
	-rm -f $@; \
	if test "x$(TARGET_APPLE_CC)" = x1; then \
	  $(TARGET_CC) $(DEFS) $(INCLUDES) $(CPPFLAGS_EFIEMU) $(CPPFLAGS_GRUB) -DELF32 -DAPPLE_CC -m32 -Wall -Werror -nostdlib -O2 -c -o $@.bin $< || exit 1; \
	  $(OBJCONV) -felf32 -nu -nd $@.bin $@ || exit 1; \
	  rm -f $@.bin; \
	else \
	  $(TARGET_CC) $(DEFS) $(INCLUDES) $(CPPFLAGS_EFIEMU) $(CPPFLAGS_GRUB) -DELF32 -m32 -Wall -Werror -nostdlib -O2 -c -o $@ $< || exit 1; \
	  if test ! -z "$(TARGET_OBJ2ELF)"; then $(TARGET_OBJ2ELF) $@ || (rm -f $@; exit 1); fi; \
	fi

efiemu64_c.o: efiemu/runtime/efiemu.c
	if test "x$(TARGET_APPLE_CC)" = x1; then \
	  $(TARGET_CC) $(DEFS) $(INCLUDES) $(CPPFLAGS_EFIEMU) $(CPPFLAGS_GRUB) -DELF64 -DAPPLE_CC=1 -m64 -nostdlib -Wall -Werror -mno-red-zone -c -o $@ $< || exit 1; \
	else \
	  $(TARGET_CC) $(DEFS) $(INCLUDES) $(CPPFLAGS_EFIEMU) $(CPPFLAGS_GRUB) -DELF64 -m64 -nostdlib -Wall -Werror -O2 -mcmodel=large -mno-red-zone -c -o $@ $< || exit 1; \
	fi

efiemu64_s.o: efiemu/runtime/efiemu.S
	-rm -f $@
	if test "x$(TARGET_APPLE_CC)" = x1; then \
	  $(TARGET_CC) $(DEFS) $(INCLUDES) $(CPPFLAGS_EFIEMU) $(CPPFLAGS_GRUB) -DELF64 -DAPPLE_CC=1 -m64 -Wall -Werror -nostdlib -O2 -mno-red-zone -c -o $@ $< || exit 1; \
	else \
	  $(TARGET_CC) $(DEFS) $(INCLUDES) $(CPPFLAGS_EFIEMU) $(CPPFLAGS_GRUB) -DELF64 -m64 -Wall -Werror -nostdlib -O2 -mcmodel=large -mno-red-zone -c -o $@ $< || exit 1; \
	fi

efiemu64.o: efiemu64_c.o efiemu64_s.o $(TARGET_OBJ2ELEF)
	-rm -f $@; \
	if test "x$(TARGET_APPLE_CC)" = x1; then \
	  rm -f $@.bin; \
	  $(TARGET_CC) -m64 -Wl,-r -nostdlib -o $@.bin $^ || exit 1; \
	  $(OBJCONV) -felf64 -nu -nd $@.bin $@ || exit 1; \
	  rm -f $@.bin; \
	else \
	  $(TARGET_CC) -m64 -nostdlib -Wl,-r -o $@ $^ || exit 1; \
	  if test ! -z "$(TARGET_OBJ2ELF)"; then $(TARGET_OBJ2ELF) $@ || (rm -f $@; exit 1); fi; \
	fi

platform_DATA += efiemu32.o efiemu64.o
CLEANFILES += efiemu32.o efiemu64.o efiemu64_c.o efiemu64_s.o
endif
endif

