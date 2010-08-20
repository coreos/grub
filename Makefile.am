AUTOMAKE_OPTIONS = subdir-objects

DEPDIR = .deps-util
SUBDIRS = . grub-core po docs

include $(top_srcdir)/conf/Makefile.common
include $(top_srcdir)/conf/Makefile.extra-dist

AM_CFLAGS = $(HOST_CFLAGS)
AM_LDFLAGS = $(HOST_LDFLAGS)
AM_CPPFLAGS = $(HOST_CPPFLAGS) $(CPPFLAGS_DEFAULT)
AM_CCASFLAGS = $(HOST_CCASFLAGS) $(CCASFLAGS_DEFAULT)

CFLAGS_PROGRAM += $(CFLAGS_GNULIB)
LDFLAGS_PROGRAM += $(LDFLAGS_GNULIB)
CPPFLAGS_PROGRAM += $(CPPFLAGS_GNULIB)
CCASFLAGS_PROGRAM += $(CCASFLAGS_GNULIB)

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
