info_TEXINFOS = grub.texi multiboot.texi
EXAMPLES = boot.S kernel.c multiboot.h
multiboot_TEXINFOS = boot.S.texi kernel.c.texi multiboot.h.texi
man_MANS = grub.8
HELP2MAN = help2man
SRC2TEXI = src2texi
noinst_SCRIPTS = $(HELP2MAN) $(SRC2TEXI)

EXTRA_DIST = menu.lst $(man_MANS) $(noinst_SCRIPTS) \
	$(EXAMPLES) $(multiboot_TEXINFOS)

%.c.texi: %.c $(srcdir)/$(SRC2TEXI)
	/bin/sh $(srcdir)/$(SRC2TEXI) $(srcdir) $< $@

%.h.texi: %.h $(srcdir)/$(SRC2TEXI)
	/bin/sh $(srcdir)/$(SRC2TEXI) $(srcdir) $< $@

%.S.texi: %.S $(srcdir)/$(SRC2TEXI)
	/bin/sh $(srcdir)/$(SRC2TEXI) $(srcdir) $< $@

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
