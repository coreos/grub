info_TEXINFOS = grub.texi multiboot.texi
man_MANS = grub.8
HELP2MAN = $(srcdir)/help2man
noinst_SCRIPTS = $(HELP2MAN)

EXTRA_DIST = menu.lst $(man_MANS) $(noinst_SCRIPTS)

if GRUB_MAINT
$(srcdir)/grub.8: ../grub/grub $(HELP2MAN)
	$(HELP2MAN) --name="the stage2 emulator shell" --section=8 \
		--output=$@ $<
endif
