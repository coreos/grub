info_TEXINFOS = grub.texi multiboot.texi
man_MANS = grub.8
HELP2MAN = help2man
noinst_SCRIPTS = $(HELP2MAN)

EXTRA_DIST = menu.lst $(man_MANS) $(noinst_SCRIPTS)

if GRUB_MAINT
$(srcdir)/grub.8: ../grub/grub $(srcdir)/$(HELP2MAN)
	$(srcdir)/$(HELP2MAN) --name="the stage2 emulator shell" \
		--section=8 --output=$@ $<
endif
