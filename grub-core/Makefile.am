AUTOMAKE_OPTIONS = subdir-objects

DEPDIR=.deps-core

include $(top_srcdir)/conf/Makefile.common

CC=$(TARGET_CC)
CPP=$(TARGET_CC)
CCAS=$(TARGET_CC)

if COND_GRUB_MKFONT
if COND_HAVE_FONT_SOURCE
TARGET_CFLAGS += -DUSE_ASCII_FAILBACK=1 -DHAVE_UNIFONT_WIDTHSPEC=1
endif
endif

AM_CFLAGS = $(TARGET_CFLAGS)
AM_LDFLAGS = $(TARGET_LDFLAGS)
AM_CPPFLAGS = $(TARGET_CPPFLAGS) $(CPPFLAGS_DEFAULT)
AM_CCASFLAGS = $(TARGET_CCASFLAGS) $(CCASFLAGS_DEFAULT)

CFLAGS_PROGRAM += $(CFLAGS_PLATFORM)
LDFLAGS_PROGRAM += $(LDFLAGS_PLATFORM)
CPPFLAGS_PROGRAM += $(CPPFLAGS_PLATFORM)
CCASFLAGS_PROGRAM += $(CCASFLAGS_PLATFORM)

CFLAGS_LIBRARY += $(CFLAGS_PLATFORM) -fno-builtin
CPPFLAGS_LIBRARY += $(CPPFLAGS_PLATFORM)
CCASFLAGS_LIBRARY += $(CCASFLAGS_PLATFORM)

# gentrigtables
gentrigtables: gentrigtables.c
	$(BUILD_CC) -o $@ -I$(top_srcdir)/include $(CPPFLAGS) -lm $<
CLEANFILES += gentrigtables

# trigtables.c
trigtables.c: gentrigtables gentrigtables.c $(top_srcdir)/configure.ac
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

include $(srcdir)/Makefile.core.am
include $(srcdir)/Makefile.gcry.am

KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/cache.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/command.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/device.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/disk.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/dl.h
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
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/parser.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/partition.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/term.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/time.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/mm_private.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/boot.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/libgcc.h

if COND_i386_pc
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/machine/memory.h
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/machine/loader.h
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/machine/kernel.h
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/machine/pxe.h
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/machine/int.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/i386/pit.h
endif

if COND_i386_efi
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/efi/efi.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/efi/time.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/efi/disk.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/i386/pit.h
endif

if COND_i386_coreboot
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/machine/memory.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/i386/pit.h
endif

if COND_i386_multiboot
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/machine/memory.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/i386/pit.h
endif

if COND_i386_qemu
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/machine/memory.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/pci.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/i386/pit.h
endif

if COND_i386_ieee1275
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/ieee1275/ieee1275.h
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/machine/memory.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/terminfo.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/i386/pit.h
endif

if COND_x86_64_efi
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/efi/efi.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/efi/time.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/efi/disk.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/i386/pit.h
endif


if COND_ia64_efi
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/efi/efi.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/efi/time.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/efi/disk.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/ia64/efi/misc.h
endif

if COND_mips_yeeloong
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
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/cs5536.h
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/machine/pci.h
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/serial.h
endif

if COND_powerpc_ieee1275
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/ieee1275/ieee1275.h
endif

if COND_sparc64_ieee1275
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/ieee1275/ieee1275.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/sparc64/ieee1275/ieee1275.h
endif

if COND_emu
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

if COND_ENABLE_EFIEMU
efiemu32.o: efiemu/runtime/efiemu.c $(TARGET_OBJ2ELF)
	-rm -f $@; \
	if test "x$(TARGET_APPLE_CC)" = x1; then \
	  $(TARGET_CC) $(DEFS) $(INCLUDES) $(CPPFLAGS_EFIEMU) $(CPPFLAGS_DEFAULT) -DELF32 -DAPPLE_CC -m32 -Wall -Werror -nostdlib -O2 -c -o $@.bin $< || exit 1; \
	  $(OBJCONV) -felf32 -nu -nd $@.bin $@ || exit 1; \
	  rm -f $@.bin; \
	else \
	  $(TARGET_CC) $(DEFS) $(INCLUDES) $(CPPFLAGS_EFIEMU) $(CPPFLAGS_DEFAULT) -DELF32 -m32 -Wall -Werror -nostdlib -O2 -c -o $@ $< || exit 1; \
	  if test ! -z "$(TARGET_OBJ2ELF)"; then $(TARGET_OBJ2ELF) $@ || (rm -f $@; exit 1); fi; \
	fi

efiemu64_c.o: efiemu/runtime/efiemu.c
	if test "x$(TARGET_APPLE_CC)" = x1; then \
	  $(TARGET_CC) $(DEFS) $(INCLUDES) $(CPPFLAGS_EFIEMU) $(CPPFLAGS_DEFAULT) -DELF64 -DAPPLE_CC=1 -m64 -nostdlib -Wall -Werror -mno-red-zone -c -o $@ $< || exit 1; \
	else \
	  $(TARGET_CC) $(DEFS) $(INCLUDES) $(CPPFLAGS_EFIEMU) $(CPPFLAGS_DEFAULT) -DELF64 -m64 -nostdlib -Wall -Werror -O2 -mcmodel=large -mno-red-zone -c -o $@ $< || exit 1; \
	fi

efiemu64_s.o: efiemu/runtime/efiemu.S
	-rm -f $@
	if test "x$(TARGET_APPLE_CC)" = x1; then \
	  $(TARGET_CC) $(DEFS) $(INCLUDES) $(CPPFLAGS_EFIEMU) $(CPPFLAGS_DEFAULT) -DELF64 -DAPPLE_CC=1 -m64 -Wall -Werror -nostdlib -O2 -mno-red-zone -c -o $@ $< || exit 1; \
	else \
	  $(TARGET_CC) $(DEFS) $(INCLUDES) $(CPPFLAGS_EFIEMU) $(CPPFLAGS_DEFAULT) -DELF64 -m64 -Wall -Werror -nostdlib -O2 -mcmodel=large -mno-red-zone -c -o $@ $< || exit 1; \
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
