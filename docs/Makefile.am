info_TEXINFOS = grub.texi multiboot.texi
man_MANS = grub.8
HELP2MAN = help2man
noinst_SCRIPTS = $(HELP2MAN)

EXTRA_DIST = menu.lst $(man_MANS) $(noinst_SCRIPTS)

# FIXME: Use this rule until Debian has the new Texinfo.
grub.info: grub.texi
	@cd $(srcdir) && rm -f $@ $@-[0-9] $@-[0-9][0-9]
	cd $(srcdir) \
	  && sed 's/@command/@file/g; s/ifnottex/ifinfo/' grub.texi | $(MAKEINFO)

if MAINTAINER_MODE
$(srcdir)/grub.8: ../grub/grub $(srcdir)/$(HELP2MAN)
	$(PERL) $(srcdir)/$(HELP2MAN) --name="the grub shell" \
		--section=8 --output=$@ $<
endif
