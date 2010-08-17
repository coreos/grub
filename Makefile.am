AUTOMAKE_OPTIONS = subdir-objects

SUBDIRS = . grub-core po docs
EXTRA_DIST = autogen.sh gentpl.py Makefile.tpl modules.def geninit.sh

grubconfdir = $(sysconfdir)/grub.d
platformdir = $(pkglibrootdir)/$(target_cpu)-$(platform)

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

CFLAGS_PROGRAM = $(HOST_CFLAGS) $(CFLAGS_GNULIB)
LDFLAGS_PROGRAM = $(HOST_LDFLAGS) $(LDFLAGS_GNULIB)
CPPFLAGS_PROGRAM = $(HOST_CPPFLAGS) $(CPPFLAGS_GNULIB)
CCASFLAGS_PROGRAM = $(HOST_CCASFLAGS) $(CCASFLAGS_GNULIB)

CFLAGS_LIBRARY = $(CFLAGS_PROGRAM)
CPPFLAGS_LIBRARY = $(CPPFLAGS_PROGRAM)
CCASFLAGS_LIBRARY = $(CCASFLAGS_PROGRAM)

AM_CFLAGS =
AM_LDFLAGS =
AM_CPPFLAGS = $(CPPFLAGS_GRUB)
AM_CCASFLAGS = $(CCASFLAGS_GRUB)

include $(srcdir)/modules.am

# XXX Use Automake's LEX & YACC support
grub_script.tab.h: $(top_srcdir)/grub-core/script/parser.y
	$(YACC) -d -p grub_script_yy -b grub_script $(top_srcdir)/grub-core/script/parser.y
grub_script.tab.c: grub_script.tab.h
CLEANFILES += grub_script.tab.c grub_script.tab.h

# For the lexer.
grub_script.yy.h: $(top_srcdir)/grub-core/script/yylex.l
	$(LEX) -o grub_script.yy.c --header-file=grub_script.yy.h $(top_srcdir)/grub-core/script/yylex.l
grub_script.yy.c: grub_script.yy.h
CLEANFILES += grub_script.yy.c grub_script.yy.h

# For libgrub.a
libgrub.pp: grub_script.tab.h grub_script.yy.h $(libgrub_a_SOURCES)
	$(CPP) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(libgrub_a_CPPFLAGS) $(CPPFLAGS) \
	  -D'GRUB_MOD_INIT(x)=@MARKER@x@' $^ > $@ || (rm -f $@; exit 1)
CLEANFILES += libgrub.pp

libgrub_a_init.lst: libgrub.pp
	cat $< | grep '@MARKER@' | sed 's/@MARKER@\(.*\)@/\1/g' | sort -u > $@ || (rm -f $@; exit 1)
CLEANFILES += libgrub_a_init.lst

libgrub_a_init.c: libgrub_a_init.lst $(top_srcdir)/geninit.sh
	sh $(top_srcdir)/geninit.sh `cat $<` > $@ || (rm -f $@; exit 1)
CLEANFILES += libgrub_a_init.c

if COND_GRUB_MKFONT
if COND_HAVE_FONT_SOURCE
pkgdata_DATA = unicode.pf2 ascii.pf2 ascii.h
endif
endif

unicode.pf2: $(FONT_SOURCE) grub-mkfont
	$(builddir)/grub-mkfont -o $@ $(FONT_SOURCE)
CLEANFILES += unicode.pf2

# Arrows and lines are needed to draw the menu, so always include them
UNICODE_ARROWS=0x2190-0x2193
UNICODE_LINES=0x2501-0x251B

ascii.pf2: $(FONT_SOURCE) grub-mkfont
	$(builddir)/grub-mkfont -o $@ $(FONT_SOURCE) -r 0x0-0x7f,$(UNICODE_ARROWS),$(UNICODE_LINES)
CLEANFILES += ascii.pf2

ascii.bitmaps: $(FONT_SOURCE) grub-mkfont
	$(builddir)/grub-mkfont --ascii-bitmaps -o $@ $(FONT_SOURCE)
CLEANFILES += ascii.bitmaps

ascii.h: ascii.bitmaps grub-bin2h
	$(builddir)/grub-bin2h ascii_bitmaps < $< > $@
	cp $@ $(top_builddir)/grub-core/include
CLEANFILES += ascii.h $(top_builddir)/grub-core/include/ascii.h

widthspec.bin: $(FONT_SOURCE) grub-mkfont
	$(builddir)/grub-mkfont --width-spec -o $@ $(FONT_SOURCE)
CLEANFILES += widthspec.bin

widthspec.h: widthspec.bin grub-bin2h
	$(builddir)/grub-bin2h widthspec < $< > $@
CLEANFILES += widthspec.h

# XXX: TARGET_CFLAGS += -DUSE_ASCII_FAILBACK=1 -DHAVE_UNIFONT_WIDTHSPEC=1

# Install config.h into platformdir
platform_HEADERS = config.h
