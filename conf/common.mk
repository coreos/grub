# -*- makefile -*-

# For the parser.
grub_script.tab.c grub_script.tab.h: normal/parser.y
	$(YACC) -d -p grub_script_yy -b grub_script $(srcdir)/normal/parser.y

# For grub-emu.
grub_modules_init.lst: geninit.sh
	(cd $(srcdir); grep -r --include="*.c" GRUB_MOD_INIT *) > $@

grub_modules_init.h: $(filter-out grub_emu_init.c,$(grub_emu_SOURCES)) geninitheader.sh grub_modules_init.lst
	sh $(srcdir)/geninitheader.sh > $@

grub_emu_init.c: $(filter-out grub_emu_init.c,$(grub_emu_SOURCES)) geninit.sh grub_modules_init.lst grub_modules_init.h
	sh $(srcdir)/geninit.sh $(filter %.c,$^) > $@



# Filing systems.
pkgdata_MODULES += fshelp.mod fat.mod ufs.mod ext2.mod		\
	minix.mod hfs.mod jfs.mod iso9660.mod xfs.mod affs.mod	\
	sfs.mod

# For fshelp.mod.
fshelp_mod_SOURCES = fs/fshelp.c
CLEANFILES += fshelp.mod mod-fshelp.o mod-fshelp.c pre-fshelp.o fshelp_mod-fs_fshelp.o def-fshelp.lst und-fshelp.lst
MOSTLYCLEANFILES += fshelp_mod-fs_fshelp.d
DEFSYMFILES += def-fshelp.lst
UNDSYMFILES += und-fshelp.lst

fshelp.mod: pre-fshelp.o mod-fshelp.o
	-rm -f $@
	$(LD) $(fshelp_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-fshelp.o: fshelp_mod-fs_fshelp.o
	-rm -f $@
	$(LD) $(fshelp_mod_LDFLAGS) -r -d -o $@ $^

mod-fshelp.o: mod-fshelp.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(fshelp_mod_CFLAGS) -c -o $@ $<

mod-fshelp.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'fshelp' $< > $@ || (rm -f $@; exit 1)

def-fshelp.lst: pre-fshelp.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 fshelp/' > $@

und-fshelp.lst: pre-fshelp.o
	echo 'fshelp' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

fshelp_mod-fs_fshelp.o: fs/fshelp.c
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(fshelp_mod_CFLAGS) -c -o $@ $<

fshelp_mod-fs_fshelp.d: fs/fshelp.c
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(fshelp_mod_CFLAGS) -M $< 	  | sed 's,fshelp\.o[ :]*,fshelp_mod-fs_fshelp.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include fshelp_mod-fs_fshelp.d

CLEANFILES += cmd-fshelp_mod-fs_fshelp.lst fs-fshelp_mod-fs_fshelp.lst
COMMANDFILES += cmd-fshelp_mod-fs_fshelp.lst
FSFILES += fs-fshelp_mod-fs_fshelp.lst

cmd-fshelp_mod-fs_fshelp.lst: fs/fshelp.c gencmdlist.sh
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(fshelp_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh fshelp > $@ || (rm -f $@; exit 1)

fs-fshelp_mod-fs_fshelp.lst: fs/fshelp.c genfslist.sh
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(fshelp_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh fshelp > $@ || (rm -f $@; exit 1)


fshelp_mod_CFLAGS = $(COMMON_CFLAGS)
fshelp_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For fat.mod.
fat_mod_SOURCES = fs/fat.c
CLEANFILES += fat.mod mod-fat.o mod-fat.c pre-fat.o fat_mod-fs_fat.o def-fat.lst und-fat.lst
MOSTLYCLEANFILES += fat_mod-fs_fat.d
DEFSYMFILES += def-fat.lst
UNDSYMFILES += und-fat.lst

fat.mod: pre-fat.o mod-fat.o
	-rm -f $@
	$(LD) $(fat_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-fat.o: fat_mod-fs_fat.o
	-rm -f $@
	$(LD) $(fat_mod_LDFLAGS) -r -d -o $@ $^

mod-fat.o: mod-fat.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(fat_mod_CFLAGS) -c -o $@ $<

mod-fat.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'fat' $< > $@ || (rm -f $@; exit 1)

def-fat.lst: pre-fat.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 fat/' > $@

und-fat.lst: pre-fat.o
	echo 'fat' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

fat_mod-fs_fat.o: fs/fat.c
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(fat_mod_CFLAGS) -c -o $@ $<

fat_mod-fs_fat.d: fs/fat.c
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(fat_mod_CFLAGS) -M $< 	  | sed 's,fat\.o[ :]*,fat_mod-fs_fat.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include fat_mod-fs_fat.d

CLEANFILES += cmd-fat_mod-fs_fat.lst fs-fat_mod-fs_fat.lst
COMMANDFILES += cmd-fat_mod-fs_fat.lst
FSFILES += fs-fat_mod-fs_fat.lst

cmd-fat_mod-fs_fat.lst: fs/fat.c gencmdlist.sh
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(fat_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh fat > $@ || (rm -f $@; exit 1)

fs-fat_mod-fs_fat.lst: fs/fat.c genfslist.sh
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(fat_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh fat > $@ || (rm -f $@; exit 1)


fat_mod_CFLAGS = $(COMMON_CFLAGS)
fat_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For ufs.mod.
ufs_mod_SOURCES = fs/ufs.c
CLEANFILES += ufs.mod mod-ufs.o mod-ufs.c pre-ufs.o ufs_mod-fs_ufs.o def-ufs.lst und-ufs.lst
MOSTLYCLEANFILES += ufs_mod-fs_ufs.d
DEFSYMFILES += def-ufs.lst
UNDSYMFILES += und-ufs.lst

ufs.mod: pre-ufs.o mod-ufs.o
	-rm -f $@
	$(LD) $(ufs_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-ufs.o: ufs_mod-fs_ufs.o
	-rm -f $@
	$(LD) $(ufs_mod_LDFLAGS) -r -d -o $@ $^

mod-ufs.o: mod-ufs.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(ufs_mod_CFLAGS) -c -o $@ $<

mod-ufs.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'ufs' $< > $@ || (rm -f $@; exit 1)

def-ufs.lst: pre-ufs.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 ufs/' > $@

und-ufs.lst: pre-ufs.o
	echo 'ufs' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

ufs_mod-fs_ufs.o: fs/ufs.c
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(ufs_mod_CFLAGS) -c -o $@ $<

ufs_mod-fs_ufs.d: fs/ufs.c
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(ufs_mod_CFLAGS) -M $< 	  | sed 's,ufs\.o[ :]*,ufs_mod-fs_ufs.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include ufs_mod-fs_ufs.d

CLEANFILES += cmd-ufs_mod-fs_ufs.lst fs-ufs_mod-fs_ufs.lst
COMMANDFILES += cmd-ufs_mod-fs_ufs.lst
FSFILES += fs-ufs_mod-fs_ufs.lst

cmd-ufs_mod-fs_ufs.lst: fs/ufs.c gencmdlist.sh
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(ufs_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh ufs > $@ || (rm -f $@; exit 1)

fs-ufs_mod-fs_ufs.lst: fs/ufs.c genfslist.sh
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(ufs_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh ufs > $@ || (rm -f $@; exit 1)


ufs_mod_CFLAGS = $(COMMON_CFLAGS)
ufs_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For ext2.mod.
ext2_mod_SOURCES = fs/ext2.c
CLEANFILES += ext2.mod mod-ext2.o mod-ext2.c pre-ext2.o ext2_mod-fs_ext2.o def-ext2.lst und-ext2.lst
MOSTLYCLEANFILES += ext2_mod-fs_ext2.d
DEFSYMFILES += def-ext2.lst
UNDSYMFILES += und-ext2.lst

ext2.mod: pre-ext2.o mod-ext2.o
	-rm -f $@
	$(LD) $(ext2_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-ext2.o: ext2_mod-fs_ext2.o
	-rm -f $@
	$(LD) $(ext2_mod_LDFLAGS) -r -d -o $@ $^

mod-ext2.o: mod-ext2.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(ext2_mod_CFLAGS) -c -o $@ $<

mod-ext2.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'ext2' $< > $@ || (rm -f $@; exit 1)

def-ext2.lst: pre-ext2.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 ext2/' > $@

und-ext2.lst: pre-ext2.o
	echo 'ext2' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

ext2_mod-fs_ext2.o: fs/ext2.c
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(ext2_mod_CFLAGS) -c -o $@ $<

ext2_mod-fs_ext2.d: fs/ext2.c
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(ext2_mod_CFLAGS) -M $< 	  | sed 's,ext2\.o[ :]*,ext2_mod-fs_ext2.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include ext2_mod-fs_ext2.d

CLEANFILES += cmd-ext2_mod-fs_ext2.lst fs-ext2_mod-fs_ext2.lst
COMMANDFILES += cmd-ext2_mod-fs_ext2.lst
FSFILES += fs-ext2_mod-fs_ext2.lst

cmd-ext2_mod-fs_ext2.lst: fs/ext2.c gencmdlist.sh
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(ext2_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh ext2 > $@ || (rm -f $@; exit 1)

fs-ext2_mod-fs_ext2.lst: fs/ext2.c genfslist.sh
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(ext2_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh ext2 > $@ || (rm -f $@; exit 1)


ext2_mod_CFLAGS = $(COMMON_CFLAGS)
ext2_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For minix.mod.
minix_mod_SOURCES = fs/minix.c
CLEANFILES += minix.mod mod-minix.o mod-minix.c pre-minix.o minix_mod-fs_minix.o def-minix.lst und-minix.lst
MOSTLYCLEANFILES += minix_mod-fs_minix.d
DEFSYMFILES += def-minix.lst
UNDSYMFILES += und-minix.lst

minix.mod: pre-minix.o mod-minix.o
	-rm -f $@
	$(LD) $(minix_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-minix.o: minix_mod-fs_minix.o
	-rm -f $@
	$(LD) $(minix_mod_LDFLAGS) -r -d -o $@ $^

mod-minix.o: mod-minix.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(minix_mod_CFLAGS) -c -o $@ $<

mod-minix.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'minix' $< > $@ || (rm -f $@; exit 1)

def-minix.lst: pre-minix.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 minix/' > $@

und-minix.lst: pre-minix.o
	echo 'minix' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

minix_mod-fs_minix.o: fs/minix.c
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(minix_mod_CFLAGS) -c -o $@ $<

minix_mod-fs_minix.d: fs/minix.c
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(minix_mod_CFLAGS) -M $< 	  | sed 's,minix\.o[ :]*,minix_mod-fs_minix.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include minix_mod-fs_minix.d

CLEANFILES += cmd-minix_mod-fs_minix.lst fs-minix_mod-fs_minix.lst
COMMANDFILES += cmd-minix_mod-fs_minix.lst
FSFILES += fs-minix_mod-fs_minix.lst

cmd-minix_mod-fs_minix.lst: fs/minix.c gencmdlist.sh
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(minix_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh minix > $@ || (rm -f $@; exit 1)

fs-minix_mod-fs_minix.lst: fs/minix.c genfslist.sh
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(minix_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh minix > $@ || (rm -f $@; exit 1)


minix_mod_CFLAGS = $(COMMON_CFLAGS)
minix_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For hfs.mod.
hfs_mod_SOURCES = fs/hfs.c
CLEANFILES += hfs.mod mod-hfs.o mod-hfs.c pre-hfs.o hfs_mod-fs_hfs.o def-hfs.lst und-hfs.lst
MOSTLYCLEANFILES += hfs_mod-fs_hfs.d
DEFSYMFILES += def-hfs.lst
UNDSYMFILES += und-hfs.lst

hfs.mod: pre-hfs.o mod-hfs.o
	-rm -f $@
	$(LD) $(hfs_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-hfs.o: hfs_mod-fs_hfs.o
	-rm -f $@
	$(LD) $(hfs_mod_LDFLAGS) -r -d -o $@ $^

mod-hfs.o: mod-hfs.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(hfs_mod_CFLAGS) -c -o $@ $<

mod-hfs.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'hfs' $< > $@ || (rm -f $@; exit 1)

def-hfs.lst: pre-hfs.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 hfs/' > $@

und-hfs.lst: pre-hfs.o
	echo 'hfs' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

hfs_mod-fs_hfs.o: fs/hfs.c
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(hfs_mod_CFLAGS) -c -o $@ $<

hfs_mod-fs_hfs.d: fs/hfs.c
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(hfs_mod_CFLAGS) -M $< 	  | sed 's,hfs\.o[ :]*,hfs_mod-fs_hfs.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include hfs_mod-fs_hfs.d

CLEANFILES += cmd-hfs_mod-fs_hfs.lst fs-hfs_mod-fs_hfs.lst
COMMANDFILES += cmd-hfs_mod-fs_hfs.lst
FSFILES += fs-hfs_mod-fs_hfs.lst

cmd-hfs_mod-fs_hfs.lst: fs/hfs.c gencmdlist.sh
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(hfs_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh hfs > $@ || (rm -f $@; exit 1)

fs-hfs_mod-fs_hfs.lst: fs/hfs.c genfslist.sh
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(hfs_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh hfs > $@ || (rm -f $@; exit 1)


hfs_mod_CFLAGS = $(COMMON_CFLAGS)
hfs_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For jfs.mod.
jfs_mod_SOURCES = fs/jfs.c
CLEANFILES += jfs.mod mod-jfs.o mod-jfs.c pre-jfs.o jfs_mod-fs_jfs.o def-jfs.lst und-jfs.lst
MOSTLYCLEANFILES += jfs_mod-fs_jfs.d
DEFSYMFILES += def-jfs.lst
UNDSYMFILES += und-jfs.lst

jfs.mod: pre-jfs.o mod-jfs.o
	-rm -f $@
	$(LD) $(jfs_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-jfs.o: jfs_mod-fs_jfs.o
	-rm -f $@
	$(LD) $(jfs_mod_LDFLAGS) -r -d -o $@ $^

mod-jfs.o: mod-jfs.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(jfs_mod_CFLAGS) -c -o $@ $<

mod-jfs.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'jfs' $< > $@ || (rm -f $@; exit 1)

def-jfs.lst: pre-jfs.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 jfs/' > $@

und-jfs.lst: pre-jfs.o
	echo 'jfs' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

jfs_mod-fs_jfs.o: fs/jfs.c
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(jfs_mod_CFLAGS) -c -o $@ $<

jfs_mod-fs_jfs.d: fs/jfs.c
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(jfs_mod_CFLAGS) -M $< 	  | sed 's,jfs\.o[ :]*,jfs_mod-fs_jfs.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include jfs_mod-fs_jfs.d

CLEANFILES += cmd-jfs_mod-fs_jfs.lst fs-jfs_mod-fs_jfs.lst
COMMANDFILES += cmd-jfs_mod-fs_jfs.lst
FSFILES += fs-jfs_mod-fs_jfs.lst

cmd-jfs_mod-fs_jfs.lst: fs/jfs.c gencmdlist.sh
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(jfs_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh jfs > $@ || (rm -f $@; exit 1)

fs-jfs_mod-fs_jfs.lst: fs/jfs.c genfslist.sh
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(jfs_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh jfs > $@ || (rm -f $@; exit 1)


jfs_mod_CFLAGS = $(COMMON_CFLAGS)
jfs_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For iso9660.mod.
iso9660_mod_SOURCES = fs/iso9660.c
CLEANFILES += iso9660.mod mod-iso9660.o mod-iso9660.c pre-iso9660.o iso9660_mod-fs_iso9660.o def-iso9660.lst und-iso9660.lst
MOSTLYCLEANFILES += iso9660_mod-fs_iso9660.d
DEFSYMFILES += def-iso9660.lst
UNDSYMFILES += und-iso9660.lst

iso9660.mod: pre-iso9660.o mod-iso9660.o
	-rm -f $@
	$(LD) $(iso9660_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-iso9660.o: iso9660_mod-fs_iso9660.o
	-rm -f $@
	$(LD) $(iso9660_mod_LDFLAGS) -r -d -o $@ $^

mod-iso9660.o: mod-iso9660.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(iso9660_mod_CFLAGS) -c -o $@ $<

mod-iso9660.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'iso9660' $< > $@ || (rm -f $@; exit 1)

def-iso9660.lst: pre-iso9660.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 iso9660/' > $@

und-iso9660.lst: pre-iso9660.o
	echo 'iso9660' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

iso9660_mod-fs_iso9660.o: fs/iso9660.c
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(iso9660_mod_CFLAGS) -c -o $@ $<

iso9660_mod-fs_iso9660.d: fs/iso9660.c
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(iso9660_mod_CFLAGS) -M $< 	  | sed 's,iso9660\.o[ :]*,iso9660_mod-fs_iso9660.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include iso9660_mod-fs_iso9660.d

CLEANFILES += cmd-iso9660_mod-fs_iso9660.lst fs-iso9660_mod-fs_iso9660.lst
COMMANDFILES += cmd-iso9660_mod-fs_iso9660.lst
FSFILES += fs-iso9660_mod-fs_iso9660.lst

cmd-iso9660_mod-fs_iso9660.lst: fs/iso9660.c gencmdlist.sh
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(iso9660_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh iso9660 > $@ || (rm -f $@; exit 1)

fs-iso9660_mod-fs_iso9660.lst: fs/iso9660.c genfslist.sh
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(iso9660_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh iso9660 > $@ || (rm -f $@; exit 1)


iso9660_mod_CFLAGS = $(COMMON_CFLAGS)
iso9660_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For xfs.mod.
xfs_mod_SOURCES = fs/xfs.c
CLEANFILES += xfs.mod mod-xfs.o mod-xfs.c pre-xfs.o xfs_mod-fs_xfs.o def-xfs.lst und-xfs.lst
MOSTLYCLEANFILES += xfs_mod-fs_xfs.d
DEFSYMFILES += def-xfs.lst
UNDSYMFILES += und-xfs.lst

xfs.mod: pre-xfs.o mod-xfs.o
	-rm -f $@
	$(LD) $(xfs_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-xfs.o: xfs_mod-fs_xfs.o
	-rm -f $@
	$(LD) $(xfs_mod_LDFLAGS) -r -d -o $@ $^

mod-xfs.o: mod-xfs.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(xfs_mod_CFLAGS) -c -o $@ $<

mod-xfs.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'xfs' $< > $@ || (rm -f $@; exit 1)

def-xfs.lst: pre-xfs.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 xfs/' > $@

und-xfs.lst: pre-xfs.o
	echo 'xfs' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

xfs_mod-fs_xfs.o: fs/xfs.c
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(xfs_mod_CFLAGS) -c -o $@ $<

xfs_mod-fs_xfs.d: fs/xfs.c
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(xfs_mod_CFLAGS) -M $< 	  | sed 's,xfs\.o[ :]*,xfs_mod-fs_xfs.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include xfs_mod-fs_xfs.d

CLEANFILES += cmd-xfs_mod-fs_xfs.lst fs-xfs_mod-fs_xfs.lst
COMMANDFILES += cmd-xfs_mod-fs_xfs.lst
FSFILES += fs-xfs_mod-fs_xfs.lst

cmd-xfs_mod-fs_xfs.lst: fs/xfs.c gencmdlist.sh
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(xfs_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh xfs > $@ || (rm -f $@; exit 1)

fs-xfs_mod-fs_xfs.lst: fs/xfs.c genfslist.sh
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(xfs_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh xfs > $@ || (rm -f $@; exit 1)


xfs_mod_CFLAGS = $(COMMON_CFLAGS)
xfs_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For affs.mod.
affs_mod_SOURCES = fs/affs.c
CLEANFILES += affs.mod mod-affs.o mod-affs.c pre-affs.o affs_mod-fs_affs.o def-affs.lst und-affs.lst
MOSTLYCLEANFILES += affs_mod-fs_affs.d
DEFSYMFILES += def-affs.lst
UNDSYMFILES += und-affs.lst

affs.mod: pre-affs.o mod-affs.o
	-rm -f $@
	$(LD) $(affs_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-affs.o: affs_mod-fs_affs.o
	-rm -f $@
	$(LD) $(affs_mod_LDFLAGS) -r -d -o $@ $^

mod-affs.o: mod-affs.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(affs_mod_CFLAGS) -c -o $@ $<

mod-affs.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'affs' $< > $@ || (rm -f $@; exit 1)

def-affs.lst: pre-affs.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 affs/' > $@

und-affs.lst: pre-affs.o
	echo 'affs' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

affs_mod-fs_affs.o: fs/affs.c
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(affs_mod_CFLAGS) -c -o $@ $<

affs_mod-fs_affs.d: fs/affs.c
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(affs_mod_CFLAGS) -M $< 	  | sed 's,affs\.o[ :]*,affs_mod-fs_affs.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include affs_mod-fs_affs.d

CLEANFILES += cmd-affs_mod-fs_affs.lst fs-affs_mod-fs_affs.lst
COMMANDFILES += cmd-affs_mod-fs_affs.lst
FSFILES += fs-affs_mod-fs_affs.lst

cmd-affs_mod-fs_affs.lst: fs/affs.c gencmdlist.sh
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(affs_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh affs > $@ || (rm -f $@; exit 1)

fs-affs_mod-fs_affs.lst: fs/affs.c genfslist.sh
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(affs_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh affs > $@ || (rm -f $@; exit 1)


affs_mod_CFLAGS = $(COMMON_CFLAGS)
affs_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For sfs.mod.
sfs_mod_SOURCES = fs/sfs.c
CLEANFILES += sfs.mod mod-sfs.o mod-sfs.c pre-sfs.o sfs_mod-fs_sfs.o def-sfs.lst und-sfs.lst
MOSTLYCLEANFILES += sfs_mod-fs_sfs.d
DEFSYMFILES += def-sfs.lst
UNDSYMFILES += und-sfs.lst

sfs.mod: pre-sfs.o mod-sfs.o
	-rm -f $@
	$(LD) $(sfs_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-sfs.o: sfs_mod-fs_sfs.o
	-rm -f $@
	$(LD) $(sfs_mod_LDFLAGS) -r -d -o $@ $^

mod-sfs.o: mod-sfs.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(sfs_mod_CFLAGS) -c -o $@ $<

mod-sfs.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'sfs' $< > $@ || (rm -f $@; exit 1)

def-sfs.lst: pre-sfs.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 sfs/' > $@

und-sfs.lst: pre-sfs.o
	echo 'sfs' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

sfs_mod-fs_sfs.o: fs/sfs.c
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(sfs_mod_CFLAGS) -c -o $@ $<

sfs_mod-fs_sfs.d: fs/sfs.c
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(sfs_mod_CFLAGS) -M $< 	  | sed 's,sfs\.o[ :]*,sfs_mod-fs_sfs.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include sfs_mod-fs_sfs.d

CLEANFILES += cmd-sfs_mod-fs_sfs.lst fs-sfs_mod-fs_sfs.lst
COMMANDFILES += cmd-sfs_mod-fs_sfs.lst
FSFILES += fs-sfs_mod-fs_sfs.lst

cmd-sfs_mod-fs_sfs.lst: fs/sfs.c gencmdlist.sh
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(sfs_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh sfs > $@ || (rm -f $@; exit 1)

fs-sfs_mod-fs_sfs.lst: fs/sfs.c genfslist.sh
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(sfs_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh sfs > $@ || (rm -f $@; exit 1)


sfs_mod_CFLAGS = $(COMMON_CFLAGS)
sfs_mod_LDFLAGS = $(COMMON_LDFLAGS)
 

# Partiton maps.
pkgdata_MODULES += amiga.mod apple.mod pc.mod sun.mod acorn.mod

# For amiga.mod
amiga_mod_SOURCES = partmap/amiga.c
CLEANFILES += amiga.mod mod-amiga.o mod-amiga.c pre-amiga.o amiga_mod-partmap_amiga.o def-amiga.lst und-amiga.lst
MOSTLYCLEANFILES += amiga_mod-partmap_amiga.d
DEFSYMFILES += def-amiga.lst
UNDSYMFILES += und-amiga.lst

amiga.mod: pre-amiga.o mod-amiga.o
	-rm -f $@
	$(LD) $(amiga_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-amiga.o: amiga_mod-partmap_amiga.o
	-rm -f $@
	$(LD) $(amiga_mod_LDFLAGS) -r -d -o $@ $^

mod-amiga.o: mod-amiga.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(amiga_mod_CFLAGS) -c -o $@ $<

mod-amiga.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'amiga' $< > $@ || (rm -f $@; exit 1)

def-amiga.lst: pre-amiga.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 amiga/' > $@

und-amiga.lst: pre-amiga.o
	echo 'amiga' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

amiga_mod-partmap_amiga.o: partmap/amiga.c
	$(CC) -Ipartmap -I$(srcdir)/partmap $(CPPFLAGS) $(CFLAGS) $(amiga_mod_CFLAGS) -c -o $@ $<

amiga_mod-partmap_amiga.d: partmap/amiga.c
	set -e; 	  $(CC) -Ipartmap -I$(srcdir)/partmap $(CPPFLAGS) $(CFLAGS) $(amiga_mod_CFLAGS) -M $< 	  | sed 's,amiga\.o[ :]*,amiga_mod-partmap_amiga.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include amiga_mod-partmap_amiga.d

CLEANFILES += cmd-amiga_mod-partmap_amiga.lst fs-amiga_mod-partmap_amiga.lst
COMMANDFILES += cmd-amiga_mod-partmap_amiga.lst
FSFILES += fs-amiga_mod-partmap_amiga.lst

cmd-amiga_mod-partmap_amiga.lst: partmap/amiga.c gencmdlist.sh
	set -e; 	  $(CC) -Ipartmap -I$(srcdir)/partmap $(CPPFLAGS) $(CFLAGS) $(amiga_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh amiga > $@ || (rm -f $@; exit 1)

fs-amiga_mod-partmap_amiga.lst: partmap/amiga.c genfslist.sh
	set -e; 	  $(CC) -Ipartmap -I$(srcdir)/partmap $(CPPFLAGS) $(CFLAGS) $(amiga_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh amiga > $@ || (rm -f $@; exit 1)


amiga_mod_CFLAGS = $(COMMON_CFLAGS)
amiga_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For apple.mod
apple_mod_SOURCES = partmap/apple.c
CLEANFILES += apple.mod mod-apple.o mod-apple.c pre-apple.o apple_mod-partmap_apple.o def-apple.lst und-apple.lst
MOSTLYCLEANFILES += apple_mod-partmap_apple.d
DEFSYMFILES += def-apple.lst
UNDSYMFILES += und-apple.lst

apple.mod: pre-apple.o mod-apple.o
	-rm -f $@
	$(LD) $(apple_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-apple.o: apple_mod-partmap_apple.o
	-rm -f $@
	$(LD) $(apple_mod_LDFLAGS) -r -d -o $@ $^

mod-apple.o: mod-apple.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(apple_mod_CFLAGS) -c -o $@ $<

mod-apple.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'apple' $< > $@ || (rm -f $@; exit 1)

def-apple.lst: pre-apple.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 apple/' > $@

und-apple.lst: pre-apple.o
	echo 'apple' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

apple_mod-partmap_apple.o: partmap/apple.c
	$(CC) -Ipartmap -I$(srcdir)/partmap $(CPPFLAGS) $(CFLAGS) $(apple_mod_CFLAGS) -c -o $@ $<

apple_mod-partmap_apple.d: partmap/apple.c
	set -e; 	  $(CC) -Ipartmap -I$(srcdir)/partmap $(CPPFLAGS) $(CFLAGS) $(apple_mod_CFLAGS) -M $< 	  | sed 's,apple\.o[ :]*,apple_mod-partmap_apple.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include apple_mod-partmap_apple.d

CLEANFILES += cmd-apple_mod-partmap_apple.lst fs-apple_mod-partmap_apple.lst
COMMANDFILES += cmd-apple_mod-partmap_apple.lst
FSFILES += fs-apple_mod-partmap_apple.lst

cmd-apple_mod-partmap_apple.lst: partmap/apple.c gencmdlist.sh
	set -e; 	  $(CC) -Ipartmap -I$(srcdir)/partmap $(CPPFLAGS) $(CFLAGS) $(apple_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh apple > $@ || (rm -f $@; exit 1)

fs-apple_mod-partmap_apple.lst: partmap/apple.c genfslist.sh
	set -e; 	  $(CC) -Ipartmap -I$(srcdir)/partmap $(CPPFLAGS) $(CFLAGS) $(apple_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh apple > $@ || (rm -f $@; exit 1)


apple_mod_CFLAGS = $(COMMON_CFLAGS)
apple_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For pc.mod
pc_mod_SOURCES = partmap/pc.c
CLEANFILES += pc.mod mod-pc.o mod-pc.c pre-pc.o pc_mod-partmap_pc.o def-pc.lst und-pc.lst
MOSTLYCLEANFILES += pc_mod-partmap_pc.d
DEFSYMFILES += def-pc.lst
UNDSYMFILES += und-pc.lst

pc.mod: pre-pc.o mod-pc.o
	-rm -f $@
	$(LD) $(pc_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-pc.o: pc_mod-partmap_pc.o
	-rm -f $@
	$(LD) $(pc_mod_LDFLAGS) -r -d -o $@ $^

mod-pc.o: mod-pc.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(pc_mod_CFLAGS) -c -o $@ $<

mod-pc.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'pc' $< > $@ || (rm -f $@; exit 1)

def-pc.lst: pre-pc.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 pc/' > $@

und-pc.lst: pre-pc.o
	echo 'pc' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

pc_mod-partmap_pc.o: partmap/pc.c
	$(CC) -Ipartmap -I$(srcdir)/partmap $(CPPFLAGS) $(CFLAGS) $(pc_mod_CFLAGS) -c -o $@ $<

pc_mod-partmap_pc.d: partmap/pc.c
	set -e; 	  $(CC) -Ipartmap -I$(srcdir)/partmap $(CPPFLAGS) $(CFLAGS) $(pc_mod_CFLAGS) -M $< 	  | sed 's,pc\.o[ :]*,pc_mod-partmap_pc.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pc_mod-partmap_pc.d

CLEANFILES += cmd-pc_mod-partmap_pc.lst fs-pc_mod-partmap_pc.lst
COMMANDFILES += cmd-pc_mod-partmap_pc.lst
FSFILES += fs-pc_mod-partmap_pc.lst

cmd-pc_mod-partmap_pc.lst: partmap/pc.c gencmdlist.sh
	set -e; 	  $(CC) -Ipartmap -I$(srcdir)/partmap $(CPPFLAGS) $(CFLAGS) $(pc_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh pc > $@ || (rm -f $@; exit 1)

fs-pc_mod-partmap_pc.lst: partmap/pc.c genfslist.sh
	set -e; 	  $(CC) -Ipartmap -I$(srcdir)/partmap $(CPPFLAGS) $(CFLAGS) $(pc_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh pc > $@ || (rm -f $@; exit 1)


pc_mod_CFLAGS = $(COMMON_CFLAGS)
pc_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For sun.mod
sun_mod_SOURCES = partmap/sun.c
CLEANFILES += sun.mod mod-sun.o mod-sun.c pre-sun.o sun_mod-partmap_sun.o def-sun.lst und-sun.lst
MOSTLYCLEANFILES += sun_mod-partmap_sun.d
DEFSYMFILES += def-sun.lst
UNDSYMFILES += und-sun.lst

sun.mod: pre-sun.o mod-sun.o
	-rm -f $@
	$(LD) $(sun_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-sun.o: sun_mod-partmap_sun.o
	-rm -f $@
	$(LD) $(sun_mod_LDFLAGS) -r -d -o $@ $^

mod-sun.o: mod-sun.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(sun_mod_CFLAGS) -c -o $@ $<

mod-sun.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'sun' $< > $@ || (rm -f $@; exit 1)

def-sun.lst: pre-sun.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 sun/' > $@

und-sun.lst: pre-sun.o
	echo 'sun' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

sun_mod-partmap_sun.o: partmap/sun.c
	$(CC) -Ipartmap -I$(srcdir)/partmap $(CPPFLAGS) $(CFLAGS) $(sun_mod_CFLAGS) -c -o $@ $<

sun_mod-partmap_sun.d: partmap/sun.c
	set -e; 	  $(CC) -Ipartmap -I$(srcdir)/partmap $(CPPFLAGS) $(CFLAGS) $(sun_mod_CFLAGS) -M $< 	  | sed 's,sun\.o[ :]*,sun_mod-partmap_sun.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include sun_mod-partmap_sun.d

CLEANFILES += cmd-sun_mod-partmap_sun.lst fs-sun_mod-partmap_sun.lst
COMMANDFILES += cmd-sun_mod-partmap_sun.lst
FSFILES += fs-sun_mod-partmap_sun.lst

cmd-sun_mod-partmap_sun.lst: partmap/sun.c gencmdlist.sh
	set -e; 	  $(CC) -Ipartmap -I$(srcdir)/partmap $(CPPFLAGS) $(CFLAGS) $(sun_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh sun > $@ || (rm -f $@; exit 1)

fs-sun_mod-partmap_sun.lst: partmap/sun.c genfslist.sh
	set -e; 	  $(CC) -Ipartmap -I$(srcdir)/partmap $(CPPFLAGS) $(CFLAGS) $(sun_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh sun > $@ || (rm -f $@; exit 1)


sun_mod_CFLAGS = $(COMMON_CFLAGS)
sun_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For acorn.mod
acorn_mod_SOURCES = partmap/acorn.c
CLEANFILES += acorn.mod mod-acorn.o mod-acorn.c pre-acorn.o acorn_mod-partmap_acorn.o def-acorn.lst und-acorn.lst
MOSTLYCLEANFILES += acorn_mod-partmap_acorn.d
DEFSYMFILES += def-acorn.lst
UNDSYMFILES += und-acorn.lst

acorn.mod: pre-acorn.o mod-acorn.o
	-rm -f $@
	$(LD) $(acorn_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-acorn.o: acorn_mod-partmap_acorn.o
	-rm -f $@
	$(LD) $(acorn_mod_LDFLAGS) -r -d -o $@ $^

mod-acorn.o: mod-acorn.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(acorn_mod_CFLAGS) -c -o $@ $<

mod-acorn.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'acorn' $< > $@ || (rm -f $@; exit 1)

def-acorn.lst: pre-acorn.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 acorn/' > $@

und-acorn.lst: pre-acorn.o
	echo 'acorn' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

acorn_mod-partmap_acorn.o: partmap/acorn.c
	$(CC) -Ipartmap -I$(srcdir)/partmap $(CPPFLAGS) $(CFLAGS) $(acorn_mod_CFLAGS) -c -o $@ $<

acorn_mod-partmap_acorn.d: partmap/acorn.c
	set -e; 	  $(CC) -Ipartmap -I$(srcdir)/partmap $(CPPFLAGS) $(CFLAGS) $(acorn_mod_CFLAGS) -M $< 	  | sed 's,acorn\.o[ :]*,acorn_mod-partmap_acorn.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include acorn_mod-partmap_acorn.d

CLEANFILES += cmd-acorn_mod-partmap_acorn.lst fs-acorn_mod-partmap_acorn.lst
COMMANDFILES += cmd-acorn_mod-partmap_acorn.lst
FSFILES += fs-acorn_mod-partmap_acorn.lst

cmd-acorn_mod-partmap_acorn.lst: partmap/acorn.c gencmdlist.sh
	set -e; 	  $(CC) -Ipartmap -I$(srcdir)/partmap $(CPPFLAGS) $(CFLAGS) $(acorn_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh acorn > $@ || (rm -f $@; exit 1)

fs-acorn_mod-partmap_acorn.lst: partmap/acorn.c genfslist.sh
	set -e; 	  $(CC) -Ipartmap -I$(srcdir)/partmap $(CPPFLAGS) $(CFLAGS) $(acorn_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh acorn > $@ || (rm -f $@; exit 1)


acorn_mod_CFLAGS = $(COMMON_CFLAGS)
acorn_mod_LDFLAGS = $(COMMON_LDFLAGS)


# Commands.
pkgdata_MODULES += hello.mod boot.mod terminal.mod ls.mod	\
	cmp.mod cat.mod help.mod font.mod search.mod		\
	loopback.mod default.mod timeout.mod configfile.mod	\
	terminfo.mod

# For hello.mod.
hello_mod_SOURCES = hello/hello.c
CLEANFILES += hello.mod mod-hello.o mod-hello.c pre-hello.o hello_mod-hello_hello.o def-hello.lst und-hello.lst
MOSTLYCLEANFILES += hello_mod-hello_hello.d
DEFSYMFILES += def-hello.lst
UNDSYMFILES += und-hello.lst

hello.mod: pre-hello.o mod-hello.o
	-rm -f $@
	$(LD) $(hello_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-hello.o: hello_mod-hello_hello.o
	-rm -f $@
	$(LD) $(hello_mod_LDFLAGS) -r -d -o $@ $^

mod-hello.o: mod-hello.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(hello_mod_CFLAGS) -c -o $@ $<

mod-hello.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'hello' $< > $@ || (rm -f $@; exit 1)

def-hello.lst: pre-hello.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 hello/' > $@

und-hello.lst: pre-hello.o
	echo 'hello' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

hello_mod-hello_hello.o: hello/hello.c
	$(CC) -Ihello -I$(srcdir)/hello $(CPPFLAGS) $(CFLAGS) $(hello_mod_CFLAGS) -c -o $@ $<

hello_mod-hello_hello.d: hello/hello.c
	set -e; 	  $(CC) -Ihello -I$(srcdir)/hello $(CPPFLAGS) $(CFLAGS) $(hello_mod_CFLAGS) -M $< 	  | sed 's,hello\.o[ :]*,hello_mod-hello_hello.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include hello_mod-hello_hello.d

CLEANFILES += cmd-hello_mod-hello_hello.lst fs-hello_mod-hello_hello.lst
COMMANDFILES += cmd-hello_mod-hello_hello.lst
FSFILES += fs-hello_mod-hello_hello.lst

cmd-hello_mod-hello_hello.lst: hello/hello.c gencmdlist.sh
	set -e; 	  $(CC) -Ihello -I$(srcdir)/hello $(CPPFLAGS) $(CFLAGS) $(hello_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh hello > $@ || (rm -f $@; exit 1)

fs-hello_mod-hello_hello.lst: hello/hello.c genfslist.sh
	set -e; 	  $(CC) -Ihello -I$(srcdir)/hello $(CPPFLAGS) $(CFLAGS) $(hello_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh hello > $@ || (rm -f $@; exit 1)


hello_mod_CFLAGS = $(COMMON_CFLAGS)
hello_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For boot.mod.
boot_mod_SOURCES = commands/boot.c
CLEANFILES += boot.mod mod-boot.o mod-boot.c pre-boot.o boot_mod-commands_boot.o def-boot.lst und-boot.lst
MOSTLYCLEANFILES += boot_mod-commands_boot.d
DEFSYMFILES += def-boot.lst
UNDSYMFILES += und-boot.lst

boot.mod: pre-boot.o mod-boot.o
	-rm -f $@
	$(LD) $(boot_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-boot.o: boot_mod-commands_boot.o
	-rm -f $@
	$(LD) $(boot_mod_LDFLAGS) -r -d -o $@ $^

mod-boot.o: mod-boot.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(boot_mod_CFLAGS) -c -o $@ $<

mod-boot.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'boot' $< > $@ || (rm -f $@; exit 1)

def-boot.lst: pre-boot.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 boot/' > $@

und-boot.lst: pre-boot.o
	echo 'boot' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

boot_mod-commands_boot.o: commands/boot.c
	$(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(boot_mod_CFLAGS) -c -o $@ $<

boot_mod-commands_boot.d: commands/boot.c
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(boot_mod_CFLAGS) -M $< 	  | sed 's,boot\.o[ :]*,boot_mod-commands_boot.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include boot_mod-commands_boot.d

CLEANFILES += cmd-boot_mod-commands_boot.lst fs-boot_mod-commands_boot.lst
COMMANDFILES += cmd-boot_mod-commands_boot.lst
FSFILES += fs-boot_mod-commands_boot.lst

cmd-boot_mod-commands_boot.lst: commands/boot.c gencmdlist.sh
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(boot_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh boot > $@ || (rm -f $@; exit 1)

fs-boot_mod-commands_boot.lst: commands/boot.c genfslist.sh
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(boot_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh boot > $@ || (rm -f $@; exit 1)


boot_mod_CFLAGS = $(COMMON_CFLAGS)
boot_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For terminal.mod.
terminal_mod_SOURCES = commands/terminal.c
CLEANFILES += terminal.mod mod-terminal.o mod-terminal.c pre-terminal.o terminal_mod-commands_terminal.o def-terminal.lst und-terminal.lst
MOSTLYCLEANFILES += terminal_mod-commands_terminal.d
DEFSYMFILES += def-terminal.lst
UNDSYMFILES += und-terminal.lst

terminal.mod: pre-terminal.o mod-terminal.o
	-rm -f $@
	$(LD) $(terminal_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-terminal.o: terminal_mod-commands_terminal.o
	-rm -f $@
	$(LD) $(terminal_mod_LDFLAGS) -r -d -o $@ $^

mod-terminal.o: mod-terminal.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(terminal_mod_CFLAGS) -c -o $@ $<

mod-terminal.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'terminal' $< > $@ || (rm -f $@; exit 1)

def-terminal.lst: pre-terminal.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 terminal/' > $@

und-terminal.lst: pre-terminal.o
	echo 'terminal' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

terminal_mod-commands_terminal.o: commands/terminal.c
	$(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(terminal_mod_CFLAGS) -c -o $@ $<

terminal_mod-commands_terminal.d: commands/terminal.c
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(terminal_mod_CFLAGS) -M $< 	  | sed 's,terminal\.o[ :]*,terminal_mod-commands_terminal.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include terminal_mod-commands_terminal.d

CLEANFILES += cmd-terminal_mod-commands_terminal.lst fs-terminal_mod-commands_terminal.lst
COMMANDFILES += cmd-terminal_mod-commands_terminal.lst
FSFILES += fs-terminal_mod-commands_terminal.lst

cmd-terminal_mod-commands_terminal.lst: commands/terminal.c gencmdlist.sh
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(terminal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh terminal > $@ || (rm -f $@; exit 1)

fs-terminal_mod-commands_terminal.lst: commands/terminal.c genfslist.sh
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(terminal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh terminal > $@ || (rm -f $@; exit 1)


terminal_mod_CFLAGS = $(COMMON_CFLAGS)
terminal_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For ls.mod.
ls_mod_SOURCES = commands/ls.c
CLEANFILES += ls.mod mod-ls.o mod-ls.c pre-ls.o ls_mod-commands_ls.o def-ls.lst und-ls.lst
MOSTLYCLEANFILES += ls_mod-commands_ls.d
DEFSYMFILES += def-ls.lst
UNDSYMFILES += und-ls.lst

ls.mod: pre-ls.o mod-ls.o
	-rm -f $@
	$(LD) $(ls_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-ls.o: ls_mod-commands_ls.o
	-rm -f $@
	$(LD) $(ls_mod_LDFLAGS) -r -d -o $@ $^

mod-ls.o: mod-ls.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(ls_mod_CFLAGS) -c -o $@ $<

mod-ls.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'ls' $< > $@ || (rm -f $@; exit 1)

def-ls.lst: pre-ls.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 ls/' > $@

und-ls.lst: pre-ls.o
	echo 'ls' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

ls_mod-commands_ls.o: commands/ls.c
	$(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(ls_mod_CFLAGS) -c -o $@ $<

ls_mod-commands_ls.d: commands/ls.c
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(ls_mod_CFLAGS) -M $< 	  | sed 's,ls\.o[ :]*,ls_mod-commands_ls.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include ls_mod-commands_ls.d

CLEANFILES += cmd-ls_mod-commands_ls.lst fs-ls_mod-commands_ls.lst
COMMANDFILES += cmd-ls_mod-commands_ls.lst
FSFILES += fs-ls_mod-commands_ls.lst

cmd-ls_mod-commands_ls.lst: commands/ls.c gencmdlist.sh
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(ls_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh ls > $@ || (rm -f $@; exit 1)

fs-ls_mod-commands_ls.lst: commands/ls.c genfslist.sh
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(ls_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh ls > $@ || (rm -f $@; exit 1)


ls_mod_CFLAGS = $(COMMON_CFLAGS)
ls_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For cmp.mod.
cmp_mod_SOURCES = commands/cmp.c
CLEANFILES += cmp.mod mod-cmp.o mod-cmp.c pre-cmp.o cmp_mod-commands_cmp.o def-cmp.lst und-cmp.lst
MOSTLYCLEANFILES += cmp_mod-commands_cmp.d
DEFSYMFILES += def-cmp.lst
UNDSYMFILES += und-cmp.lst

cmp.mod: pre-cmp.o mod-cmp.o
	-rm -f $@
	$(LD) $(cmp_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-cmp.o: cmp_mod-commands_cmp.o
	-rm -f $@
	$(LD) $(cmp_mod_LDFLAGS) -r -d -o $@ $^

mod-cmp.o: mod-cmp.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(cmp_mod_CFLAGS) -c -o $@ $<

mod-cmp.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'cmp' $< > $@ || (rm -f $@; exit 1)

def-cmp.lst: pre-cmp.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 cmp/' > $@

und-cmp.lst: pre-cmp.o
	echo 'cmp' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

cmp_mod-commands_cmp.o: commands/cmp.c
	$(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(cmp_mod_CFLAGS) -c -o $@ $<

cmp_mod-commands_cmp.d: commands/cmp.c
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(cmp_mod_CFLAGS) -M $< 	  | sed 's,cmp\.o[ :]*,cmp_mod-commands_cmp.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include cmp_mod-commands_cmp.d

CLEANFILES += cmd-cmp_mod-commands_cmp.lst fs-cmp_mod-commands_cmp.lst
COMMANDFILES += cmd-cmp_mod-commands_cmp.lst
FSFILES += fs-cmp_mod-commands_cmp.lst

cmd-cmp_mod-commands_cmp.lst: commands/cmp.c gencmdlist.sh
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(cmp_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh cmp > $@ || (rm -f $@; exit 1)

fs-cmp_mod-commands_cmp.lst: commands/cmp.c genfslist.sh
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(cmp_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh cmp > $@ || (rm -f $@; exit 1)


cmp_mod_CFLAGS = $(COMMON_CFLAGS)
cmp_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For cat.mod.
cat_mod_SOURCES = commands/cat.c
CLEANFILES += cat.mod mod-cat.o mod-cat.c pre-cat.o cat_mod-commands_cat.o def-cat.lst und-cat.lst
MOSTLYCLEANFILES += cat_mod-commands_cat.d
DEFSYMFILES += def-cat.lst
UNDSYMFILES += und-cat.lst

cat.mod: pre-cat.o mod-cat.o
	-rm -f $@
	$(LD) $(cat_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-cat.o: cat_mod-commands_cat.o
	-rm -f $@
	$(LD) $(cat_mod_LDFLAGS) -r -d -o $@ $^

mod-cat.o: mod-cat.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(cat_mod_CFLAGS) -c -o $@ $<

mod-cat.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'cat' $< > $@ || (rm -f $@; exit 1)

def-cat.lst: pre-cat.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 cat/' > $@

und-cat.lst: pre-cat.o
	echo 'cat' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

cat_mod-commands_cat.o: commands/cat.c
	$(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(cat_mod_CFLAGS) -c -o $@ $<

cat_mod-commands_cat.d: commands/cat.c
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(cat_mod_CFLAGS) -M $< 	  | sed 's,cat\.o[ :]*,cat_mod-commands_cat.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include cat_mod-commands_cat.d

CLEANFILES += cmd-cat_mod-commands_cat.lst fs-cat_mod-commands_cat.lst
COMMANDFILES += cmd-cat_mod-commands_cat.lst
FSFILES += fs-cat_mod-commands_cat.lst

cmd-cat_mod-commands_cat.lst: commands/cat.c gencmdlist.sh
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(cat_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh cat > $@ || (rm -f $@; exit 1)

fs-cat_mod-commands_cat.lst: commands/cat.c genfslist.sh
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(cat_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh cat > $@ || (rm -f $@; exit 1)


cat_mod_CFLAGS = $(COMMON_CFLAGS)
cat_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For help.mod.
help_mod_SOURCES = commands/help.c
CLEANFILES += help.mod mod-help.o mod-help.c pre-help.o help_mod-commands_help.o def-help.lst und-help.lst
MOSTLYCLEANFILES += help_mod-commands_help.d
DEFSYMFILES += def-help.lst
UNDSYMFILES += und-help.lst

help.mod: pre-help.o mod-help.o
	-rm -f $@
	$(LD) $(help_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-help.o: help_mod-commands_help.o
	-rm -f $@
	$(LD) $(help_mod_LDFLAGS) -r -d -o $@ $^

mod-help.o: mod-help.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(help_mod_CFLAGS) -c -o $@ $<

mod-help.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'help' $< > $@ || (rm -f $@; exit 1)

def-help.lst: pre-help.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 help/' > $@

und-help.lst: pre-help.o
	echo 'help' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

help_mod-commands_help.o: commands/help.c
	$(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(help_mod_CFLAGS) -c -o $@ $<

help_mod-commands_help.d: commands/help.c
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(help_mod_CFLAGS) -M $< 	  | sed 's,help\.o[ :]*,help_mod-commands_help.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include help_mod-commands_help.d

CLEANFILES += cmd-help_mod-commands_help.lst fs-help_mod-commands_help.lst
COMMANDFILES += cmd-help_mod-commands_help.lst
FSFILES += fs-help_mod-commands_help.lst

cmd-help_mod-commands_help.lst: commands/help.c gencmdlist.sh
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(help_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh help > $@ || (rm -f $@; exit 1)

fs-help_mod-commands_help.lst: commands/help.c genfslist.sh
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(help_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh help > $@ || (rm -f $@; exit 1)


help_mod_CFLAGS = $(COMMON_CFLAGS)
help_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For font.mod.
font_mod_SOURCES = font/manager.c
CLEANFILES += font.mod mod-font.o mod-font.c pre-font.o font_mod-font_manager.o def-font.lst und-font.lst
MOSTLYCLEANFILES += font_mod-font_manager.d
DEFSYMFILES += def-font.lst
UNDSYMFILES += und-font.lst

font.mod: pre-font.o mod-font.o
	-rm -f $@
	$(LD) $(font_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-font.o: font_mod-font_manager.o
	-rm -f $@
	$(LD) $(font_mod_LDFLAGS) -r -d -o $@ $^

mod-font.o: mod-font.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(font_mod_CFLAGS) -c -o $@ $<

mod-font.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'font' $< > $@ || (rm -f $@; exit 1)

def-font.lst: pre-font.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 font/' > $@

und-font.lst: pre-font.o
	echo 'font' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

font_mod-font_manager.o: font/manager.c
	$(CC) -Ifont -I$(srcdir)/font $(CPPFLAGS) $(CFLAGS) $(font_mod_CFLAGS) -c -o $@ $<

font_mod-font_manager.d: font/manager.c
	set -e; 	  $(CC) -Ifont -I$(srcdir)/font $(CPPFLAGS) $(CFLAGS) $(font_mod_CFLAGS) -M $< 	  | sed 's,manager\.o[ :]*,font_mod-font_manager.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include font_mod-font_manager.d

CLEANFILES += cmd-font_mod-font_manager.lst fs-font_mod-font_manager.lst
COMMANDFILES += cmd-font_mod-font_manager.lst
FSFILES += fs-font_mod-font_manager.lst

cmd-font_mod-font_manager.lst: font/manager.c gencmdlist.sh
	set -e; 	  $(CC) -Ifont -I$(srcdir)/font $(CPPFLAGS) $(CFLAGS) $(font_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh font > $@ || (rm -f $@; exit 1)

fs-font_mod-font_manager.lst: font/manager.c genfslist.sh
	set -e; 	  $(CC) -Ifont -I$(srcdir)/font $(CPPFLAGS) $(CFLAGS) $(font_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh font > $@ || (rm -f $@; exit 1)


font_mod_CFLAGS = $(COMMON_CFLAGS)
font_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For search.mod.
search_mod_SOURCES = commands/search.c
CLEANFILES += search.mod mod-search.o mod-search.c pre-search.o search_mod-commands_search.o def-search.lst und-search.lst
MOSTLYCLEANFILES += search_mod-commands_search.d
DEFSYMFILES += def-search.lst
UNDSYMFILES += und-search.lst

search.mod: pre-search.o mod-search.o
	-rm -f $@
	$(LD) $(search_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-search.o: search_mod-commands_search.o
	-rm -f $@
	$(LD) $(search_mod_LDFLAGS) -r -d -o $@ $^

mod-search.o: mod-search.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(search_mod_CFLAGS) -c -o $@ $<

mod-search.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'search' $< > $@ || (rm -f $@; exit 1)

def-search.lst: pre-search.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 search/' > $@

und-search.lst: pre-search.o
	echo 'search' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

search_mod-commands_search.o: commands/search.c
	$(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(search_mod_CFLAGS) -c -o $@ $<

search_mod-commands_search.d: commands/search.c
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(search_mod_CFLAGS) -M $< 	  | sed 's,search\.o[ :]*,search_mod-commands_search.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include search_mod-commands_search.d

CLEANFILES += cmd-search_mod-commands_search.lst fs-search_mod-commands_search.lst
COMMANDFILES += cmd-search_mod-commands_search.lst
FSFILES += fs-search_mod-commands_search.lst

cmd-search_mod-commands_search.lst: commands/search.c gencmdlist.sh
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(search_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh search > $@ || (rm -f $@; exit 1)

fs-search_mod-commands_search.lst: commands/search.c genfslist.sh
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(search_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh search > $@ || (rm -f $@; exit 1)


search_mod_CFLAGS = $(COMMON_CFLAGS)
search_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For test.mod.
test_mod_SOURCES = commands/test.c
test_mod_CFLAGS = $(COMMON_CFLAGS)
test_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For loopback.mod
loopback_mod_SOURCES = disk/loopback.c
CLEANFILES += loopback.mod mod-loopback.o mod-loopback.c pre-loopback.o loopback_mod-disk_loopback.o def-loopback.lst und-loopback.lst
MOSTLYCLEANFILES += loopback_mod-disk_loopback.d
DEFSYMFILES += def-loopback.lst
UNDSYMFILES += und-loopback.lst

loopback.mod: pre-loopback.o mod-loopback.o
	-rm -f $@
	$(LD) $(loopback_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-loopback.o: loopback_mod-disk_loopback.o
	-rm -f $@
	$(LD) $(loopback_mod_LDFLAGS) -r -d -o $@ $^

mod-loopback.o: mod-loopback.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(loopback_mod_CFLAGS) -c -o $@ $<

mod-loopback.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'loopback' $< > $@ || (rm -f $@; exit 1)

def-loopback.lst: pre-loopback.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 loopback/' > $@

und-loopback.lst: pre-loopback.o
	echo 'loopback' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

loopback_mod-disk_loopback.o: disk/loopback.c
	$(CC) -Idisk -I$(srcdir)/disk $(CPPFLAGS) $(CFLAGS) $(loopback_mod_CFLAGS) -c -o $@ $<

loopback_mod-disk_loopback.d: disk/loopback.c
	set -e; 	  $(CC) -Idisk -I$(srcdir)/disk $(CPPFLAGS) $(CFLAGS) $(loopback_mod_CFLAGS) -M $< 	  | sed 's,loopback\.o[ :]*,loopback_mod-disk_loopback.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include loopback_mod-disk_loopback.d

CLEANFILES += cmd-loopback_mod-disk_loopback.lst fs-loopback_mod-disk_loopback.lst
COMMANDFILES += cmd-loopback_mod-disk_loopback.lst
FSFILES += fs-loopback_mod-disk_loopback.lst

cmd-loopback_mod-disk_loopback.lst: disk/loopback.c gencmdlist.sh
	set -e; 	  $(CC) -Idisk -I$(srcdir)/disk $(CPPFLAGS) $(CFLAGS) $(loopback_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh loopback > $@ || (rm -f $@; exit 1)

fs-loopback_mod-disk_loopback.lst: disk/loopback.c genfslist.sh
	set -e; 	  $(CC) -Idisk -I$(srcdir)/disk $(CPPFLAGS) $(CFLAGS) $(loopback_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh loopback > $@ || (rm -f $@; exit 1)


loopback_mod_CFLAGS = $(COMMON_CFLAGS)
loopback_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For default.mod
default_mod_SOURCES = commands/default.c
CLEANFILES += default.mod mod-default.o mod-default.c pre-default.o default_mod-commands_default.o def-default.lst und-default.lst
MOSTLYCLEANFILES += default_mod-commands_default.d
DEFSYMFILES += def-default.lst
UNDSYMFILES += und-default.lst

default.mod: pre-default.o mod-default.o
	-rm -f $@
	$(LD) $(default_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-default.o: default_mod-commands_default.o
	-rm -f $@
	$(LD) $(default_mod_LDFLAGS) -r -d -o $@ $^

mod-default.o: mod-default.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(default_mod_CFLAGS) -c -o $@ $<

mod-default.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'default' $< > $@ || (rm -f $@; exit 1)

def-default.lst: pre-default.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 default/' > $@

und-default.lst: pre-default.o
	echo 'default' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

default_mod-commands_default.o: commands/default.c
	$(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(default_mod_CFLAGS) -c -o $@ $<

default_mod-commands_default.d: commands/default.c
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(default_mod_CFLAGS) -M $< 	  | sed 's,default\.o[ :]*,default_mod-commands_default.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include default_mod-commands_default.d

CLEANFILES += cmd-default_mod-commands_default.lst fs-default_mod-commands_default.lst
COMMANDFILES += cmd-default_mod-commands_default.lst
FSFILES += fs-default_mod-commands_default.lst

cmd-default_mod-commands_default.lst: commands/default.c gencmdlist.sh
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(default_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh default > $@ || (rm -f $@; exit 1)

fs-default_mod-commands_default.lst: commands/default.c genfslist.sh
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(default_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh default > $@ || (rm -f $@; exit 1)


default_mod_CFLAGS = $(COMMON_CFLAGS)
default_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For timeout.mod
timeout_mod_SOURCES = commands/timeout.c
CLEANFILES += timeout.mod mod-timeout.o mod-timeout.c pre-timeout.o timeout_mod-commands_timeout.o def-timeout.lst und-timeout.lst
MOSTLYCLEANFILES += timeout_mod-commands_timeout.d
DEFSYMFILES += def-timeout.lst
UNDSYMFILES += und-timeout.lst

timeout.mod: pre-timeout.o mod-timeout.o
	-rm -f $@
	$(LD) $(timeout_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-timeout.o: timeout_mod-commands_timeout.o
	-rm -f $@
	$(LD) $(timeout_mod_LDFLAGS) -r -d -o $@ $^

mod-timeout.o: mod-timeout.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(timeout_mod_CFLAGS) -c -o $@ $<

mod-timeout.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'timeout' $< > $@ || (rm -f $@; exit 1)

def-timeout.lst: pre-timeout.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 timeout/' > $@

und-timeout.lst: pre-timeout.o
	echo 'timeout' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

timeout_mod-commands_timeout.o: commands/timeout.c
	$(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(timeout_mod_CFLAGS) -c -o $@ $<

timeout_mod-commands_timeout.d: commands/timeout.c
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(timeout_mod_CFLAGS) -M $< 	  | sed 's,timeout\.o[ :]*,timeout_mod-commands_timeout.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include timeout_mod-commands_timeout.d

CLEANFILES += cmd-timeout_mod-commands_timeout.lst fs-timeout_mod-commands_timeout.lst
COMMANDFILES += cmd-timeout_mod-commands_timeout.lst
FSFILES += fs-timeout_mod-commands_timeout.lst

cmd-timeout_mod-commands_timeout.lst: commands/timeout.c gencmdlist.sh
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(timeout_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh timeout > $@ || (rm -f $@; exit 1)

fs-timeout_mod-commands_timeout.lst: commands/timeout.c genfslist.sh
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(timeout_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh timeout > $@ || (rm -f $@; exit 1)


timeout_mod_CFLAGS = $(COMMON_CFLAGS)
timeout_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For configfile.mod
configfile_mod_SOURCES = commands/configfile.c
CLEANFILES += configfile.mod mod-configfile.o mod-configfile.c pre-configfile.o configfile_mod-commands_configfile.o def-configfile.lst und-configfile.lst
MOSTLYCLEANFILES += configfile_mod-commands_configfile.d
DEFSYMFILES += def-configfile.lst
UNDSYMFILES += und-configfile.lst

configfile.mod: pre-configfile.o mod-configfile.o
	-rm -f $@
	$(LD) $(configfile_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-configfile.o: configfile_mod-commands_configfile.o
	-rm -f $@
	$(LD) $(configfile_mod_LDFLAGS) -r -d -o $@ $^

mod-configfile.o: mod-configfile.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(configfile_mod_CFLAGS) -c -o $@ $<

mod-configfile.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'configfile' $< > $@ || (rm -f $@; exit 1)

def-configfile.lst: pre-configfile.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 configfile/' > $@

und-configfile.lst: pre-configfile.o
	echo 'configfile' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

configfile_mod-commands_configfile.o: commands/configfile.c
	$(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(configfile_mod_CFLAGS) -c -o $@ $<

configfile_mod-commands_configfile.d: commands/configfile.c
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(configfile_mod_CFLAGS) -M $< 	  | sed 's,configfile\.o[ :]*,configfile_mod-commands_configfile.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include configfile_mod-commands_configfile.d

CLEANFILES += cmd-configfile_mod-commands_configfile.lst fs-configfile_mod-commands_configfile.lst
COMMANDFILES += cmd-configfile_mod-commands_configfile.lst
FSFILES += fs-configfile_mod-commands_configfile.lst

cmd-configfile_mod-commands_configfile.lst: commands/configfile.c gencmdlist.sh
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(configfile_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh configfile > $@ || (rm -f $@; exit 1)

fs-configfile_mod-commands_configfile.lst: commands/configfile.c genfslist.sh
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(configfile_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh configfile > $@ || (rm -f $@; exit 1)


configfile_mod_CFLAGS = $(COMMON_CFLAGS)
configfile_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For terminfo.mod.
terminfo_mod_SOURCES = term/terminfo.c term/tparm.c
CLEANFILES += terminfo.mod mod-terminfo.o mod-terminfo.c pre-terminfo.o terminfo_mod-term_terminfo.o terminfo_mod-term_tparm.o def-terminfo.lst und-terminfo.lst
MOSTLYCLEANFILES += terminfo_mod-term_terminfo.d terminfo_mod-term_tparm.d
DEFSYMFILES += def-terminfo.lst
UNDSYMFILES += und-terminfo.lst

terminfo.mod: pre-terminfo.o mod-terminfo.o
	-rm -f $@
	$(LD) $(terminfo_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-terminfo.o: terminfo_mod-term_terminfo.o terminfo_mod-term_tparm.o
	-rm -f $@
	$(LD) $(terminfo_mod_LDFLAGS) -r -d -o $@ $^

mod-terminfo.o: mod-terminfo.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(terminfo_mod_CFLAGS) -c -o $@ $<

mod-terminfo.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'terminfo' $< > $@ || (rm -f $@; exit 1)

def-terminfo.lst: pre-terminfo.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 terminfo/' > $@

und-terminfo.lst: pre-terminfo.o
	echo 'terminfo' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

terminfo_mod-term_terminfo.o: term/terminfo.c
	$(CC) -Iterm -I$(srcdir)/term $(CPPFLAGS) $(CFLAGS) $(terminfo_mod_CFLAGS) -c -o $@ $<

terminfo_mod-term_terminfo.d: term/terminfo.c
	set -e; 	  $(CC) -Iterm -I$(srcdir)/term $(CPPFLAGS) $(CFLAGS) $(terminfo_mod_CFLAGS) -M $< 	  | sed 's,terminfo\.o[ :]*,terminfo_mod-term_terminfo.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include terminfo_mod-term_terminfo.d

CLEANFILES += cmd-terminfo_mod-term_terminfo.lst fs-terminfo_mod-term_terminfo.lst
COMMANDFILES += cmd-terminfo_mod-term_terminfo.lst
FSFILES += fs-terminfo_mod-term_terminfo.lst

cmd-terminfo_mod-term_terminfo.lst: term/terminfo.c gencmdlist.sh
	set -e; 	  $(CC) -Iterm -I$(srcdir)/term $(CPPFLAGS) $(CFLAGS) $(terminfo_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh terminfo > $@ || (rm -f $@; exit 1)

fs-terminfo_mod-term_terminfo.lst: term/terminfo.c genfslist.sh
	set -e; 	  $(CC) -Iterm -I$(srcdir)/term $(CPPFLAGS) $(CFLAGS) $(terminfo_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh terminfo > $@ || (rm -f $@; exit 1)


terminfo_mod-term_tparm.o: term/tparm.c
	$(CC) -Iterm -I$(srcdir)/term $(CPPFLAGS) $(CFLAGS) $(terminfo_mod_CFLAGS) -c -o $@ $<

terminfo_mod-term_tparm.d: term/tparm.c
	set -e; 	  $(CC) -Iterm -I$(srcdir)/term $(CPPFLAGS) $(CFLAGS) $(terminfo_mod_CFLAGS) -M $< 	  | sed 's,tparm\.o[ :]*,terminfo_mod-term_tparm.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include terminfo_mod-term_tparm.d

CLEANFILES += cmd-terminfo_mod-term_tparm.lst fs-terminfo_mod-term_tparm.lst
COMMANDFILES += cmd-terminfo_mod-term_tparm.lst
FSFILES += fs-terminfo_mod-term_tparm.lst

cmd-terminfo_mod-term_tparm.lst: term/tparm.c gencmdlist.sh
	set -e; 	  $(CC) -Iterm -I$(srcdir)/term $(CPPFLAGS) $(CFLAGS) $(terminfo_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh terminfo > $@ || (rm -f $@; exit 1)

fs-terminfo_mod-term_tparm.lst: term/tparm.c genfslist.sh
	set -e; 	  $(CC) -Iterm -I$(srcdir)/term $(CPPFLAGS) $(CFLAGS) $(terminfo_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh terminfo > $@ || (rm -f $@; exit 1)


terminfo_mod_CFLAGS = $(COMMON_CFLAGS)
terminfo_mod_LDFLAGS = $(COMMON_LDFLAGS)


# Misc.
pkgdata_MODULES += gzio.mod 

# For gzio.mod.
gzio_mod_SOURCES = io/gzio.c
CLEANFILES += gzio.mod mod-gzio.o mod-gzio.c pre-gzio.o gzio_mod-io_gzio.o def-gzio.lst und-gzio.lst
MOSTLYCLEANFILES += gzio_mod-io_gzio.d
DEFSYMFILES += def-gzio.lst
UNDSYMFILES += und-gzio.lst

gzio.mod: pre-gzio.o mod-gzio.o
	-rm -f $@
	$(LD) $(gzio_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-gzio.o: gzio_mod-io_gzio.o
	-rm -f $@
	$(LD) $(gzio_mod_LDFLAGS) -r -d -o $@ $^

mod-gzio.o: mod-gzio.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(gzio_mod_CFLAGS) -c -o $@ $<

mod-gzio.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'gzio' $< > $@ || (rm -f $@; exit 1)

def-gzio.lst: pre-gzio.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 gzio/' > $@

und-gzio.lst: pre-gzio.o
	echo 'gzio' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

gzio_mod-io_gzio.o: io/gzio.c
	$(CC) -Iio -I$(srcdir)/io $(CPPFLAGS) $(CFLAGS) $(gzio_mod_CFLAGS) -c -o $@ $<

gzio_mod-io_gzio.d: io/gzio.c
	set -e; 	  $(CC) -Iio -I$(srcdir)/io $(CPPFLAGS) $(CFLAGS) $(gzio_mod_CFLAGS) -M $< 	  | sed 's,gzio\.o[ :]*,gzio_mod-io_gzio.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include gzio_mod-io_gzio.d

CLEANFILES += cmd-gzio_mod-io_gzio.lst fs-gzio_mod-io_gzio.lst
COMMANDFILES += cmd-gzio_mod-io_gzio.lst
FSFILES += fs-gzio_mod-io_gzio.lst

cmd-gzio_mod-io_gzio.lst: io/gzio.c gencmdlist.sh
	set -e; 	  $(CC) -Iio -I$(srcdir)/io $(CPPFLAGS) $(CFLAGS) $(gzio_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh gzio > $@ || (rm -f $@; exit 1)

fs-gzio_mod-io_gzio.lst: io/gzio.c genfslist.sh
	set -e; 	  $(CC) -Iio -I$(srcdir)/io $(CPPFLAGS) $(CFLAGS) $(gzio_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh gzio > $@ || (rm -f $@; exit 1)


gzio_mod_CFLAGS = $(COMMON_CFLAGS)
gzio_mod_LDFLAGS = $(COMMON_LDFLAGS)



