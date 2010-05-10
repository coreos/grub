AUTOMAKE_OPTIONS = subdir-objects
DEPDIR = .deps-util

EXTRA_DIST = autogen.sh gentpl.py Makefile.tpl modules.def		\
									\
	geninit.sh							\
									\
	gnulib/progname.h gnulib/fnmatch_loop.c gnulib/alloca.h		\
	gnulib/error.h gnulib/fnmatch.h gnulib/getopt.h			\
	gnulib/getopt_int.h gnulib/gettext.h gnulib/progname.h		\
									\
	util/mkisofs/mkisofs.h util/mkisofs/iso9660.h			\
	util/mkisofs/include/prototyp.h util/mkisofs/defaults.h		\
	util/mkisofs/match.h util/mkisofs/exclude.h			\
	util/mkisofs/msdos_partition.h util/mkisofs/include/fctldefs.h	\
	util/mkisofs/include/mconfig.h util/mkisofs/include/statdefs.h

SUBDIRS = . grub-core po docs

include $(top_srcdir)/grub-core/Makefile.vars

CFLAGS_PROGRAM = $(HOST_CFLAGS) $(CFLAGS_GNULIB)
LDFLAGS_PROGRAM = $(HOST_LDFLAGS) $(LDFLAGS_GNULIB) $(LIBINTL)
CPPFLAGS_PROGRAM = $(HOST_CPPFLAGS) $(CPPFLAGS_GNULIB)
CCASFLAGS_PROGRAM = $(HOST_CCASFLAGS) $(CCASFLAGS_GNULIB)

CFLAGS_LIBRARY = $(CFLAGS_PROGRAM)
CPPFLAGS_LIBRARY = $(CPPFLAGS_PROGRAM)
CCASFLAGS_LIBRARY = $(CCASFLAGS_PROGRAM)

AM_CFLAGS =
AM_LDFLAGS =
AM_CPPFLAGS = $(CPPFLAGS_GRUB) -DGRUB_FILE=\"$(subst $(top_srcdir)/,,$<)\"
AM_CCASFLAGS = -DASM_FILE=1

include $(srcdir)/modules.am

# XXX Use Automake's LEX & YACC support
grub_script.tab.c grub_script.tab.h: $(top_srcdir)/grub-core/script/parser.y
	$(YACC) -d -p grub_script_yy -b grub_script $(top_srcdir)/grub-core/script/parser.y
CLEANFILES += grub_script.tab.c grub_script.tab.h

# For the lexer.
grub_script.yy.c grub_script.yy.h: $(top_srcdir)/grub-core/script/yylex.l
	$(LEX) -o grub_script.yy.c --header-file=grub_script.yy.h $(top_srcdir)/grub-core/script/yylex.l
CLEANFILES += grub_script.yy.c grub_script.yy.h

# For libutil.a
libutil_a_init.lst: grub_script.tab.h grub_script.yy.h $(libutil_a_SOURCES)
	rm -f $@
	$(CPP) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(libutil_a_CPPFLAGS) $(CPPFLAGS) \
	  -D'GRUB_MOD_INIT(x)=@MARKER@x@' $^ \
	  | grep '@MARKER@' | sed 's/@MARKER@\(.*\)@/\1/g' | sort -u > $@ || (rm -f $@; exit 1)
CLEANFILES += libutil_a_init.lst

libutil_a_init.c: libutil_a_init.lst $(top_srcdir)/geninit.sh
	rm -f $@; sh $(top_srcdir)/geninit.sh `cat $<` > $@ || (rm -f $@; exit 1)
CLEANFILES += libutil_a_init.c

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

platform_HEADERS = config.h
