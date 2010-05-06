AUTOMAKE_OPTIONS = subdir-objects
SUBDIRS = po
DEPDIR = .deps-core

include $(top_srcdir)/Makefile.extra-dist
include $(top_srcdir)/Makefile.vars

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
AM_CPPFLAGS = $(CPPFLAGS_GRUB) -DGRUB_FILE=\"$(subst $(top_srcdir)/,,$<)\"
AM_CCASFLAGS = -DASM_FILE=1

# gentrigtables
gentrigtables: $(top_srcdir)/gentrigtables.c
	$(BUILD_CC) -o $@ -I$(top_srcdir)/include $(CPPFLAGS) -lm $<
CLEANFILES += gentrigtables

# trigtables.c
trigtables.c: gentrigtables.c configure.ac
	$(MAKE) $(AM_MAKEFLAGS) gentrigtables
	$(top_builddir)/gentrigtables > $@
CLEANFILES += trigtables.c

# XXX Use Automake's LEX & YACC support
# See Recording Dependencies Manually in automake doc for below rules
script/sh_module-lexer.$(OBJEXT):grub_script.tab.h
grub_script.tab.c grub_script.tab.h: $(top_srcdir)/script/parser.y
	$(YACC) -d -p grub_script_yy -b grub_script $(top_srcdir)/script/parser.y
CLEANFILES += grub_script.tab.c grub_script.tab.h

# For the lexer.
# See Recording Dependencies Manually in automake doc for below rules
script/sh_module-lexer.$(OBJEXT):grub_script.yy.h
grub_script.yy.c grub_script.yy.h: $(top_srcdir)/script/yylex.l
	$(LEX) -o grub_script.yy.c --header-file=grub_script.yy.h $(top_srcdir)/script/yylex.l
CLEANFILES += grub_script.yy.c grub_script.yy.h

include $(srcdir)/modules.am
include $(srcdir)/Makefile.kernel

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
CLEANFILES += fs.lst
CLEANFILES += command.lst
CLEANFILES += partmap.lst
CLEANFILES += handler.lst
CLEANFILES += terminal.lst
CLEANFILES += parttool.lst
CLEANFILES += video.lst
CLEANFILES += crypto.lst

fs.lst: $(FS_FILES)
	cat $^ /dev/null | sort | uniq > $@
command.lst: $(COMMAND_FILES)
	cat $^ /dev/null | sort | uniq > $@
partmap.lst: $(PARTMAP_FILES)
	cat $^ /dev/null | sort | uniq > $@
handler.lst: $(HANDLER_FILES)
	cat $^ /dev/null | sort | uniq > $@
terminal.lst: $(TERMINAL_FILES)
	cat $^ /dev/null | sort | uniq > $@
parttool.lst: $(PARTTOOL_FILES)
	cat $^ /dev/null | sort | uniq > $@
video.lst: $(VIDEO_FILES)
	cat $^ /dev/null | sort | uniq > $@

# but, crypto.lst is simply copied
crypto.lst: $(top_srcdir)/lib/libgcrypt-grub/cipher/crypto.lst
	cp $^ $@

# generate global module dependencies list
moddep.lst: kernel_syms.lst genmoddep.awk $(DEF_FILES) $(UND_FILES)
	cat $(DEF_FILES) kernel_syms.lst /dev/null \
	  | $(AWK) -f $(top_srcdir)/genmoddep.awk $(UND_FILES) > $@ \
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

