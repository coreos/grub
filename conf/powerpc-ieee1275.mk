
# -*- makefile -*-

COMMON_ASFLAGS = -nostdinc -D__ASSEMBLY__
COMMON_CFLAGS = -ffreestanding -msoft-float
COMMON_LDFLAGS += -nostdlib

# Images.

MOSTLYCLEANFILES += grubof_symlist.c kernel_syms.lst
DEFSYMFILES += kernel_syms.lst

grubof_HEADERS = arg.h boot.h device.h disk.h dl.h elf.h env.h err.h \
	file.h fs.h kernel.h misc.h mm.h net.h parser.h rescue.h symbol.h \
	term.h types.h powerpc/libgcc.h loader.h \
	partition.h pc_partition.h ieee1275/ieee1275.h machine/time.h \
	machine/kernel.h

grubof_symlist.c: $(addprefix include/grub/,$(grubof_HEADERS)) config.h gensymlist.sh
	/bin/sh gensymlist.sh $(filter %.h,$^) > $@ || (rm -f $@; exit 1)

kernel_syms.lst: $(addprefix include/grub/,$(grubof_HEADERS)) config.h genkernsyms.sh
	/bin/sh genkernsyms.sh $(filter %.h,$^) > $@ || (rm -f $@; exit 1)

# Programs
pkgdata_PROGRAMS = grubof

# Utilities.
bin_UTILITIES = grub-emu
sbin_UTILITIES = grub-mkimage
noinst_UTILITIES = genmoddep

# For grub-mkimage.
grub_mkimage_SOURCES = util/powerpc/ieee1275/grub-mkimage.c util/misc.c \
        util/resolve.c 
CLEANFILES += grub-mkimage grub_mkimage-util_powerpc_ieee1275_grub_mkimage.o grub_mkimage-util_misc.o grub_mkimage-util_resolve.o
MOSTLYCLEANFILES += grub_mkimage-util_powerpc_ieee1275_grub_mkimage.d grub_mkimage-util_misc.d grub_mkimage-util_resolve.d

grub-mkimage: grub_mkimage-util_powerpc_ieee1275_grub_mkimage.o grub_mkimage-util_misc.o grub_mkimage-util_resolve.o
	$(BUILD_CC) -o $@ $^ $(BUILD_LDFLAGS) $(grub_mkimage_LDFLAGS)

grub_mkimage-util_powerpc_ieee1275_grub_mkimage.o: util/powerpc/ieee1275/grub-mkimage.c
	$(BUILD_CC) -Iutil/powerpc/ieee1275 -I$(srcdir)/util/powerpc/ieee1275 $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_mkimage_CFLAGS) -c -o $@ $<

grub_mkimage-util_powerpc_ieee1275_grub_mkimage.d: util/powerpc/ieee1275/grub-mkimage.c
	set -e; 	  $(BUILD_CC) -Iutil/powerpc/ieee1275 -I$(srcdir)/util/powerpc/ieee1275 $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_mkimage_CFLAGS) -M $< 	  | sed 's,grub\-mkimage\.o[ :]*,grub_mkimage-util_powerpc_ieee1275_grub_mkimage.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_mkimage-util_powerpc_ieee1275_grub_mkimage.d

grub_mkimage-util_misc.o: util/misc.c
	$(BUILD_CC) -Iutil -I$(srcdir)/util $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_mkimage_CFLAGS) -c -o $@ $<

grub_mkimage-util_misc.d: util/misc.c
	set -e; 	  $(BUILD_CC) -Iutil -I$(srcdir)/util $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_mkimage_CFLAGS) -M $< 	  | sed 's,misc\.o[ :]*,grub_mkimage-util_misc.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_mkimage-util_misc.d

grub_mkimage-util_resolve.o: util/resolve.c
	$(BUILD_CC) -Iutil -I$(srcdir)/util $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_mkimage_CFLAGS) -c -o $@ $<

grub_mkimage-util_resolve.d: util/resolve.c
	set -e; 	  $(BUILD_CC) -Iutil -I$(srcdir)/util $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_mkimage_CFLAGS) -M $< 	  | sed 's,resolve\.o[ :]*,grub_mkimage-util_resolve.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_mkimage-util_resolve.d


# For grub-emu
grub_emu_SOURCES = commands/boot.c commands/cat.c commands/cmp.c 	\
	commands/configfile.c commands/help.c				\
	commands/search.c commands/terminal.c commands/test.c 		\
	commands/ls.c 							\
	commands/ieee1275/halt.c commands/ieee1275/reboot.c		\
	disk/loopback.c							\
	fs/affs.c fs/ext2.c fs/fat.c fs/fshelp.c fs/hfs.c fs/iso9660.c	\
	fs/jfs.c fs/minix.c fs/sfs.c fs/ufs.c fs/xfs.c fs/hfsplus.c	\
	io/gzio.c							\
	kern/device.c kern/disk.c kern/dl.c kern/env.c kern/err.c 	\
	kern/file.c kern/fs.c kern/loader.c kern/main.c kern/misc.c	\
	kern/parser.c kern/partition.c kern/rescue.c kern/term.c	\
	normal/arg.c normal/cmdline.c normal/command.c			\
	normal/completion.c normal/execute.c		 		\
	normal/function.c normal/lexer.c normal/main.c normal/menu.c 	\
	normal/menu_entry.c normal/misc.c normal/script.c		\
	partmap/amiga.c	partmap/apple.c partmap/pc.c partmap/sun.c	\
	partmap/acorn.c							\
	util/console.c util/grub-emu.c util/misc.c			\
	util/i386/pc/biosdisk.c util/i386/pc/getroot.c			\
	util/powerpc/ieee1275/misc.c grub_script.tab.c grub_emu_init.c
CLEANFILES += grub-emu grub_emu-commands_boot.o grub_emu-commands_cat.o grub_emu-commands_cmp.o grub_emu-commands_configfile.o grub_emu-commands_help.o grub_emu-commands_search.o grub_emu-commands_terminal.o grub_emu-commands_test.o grub_emu-commands_ls.o grub_emu-commands_ieee1275_halt.o grub_emu-commands_ieee1275_reboot.o grub_emu-disk_loopback.o grub_emu-fs_affs.o grub_emu-fs_ext2.o grub_emu-fs_fat.o grub_emu-fs_fshelp.o grub_emu-fs_hfs.o grub_emu-fs_iso9660.o grub_emu-fs_jfs.o grub_emu-fs_minix.o grub_emu-fs_sfs.o grub_emu-fs_ufs.o grub_emu-fs_xfs.o grub_emu-fs_hfsplus.o grub_emu-io_gzio.o grub_emu-kern_device.o grub_emu-kern_disk.o grub_emu-kern_dl.o grub_emu-kern_env.o grub_emu-kern_err.o grub_emu-kern_file.o grub_emu-kern_fs.o grub_emu-kern_loader.o grub_emu-kern_main.o grub_emu-kern_misc.o grub_emu-kern_parser.o grub_emu-kern_partition.o grub_emu-kern_rescue.o grub_emu-kern_term.o grub_emu-normal_arg.o grub_emu-normal_cmdline.o grub_emu-normal_command.o grub_emu-normal_completion.o grub_emu-normal_execute.o grub_emu-normal_function.o grub_emu-normal_lexer.o grub_emu-normal_main.o grub_emu-normal_menu.o grub_emu-normal_menu_entry.o grub_emu-normal_misc.o grub_emu-normal_script.o grub_emu-partmap_amiga.o grub_emu-partmap_apple.o grub_emu-partmap_pc.o grub_emu-partmap_sun.o grub_emu-partmap_acorn.o grub_emu-util_console.o grub_emu-util_grub_emu.o grub_emu-util_misc.o grub_emu-util_i386_pc_biosdisk.o grub_emu-util_i386_pc_getroot.o grub_emu-util_powerpc_ieee1275_misc.o grub_emu-grub_script_tab.o grub_emu-grub_emu_init.o
MOSTLYCLEANFILES += grub_emu-commands_boot.d grub_emu-commands_cat.d grub_emu-commands_cmp.d grub_emu-commands_configfile.d grub_emu-commands_help.d grub_emu-commands_search.d grub_emu-commands_terminal.d grub_emu-commands_test.d grub_emu-commands_ls.d grub_emu-commands_ieee1275_halt.d grub_emu-commands_ieee1275_reboot.d grub_emu-disk_loopback.d grub_emu-fs_affs.d grub_emu-fs_ext2.d grub_emu-fs_fat.d grub_emu-fs_fshelp.d grub_emu-fs_hfs.d grub_emu-fs_iso9660.d grub_emu-fs_jfs.d grub_emu-fs_minix.d grub_emu-fs_sfs.d grub_emu-fs_ufs.d grub_emu-fs_xfs.d grub_emu-fs_hfsplus.d grub_emu-io_gzio.d grub_emu-kern_device.d grub_emu-kern_disk.d grub_emu-kern_dl.d grub_emu-kern_env.d grub_emu-kern_err.d grub_emu-kern_file.d grub_emu-kern_fs.d grub_emu-kern_loader.d grub_emu-kern_main.d grub_emu-kern_misc.d grub_emu-kern_parser.d grub_emu-kern_partition.d grub_emu-kern_rescue.d grub_emu-kern_term.d grub_emu-normal_arg.d grub_emu-normal_cmdline.d grub_emu-normal_command.d grub_emu-normal_completion.d grub_emu-normal_execute.d grub_emu-normal_function.d grub_emu-normal_lexer.d grub_emu-normal_main.d grub_emu-normal_menu.d grub_emu-normal_menu_entry.d grub_emu-normal_misc.d grub_emu-normal_script.d grub_emu-partmap_amiga.d grub_emu-partmap_apple.d grub_emu-partmap_pc.d grub_emu-partmap_sun.d grub_emu-partmap_acorn.d grub_emu-util_console.d grub_emu-util_grub_emu.d grub_emu-util_misc.d grub_emu-util_i386_pc_biosdisk.d grub_emu-util_i386_pc_getroot.d grub_emu-util_powerpc_ieee1275_misc.d grub_emu-grub_script_tab.d grub_emu-grub_emu_init.d

grub-emu: grub_emu-commands_boot.o grub_emu-commands_cat.o grub_emu-commands_cmp.o grub_emu-commands_configfile.o grub_emu-commands_help.o grub_emu-commands_search.o grub_emu-commands_terminal.o grub_emu-commands_test.o grub_emu-commands_ls.o grub_emu-commands_ieee1275_halt.o grub_emu-commands_ieee1275_reboot.o grub_emu-disk_loopback.o grub_emu-fs_affs.o grub_emu-fs_ext2.o grub_emu-fs_fat.o grub_emu-fs_fshelp.o grub_emu-fs_hfs.o grub_emu-fs_iso9660.o grub_emu-fs_jfs.o grub_emu-fs_minix.o grub_emu-fs_sfs.o grub_emu-fs_ufs.o grub_emu-fs_xfs.o grub_emu-fs_hfsplus.o grub_emu-io_gzio.o grub_emu-kern_device.o grub_emu-kern_disk.o grub_emu-kern_dl.o grub_emu-kern_env.o grub_emu-kern_err.o grub_emu-kern_file.o grub_emu-kern_fs.o grub_emu-kern_loader.o grub_emu-kern_main.o grub_emu-kern_misc.o grub_emu-kern_parser.o grub_emu-kern_partition.o grub_emu-kern_rescue.o grub_emu-kern_term.o grub_emu-normal_arg.o grub_emu-normal_cmdline.o grub_emu-normal_command.o grub_emu-normal_completion.o grub_emu-normal_execute.o grub_emu-normal_function.o grub_emu-normal_lexer.o grub_emu-normal_main.o grub_emu-normal_menu.o grub_emu-normal_menu_entry.o grub_emu-normal_misc.o grub_emu-normal_script.o grub_emu-partmap_amiga.o grub_emu-partmap_apple.o grub_emu-partmap_pc.o grub_emu-partmap_sun.o grub_emu-partmap_acorn.o grub_emu-util_console.o grub_emu-util_grub_emu.o grub_emu-util_misc.o grub_emu-util_i386_pc_biosdisk.o grub_emu-util_i386_pc_getroot.o grub_emu-util_powerpc_ieee1275_misc.o grub_emu-grub_script_tab.o grub_emu-grub_emu_init.o
	$(BUILD_CC) -o $@ $^ $(BUILD_LDFLAGS) $(grub_emu_LDFLAGS)

grub_emu-commands_boot.o: commands/boot.c
	$(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-commands_boot.d: commands/boot.c
	set -e; 	  $(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,boot\.o[ :]*,grub_emu-commands_boot.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-commands_boot.d

grub_emu-commands_cat.o: commands/cat.c
	$(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-commands_cat.d: commands/cat.c
	set -e; 	  $(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,cat\.o[ :]*,grub_emu-commands_cat.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-commands_cat.d

grub_emu-commands_cmp.o: commands/cmp.c
	$(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-commands_cmp.d: commands/cmp.c
	set -e; 	  $(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,cmp\.o[ :]*,grub_emu-commands_cmp.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-commands_cmp.d

grub_emu-commands_configfile.o: commands/configfile.c
	$(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-commands_configfile.d: commands/configfile.c
	set -e; 	  $(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,configfile\.o[ :]*,grub_emu-commands_configfile.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-commands_configfile.d

grub_emu-commands_help.o: commands/help.c
	$(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-commands_help.d: commands/help.c
	set -e; 	  $(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,help\.o[ :]*,grub_emu-commands_help.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-commands_help.d

grub_emu-commands_search.o: commands/search.c
	$(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-commands_search.d: commands/search.c
	set -e; 	  $(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,search\.o[ :]*,grub_emu-commands_search.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-commands_search.d

grub_emu-commands_terminal.o: commands/terminal.c
	$(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-commands_terminal.d: commands/terminal.c
	set -e; 	  $(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,terminal\.o[ :]*,grub_emu-commands_terminal.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-commands_terminal.d

grub_emu-commands_test.o: commands/test.c
	$(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-commands_test.d: commands/test.c
	set -e; 	  $(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,test\.o[ :]*,grub_emu-commands_test.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-commands_test.d

grub_emu-commands_ls.o: commands/ls.c
	$(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-commands_ls.d: commands/ls.c
	set -e; 	  $(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,ls\.o[ :]*,grub_emu-commands_ls.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-commands_ls.d

grub_emu-commands_ieee1275_halt.o: commands/ieee1275/halt.c
	$(BUILD_CC) -Icommands/ieee1275 -I$(srcdir)/commands/ieee1275 $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-commands_ieee1275_halt.d: commands/ieee1275/halt.c
	set -e; 	  $(BUILD_CC) -Icommands/ieee1275 -I$(srcdir)/commands/ieee1275 $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,halt\.o[ :]*,grub_emu-commands_ieee1275_halt.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-commands_ieee1275_halt.d

grub_emu-commands_ieee1275_reboot.o: commands/ieee1275/reboot.c
	$(BUILD_CC) -Icommands/ieee1275 -I$(srcdir)/commands/ieee1275 $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-commands_ieee1275_reboot.d: commands/ieee1275/reboot.c
	set -e; 	  $(BUILD_CC) -Icommands/ieee1275 -I$(srcdir)/commands/ieee1275 $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,reboot\.o[ :]*,grub_emu-commands_ieee1275_reboot.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-commands_ieee1275_reboot.d

grub_emu-disk_loopback.o: disk/loopback.c
	$(BUILD_CC) -Idisk -I$(srcdir)/disk $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-disk_loopback.d: disk/loopback.c
	set -e; 	  $(BUILD_CC) -Idisk -I$(srcdir)/disk $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,loopback\.o[ :]*,grub_emu-disk_loopback.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-disk_loopback.d

grub_emu-fs_affs.o: fs/affs.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-fs_affs.d: fs/affs.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,affs\.o[ :]*,grub_emu-fs_affs.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-fs_affs.d

grub_emu-fs_ext2.o: fs/ext2.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-fs_ext2.d: fs/ext2.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,ext2\.o[ :]*,grub_emu-fs_ext2.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-fs_ext2.d

grub_emu-fs_fat.o: fs/fat.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-fs_fat.d: fs/fat.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,fat\.o[ :]*,grub_emu-fs_fat.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-fs_fat.d

grub_emu-fs_fshelp.o: fs/fshelp.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-fs_fshelp.d: fs/fshelp.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,fshelp\.o[ :]*,grub_emu-fs_fshelp.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-fs_fshelp.d

grub_emu-fs_hfs.o: fs/hfs.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-fs_hfs.d: fs/hfs.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,hfs\.o[ :]*,grub_emu-fs_hfs.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-fs_hfs.d

grub_emu-fs_iso9660.o: fs/iso9660.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-fs_iso9660.d: fs/iso9660.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,iso9660\.o[ :]*,grub_emu-fs_iso9660.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-fs_iso9660.d

grub_emu-fs_jfs.o: fs/jfs.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-fs_jfs.d: fs/jfs.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,jfs\.o[ :]*,grub_emu-fs_jfs.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-fs_jfs.d

grub_emu-fs_minix.o: fs/minix.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-fs_minix.d: fs/minix.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,minix\.o[ :]*,grub_emu-fs_minix.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-fs_minix.d

grub_emu-fs_sfs.o: fs/sfs.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-fs_sfs.d: fs/sfs.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,sfs\.o[ :]*,grub_emu-fs_sfs.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-fs_sfs.d

grub_emu-fs_ufs.o: fs/ufs.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-fs_ufs.d: fs/ufs.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,ufs\.o[ :]*,grub_emu-fs_ufs.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-fs_ufs.d

grub_emu-fs_xfs.o: fs/xfs.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-fs_xfs.d: fs/xfs.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,xfs\.o[ :]*,grub_emu-fs_xfs.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-fs_xfs.d

grub_emu-fs_hfsplus.o: fs/hfsplus.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-fs_hfsplus.d: fs/hfsplus.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,hfsplus\.o[ :]*,grub_emu-fs_hfsplus.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-fs_hfsplus.d

grub_emu-io_gzio.o: io/gzio.c
	$(BUILD_CC) -Iio -I$(srcdir)/io $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-io_gzio.d: io/gzio.c
	set -e; 	  $(BUILD_CC) -Iio -I$(srcdir)/io $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,gzio\.o[ :]*,grub_emu-io_gzio.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-io_gzio.d

grub_emu-kern_device.o: kern/device.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-kern_device.d: kern/device.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,device\.o[ :]*,grub_emu-kern_device.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-kern_device.d

grub_emu-kern_disk.o: kern/disk.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-kern_disk.d: kern/disk.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,disk\.o[ :]*,grub_emu-kern_disk.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-kern_disk.d

grub_emu-kern_dl.o: kern/dl.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-kern_dl.d: kern/dl.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,dl\.o[ :]*,grub_emu-kern_dl.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-kern_dl.d

grub_emu-kern_env.o: kern/env.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-kern_env.d: kern/env.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,env\.o[ :]*,grub_emu-kern_env.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-kern_env.d

grub_emu-kern_err.o: kern/err.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-kern_err.d: kern/err.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,err\.o[ :]*,grub_emu-kern_err.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-kern_err.d

grub_emu-kern_file.o: kern/file.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-kern_file.d: kern/file.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,file\.o[ :]*,grub_emu-kern_file.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-kern_file.d

grub_emu-kern_fs.o: kern/fs.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-kern_fs.d: kern/fs.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,fs\.o[ :]*,grub_emu-kern_fs.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-kern_fs.d

grub_emu-kern_loader.o: kern/loader.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-kern_loader.d: kern/loader.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,loader\.o[ :]*,grub_emu-kern_loader.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-kern_loader.d

grub_emu-kern_main.o: kern/main.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-kern_main.d: kern/main.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,main\.o[ :]*,grub_emu-kern_main.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-kern_main.d

grub_emu-kern_misc.o: kern/misc.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-kern_misc.d: kern/misc.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,misc\.o[ :]*,grub_emu-kern_misc.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-kern_misc.d

grub_emu-kern_parser.o: kern/parser.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-kern_parser.d: kern/parser.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,parser\.o[ :]*,grub_emu-kern_parser.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-kern_parser.d

grub_emu-kern_partition.o: kern/partition.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-kern_partition.d: kern/partition.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,partition\.o[ :]*,grub_emu-kern_partition.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-kern_partition.d

grub_emu-kern_rescue.o: kern/rescue.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-kern_rescue.d: kern/rescue.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,rescue\.o[ :]*,grub_emu-kern_rescue.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-kern_rescue.d

grub_emu-kern_term.o: kern/term.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-kern_term.d: kern/term.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,term\.o[ :]*,grub_emu-kern_term.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-kern_term.d

grub_emu-normal_arg.o: normal/arg.c
	$(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-normal_arg.d: normal/arg.c
	set -e; 	  $(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,arg\.o[ :]*,grub_emu-normal_arg.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-normal_arg.d

grub_emu-normal_cmdline.o: normal/cmdline.c
	$(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-normal_cmdline.d: normal/cmdline.c
	set -e; 	  $(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,cmdline\.o[ :]*,grub_emu-normal_cmdline.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-normal_cmdline.d

grub_emu-normal_command.o: normal/command.c
	$(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-normal_command.d: normal/command.c
	set -e; 	  $(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,command\.o[ :]*,grub_emu-normal_command.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-normal_command.d

grub_emu-normal_completion.o: normal/completion.c
	$(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-normal_completion.d: normal/completion.c
	set -e; 	  $(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,completion\.o[ :]*,grub_emu-normal_completion.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-normal_completion.d

grub_emu-normal_execute.o: normal/execute.c
	$(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-normal_execute.d: normal/execute.c
	set -e; 	  $(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,execute\.o[ :]*,grub_emu-normal_execute.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-normal_execute.d

grub_emu-normal_function.o: normal/function.c
	$(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-normal_function.d: normal/function.c
	set -e; 	  $(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,function\.o[ :]*,grub_emu-normal_function.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-normal_function.d

grub_emu-normal_lexer.o: normal/lexer.c
	$(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-normal_lexer.d: normal/lexer.c
	set -e; 	  $(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,lexer\.o[ :]*,grub_emu-normal_lexer.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-normal_lexer.d

grub_emu-normal_main.o: normal/main.c
	$(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-normal_main.d: normal/main.c
	set -e; 	  $(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,main\.o[ :]*,grub_emu-normal_main.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-normal_main.d

grub_emu-normal_menu.o: normal/menu.c
	$(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-normal_menu.d: normal/menu.c
	set -e; 	  $(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,menu\.o[ :]*,grub_emu-normal_menu.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-normal_menu.d

grub_emu-normal_menu_entry.o: normal/menu_entry.c
	$(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-normal_menu_entry.d: normal/menu_entry.c
	set -e; 	  $(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,menu_entry\.o[ :]*,grub_emu-normal_menu_entry.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-normal_menu_entry.d

grub_emu-normal_misc.o: normal/misc.c
	$(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-normal_misc.d: normal/misc.c
	set -e; 	  $(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,misc\.o[ :]*,grub_emu-normal_misc.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-normal_misc.d

grub_emu-normal_script.o: normal/script.c
	$(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-normal_script.d: normal/script.c
	set -e; 	  $(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,script\.o[ :]*,grub_emu-normal_script.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-normal_script.d

grub_emu-partmap_amiga.o: partmap/amiga.c
	$(BUILD_CC) -Ipartmap -I$(srcdir)/partmap $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-partmap_amiga.d: partmap/amiga.c
	set -e; 	  $(BUILD_CC) -Ipartmap -I$(srcdir)/partmap $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,amiga\.o[ :]*,grub_emu-partmap_amiga.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-partmap_amiga.d

grub_emu-partmap_apple.o: partmap/apple.c
	$(BUILD_CC) -Ipartmap -I$(srcdir)/partmap $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-partmap_apple.d: partmap/apple.c
	set -e; 	  $(BUILD_CC) -Ipartmap -I$(srcdir)/partmap $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,apple\.o[ :]*,grub_emu-partmap_apple.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-partmap_apple.d

grub_emu-partmap_pc.o: partmap/pc.c
	$(BUILD_CC) -Ipartmap -I$(srcdir)/partmap $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-partmap_pc.d: partmap/pc.c
	set -e; 	  $(BUILD_CC) -Ipartmap -I$(srcdir)/partmap $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,pc\.o[ :]*,grub_emu-partmap_pc.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-partmap_pc.d

grub_emu-partmap_sun.o: partmap/sun.c
	$(BUILD_CC) -Ipartmap -I$(srcdir)/partmap $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-partmap_sun.d: partmap/sun.c
	set -e; 	  $(BUILD_CC) -Ipartmap -I$(srcdir)/partmap $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,sun\.o[ :]*,grub_emu-partmap_sun.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-partmap_sun.d

grub_emu-partmap_acorn.o: partmap/acorn.c
	$(BUILD_CC) -Ipartmap -I$(srcdir)/partmap $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-partmap_acorn.d: partmap/acorn.c
	set -e; 	  $(BUILD_CC) -Ipartmap -I$(srcdir)/partmap $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,acorn\.o[ :]*,grub_emu-partmap_acorn.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-partmap_acorn.d

grub_emu-util_console.o: util/console.c
	$(BUILD_CC) -Iutil -I$(srcdir)/util $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-util_console.d: util/console.c
	set -e; 	  $(BUILD_CC) -Iutil -I$(srcdir)/util $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,console\.o[ :]*,grub_emu-util_console.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-util_console.d

grub_emu-util_grub_emu.o: util/grub-emu.c
	$(BUILD_CC) -Iutil -I$(srcdir)/util $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-util_grub_emu.d: util/grub-emu.c
	set -e; 	  $(BUILD_CC) -Iutil -I$(srcdir)/util $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,grub\-emu\.o[ :]*,grub_emu-util_grub_emu.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-util_grub_emu.d

grub_emu-util_misc.o: util/misc.c
	$(BUILD_CC) -Iutil -I$(srcdir)/util $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-util_misc.d: util/misc.c
	set -e; 	  $(BUILD_CC) -Iutil -I$(srcdir)/util $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,misc\.o[ :]*,grub_emu-util_misc.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-util_misc.d

grub_emu-util_i386_pc_biosdisk.o: util/i386/pc/biosdisk.c
	$(BUILD_CC) -Iutil/i386/pc -I$(srcdir)/util/i386/pc $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-util_i386_pc_biosdisk.d: util/i386/pc/biosdisk.c
	set -e; 	  $(BUILD_CC) -Iutil/i386/pc -I$(srcdir)/util/i386/pc $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,biosdisk\.o[ :]*,grub_emu-util_i386_pc_biosdisk.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-util_i386_pc_biosdisk.d

grub_emu-util_i386_pc_getroot.o: util/i386/pc/getroot.c
	$(BUILD_CC) -Iutil/i386/pc -I$(srcdir)/util/i386/pc $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-util_i386_pc_getroot.d: util/i386/pc/getroot.c
	set -e; 	  $(BUILD_CC) -Iutil/i386/pc -I$(srcdir)/util/i386/pc $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,getroot\.o[ :]*,grub_emu-util_i386_pc_getroot.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-util_i386_pc_getroot.d

grub_emu-util_powerpc_ieee1275_misc.o: util/powerpc/ieee1275/misc.c
	$(BUILD_CC) -Iutil/powerpc/ieee1275 -I$(srcdir)/util/powerpc/ieee1275 $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-util_powerpc_ieee1275_misc.d: util/powerpc/ieee1275/misc.c
	set -e; 	  $(BUILD_CC) -Iutil/powerpc/ieee1275 -I$(srcdir)/util/powerpc/ieee1275 $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,misc\.o[ :]*,grub_emu-util_powerpc_ieee1275_misc.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-util_powerpc_ieee1275_misc.d

grub_emu-grub_script_tab.o: grub_script.tab.c
	$(BUILD_CC) -I. -I$(srcdir)/. $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-grub_script_tab.d: grub_script.tab.c
	set -e; 	  $(BUILD_CC) -I. -I$(srcdir)/. $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,grub_script\.tab\.o[ :]*,grub_emu-grub_script_tab.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-grub_script_tab.d

grub_emu-grub_emu_init.o: grub_emu_init.c
	$(BUILD_CC) -I. -I$(srcdir)/. $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-grub_emu_init.d: grub_emu_init.c
	set -e; 	  $(BUILD_CC) -I. -I$(srcdir)/. $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,grub_emu_init\.o[ :]*,grub_emu-grub_emu_init.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-grub_emu_init.d


grub_emu_LDFLAGS = $(LIBCURSES)

grubof_SOURCES = kern/powerpc/ieee1275/crt0.S kern/powerpc/ieee1275/cmain.c \
	kern/ieee1275/ieee1275.c kern/main.c kern/device.c 		\
	kern/disk.c kern/dl.c kern/file.c kern/fs.c kern/err.c 		\
	kern/misc.c kern/mm.c kern/loader.c kern/rescue.c kern/term.c 	\
	kern/powerpc/ieee1275/init.c term/ieee1275/ofconsole.c 		\
	kern/powerpc/ieee1275/openfw.c disk/ieee1275/ofdisk.c 		\
	kern/parser.c kern/partition.c kern/env.c kern/powerpc/dl.c 	\
	grubof_symlist.c kern/powerpc/cache.S
CLEANFILES += grubof grubof-kern_powerpc_ieee1275_crt0.o grubof-kern_powerpc_ieee1275_cmain.o grubof-kern_ieee1275_ieee1275.o grubof-kern_main.o grubof-kern_device.o grubof-kern_disk.o grubof-kern_dl.o grubof-kern_file.o grubof-kern_fs.o grubof-kern_err.o grubof-kern_misc.o grubof-kern_mm.o grubof-kern_loader.o grubof-kern_rescue.o grubof-kern_term.o grubof-kern_powerpc_ieee1275_init.o grubof-term_ieee1275_ofconsole.o grubof-kern_powerpc_ieee1275_openfw.o grubof-disk_ieee1275_ofdisk.o grubof-kern_parser.o grubof-kern_partition.o grubof-kern_env.o grubof-kern_powerpc_dl.o grubof-grubof_symlist.o grubof-kern_powerpc_cache.o
MOSTLYCLEANFILES += grubof-kern_powerpc_ieee1275_crt0.d grubof-kern_powerpc_ieee1275_cmain.d grubof-kern_ieee1275_ieee1275.d grubof-kern_main.d grubof-kern_device.d grubof-kern_disk.d grubof-kern_dl.d grubof-kern_file.d grubof-kern_fs.d grubof-kern_err.d grubof-kern_misc.d grubof-kern_mm.d grubof-kern_loader.d grubof-kern_rescue.d grubof-kern_term.d grubof-kern_powerpc_ieee1275_init.d grubof-term_ieee1275_ofconsole.d grubof-kern_powerpc_ieee1275_openfw.d grubof-disk_ieee1275_ofdisk.d grubof-kern_parser.d grubof-kern_partition.d grubof-kern_env.d grubof-kern_powerpc_dl.d grubof-grubof_symlist.d grubof-kern_powerpc_cache.d

grubof: grubof-kern_powerpc_ieee1275_crt0.o grubof-kern_powerpc_ieee1275_cmain.o grubof-kern_ieee1275_ieee1275.o grubof-kern_main.o grubof-kern_device.o grubof-kern_disk.o grubof-kern_dl.o grubof-kern_file.o grubof-kern_fs.o grubof-kern_err.o grubof-kern_misc.o grubof-kern_mm.o grubof-kern_loader.o grubof-kern_rescue.o grubof-kern_term.o grubof-kern_powerpc_ieee1275_init.o grubof-term_ieee1275_ofconsole.o grubof-kern_powerpc_ieee1275_openfw.o grubof-disk_ieee1275_ofdisk.o grubof-kern_parser.o grubof-kern_partition.o grubof-kern_env.o grubof-kern_powerpc_dl.o grubof-grubof_symlist.o grubof-kern_powerpc_cache.o
	$(CC) -o $@ $^ $(LDFLAGS) $(grubof_LDFLAGS)

grubof-kern_powerpc_ieee1275_crt0.o: kern/powerpc/ieee1275/crt0.S
	$(CC) -Ikern/powerpc/ieee1275 -I$(srcdir)/kern/powerpc/ieee1275 $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_powerpc_ieee1275_crt0.d: kern/powerpc/ieee1275/crt0.S
	set -e; 	  $(CC) -Ikern/powerpc/ieee1275 -I$(srcdir)/kern/powerpc/ieee1275 $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -M $< 	  | sed 's,crt0\.o[ :]*,grubof-kern_powerpc_ieee1275_crt0.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_powerpc_ieee1275_crt0.d

grubof-kern_powerpc_ieee1275_cmain.o: kern/powerpc/ieee1275/cmain.c
	$(CC) -Ikern/powerpc/ieee1275 -I$(srcdir)/kern/powerpc/ieee1275 $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_powerpc_ieee1275_cmain.d: kern/powerpc/ieee1275/cmain.c
	set -e; 	  $(CC) -Ikern/powerpc/ieee1275 -I$(srcdir)/kern/powerpc/ieee1275 $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -M $< 	  | sed 's,cmain\.o[ :]*,grubof-kern_powerpc_ieee1275_cmain.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_powerpc_ieee1275_cmain.d

grubof-kern_ieee1275_ieee1275.o: kern/ieee1275/ieee1275.c
	$(CC) -Ikern/ieee1275 -I$(srcdir)/kern/ieee1275 $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_ieee1275_ieee1275.d: kern/ieee1275/ieee1275.c
	set -e; 	  $(CC) -Ikern/ieee1275 -I$(srcdir)/kern/ieee1275 $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -M $< 	  | sed 's,ieee1275\.o[ :]*,grubof-kern_ieee1275_ieee1275.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_ieee1275_ieee1275.d

grubof-kern_main.o: kern/main.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_main.d: kern/main.c
	set -e; 	  $(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -M $< 	  | sed 's,main\.o[ :]*,grubof-kern_main.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_main.d

grubof-kern_device.o: kern/device.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_device.d: kern/device.c
	set -e; 	  $(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -M $< 	  | sed 's,device\.o[ :]*,grubof-kern_device.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_device.d

grubof-kern_disk.o: kern/disk.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_disk.d: kern/disk.c
	set -e; 	  $(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -M $< 	  | sed 's,disk\.o[ :]*,grubof-kern_disk.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_disk.d

grubof-kern_dl.o: kern/dl.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_dl.d: kern/dl.c
	set -e; 	  $(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -M $< 	  | sed 's,dl\.o[ :]*,grubof-kern_dl.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_dl.d

grubof-kern_file.o: kern/file.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_file.d: kern/file.c
	set -e; 	  $(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -M $< 	  | sed 's,file\.o[ :]*,grubof-kern_file.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_file.d

grubof-kern_fs.o: kern/fs.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_fs.d: kern/fs.c
	set -e; 	  $(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -M $< 	  | sed 's,fs\.o[ :]*,grubof-kern_fs.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_fs.d

grubof-kern_err.o: kern/err.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_err.d: kern/err.c
	set -e; 	  $(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -M $< 	  | sed 's,err\.o[ :]*,grubof-kern_err.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_err.d

grubof-kern_misc.o: kern/misc.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_misc.d: kern/misc.c
	set -e; 	  $(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -M $< 	  | sed 's,misc\.o[ :]*,grubof-kern_misc.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_misc.d

grubof-kern_mm.o: kern/mm.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_mm.d: kern/mm.c
	set -e; 	  $(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -M $< 	  | sed 's,mm\.o[ :]*,grubof-kern_mm.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_mm.d

grubof-kern_loader.o: kern/loader.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_loader.d: kern/loader.c
	set -e; 	  $(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -M $< 	  | sed 's,loader\.o[ :]*,grubof-kern_loader.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_loader.d

grubof-kern_rescue.o: kern/rescue.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_rescue.d: kern/rescue.c
	set -e; 	  $(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -M $< 	  | sed 's,rescue\.o[ :]*,grubof-kern_rescue.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_rescue.d

grubof-kern_term.o: kern/term.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_term.d: kern/term.c
	set -e; 	  $(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -M $< 	  | sed 's,term\.o[ :]*,grubof-kern_term.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_term.d

grubof-kern_powerpc_ieee1275_init.o: kern/powerpc/ieee1275/init.c
	$(CC) -Ikern/powerpc/ieee1275 -I$(srcdir)/kern/powerpc/ieee1275 $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_powerpc_ieee1275_init.d: kern/powerpc/ieee1275/init.c
	set -e; 	  $(CC) -Ikern/powerpc/ieee1275 -I$(srcdir)/kern/powerpc/ieee1275 $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -M $< 	  | sed 's,init\.o[ :]*,grubof-kern_powerpc_ieee1275_init.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_powerpc_ieee1275_init.d

grubof-term_ieee1275_ofconsole.o: term/ieee1275/ofconsole.c
	$(CC) -Iterm/ieee1275 -I$(srcdir)/term/ieee1275 $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -c -o $@ $<

grubof-term_ieee1275_ofconsole.d: term/ieee1275/ofconsole.c
	set -e; 	  $(CC) -Iterm/ieee1275 -I$(srcdir)/term/ieee1275 $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -M $< 	  | sed 's,ofconsole\.o[ :]*,grubof-term_ieee1275_ofconsole.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-term_ieee1275_ofconsole.d

grubof-kern_powerpc_ieee1275_openfw.o: kern/powerpc/ieee1275/openfw.c
	$(CC) -Ikern/powerpc/ieee1275 -I$(srcdir)/kern/powerpc/ieee1275 $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_powerpc_ieee1275_openfw.d: kern/powerpc/ieee1275/openfw.c
	set -e; 	  $(CC) -Ikern/powerpc/ieee1275 -I$(srcdir)/kern/powerpc/ieee1275 $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -M $< 	  | sed 's,openfw\.o[ :]*,grubof-kern_powerpc_ieee1275_openfw.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_powerpc_ieee1275_openfw.d

grubof-disk_ieee1275_ofdisk.o: disk/ieee1275/ofdisk.c
	$(CC) -Idisk/ieee1275 -I$(srcdir)/disk/ieee1275 $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -c -o $@ $<

grubof-disk_ieee1275_ofdisk.d: disk/ieee1275/ofdisk.c
	set -e; 	  $(CC) -Idisk/ieee1275 -I$(srcdir)/disk/ieee1275 $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -M $< 	  | sed 's,ofdisk\.o[ :]*,grubof-disk_ieee1275_ofdisk.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-disk_ieee1275_ofdisk.d

grubof-kern_parser.o: kern/parser.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_parser.d: kern/parser.c
	set -e; 	  $(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -M $< 	  | sed 's,parser\.o[ :]*,grubof-kern_parser.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_parser.d

grubof-kern_partition.o: kern/partition.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_partition.d: kern/partition.c
	set -e; 	  $(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -M $< 	  | sed 's,partition\.o[ :]*,grubof-kern_partition.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_partition.d

grubof-kern_env.o: kern/env.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_env.d: kern/env.c
	set -e; 	  $(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -M $< 	  | sed 's,env\.o[ :]*,grubof-kern_env.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_env.d

grubof-kern_powerpc_dl.o: kern/powerpc/dl.c
	$(CC) -Ikern/powerpc -I$(srcdir)/kern/powerpc $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_powerpc_dl.d: kern/powerpc/dl.c
	set -e; 	  $(CC) -Ikern/powerpc -I$(srcdir)/kern/powerpc $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -M $< 	  | sed 's,dl\.o[ :]*,grubof-kern_powerpc_dl.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_powerpc_dl.d

grubof-grubof_symlist.o: grubof_symlist.c
	$(CC) -I. -I$(srcdir)/. $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -c -o $@ $<

grubof-grubof_symlist.d: grubof_symlist.c
	set -e; 	  $(CC) -I. -I$(srcdir)/. $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -M $< 	  | sed 's,grubof_symlist\.o[ :]*,grubof-grubof_symlist.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-grubof_symlist.d

grubof-kern_powerpc_cache.o: kern/powerpc/cache.S
	$(CC) -Ikern/powerpc -I$(srcdir)/kern/powerpc $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_powerpc_cache.d: kern/powerpc/cache.S
	set -e; 	  $(CC) -Ikern/powerpc -I$(srcdir)/kern/powerpc $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -M $< 	  | sed 's,cache\.o[ :]*,grubof-kern_powerpc_cache.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_powerpc_cache.d

grubof_HEADERS = grub/powerpc/ieee1275/ieee1275.h
grubof_CFLAGS = $(COMMON_CFLAGS)
grubof_ASFLAGS = $(COMMON_ASFLAGS)
grubof_LDFLAGS = $(COMMON_LDFLAGS) -static-libgcc -lgcc \
	-Wl,-N,-S,-Ttext,0x200000,-Bstatic

# For genmoddep.
genmoddep_SOURCES = util/genmoddep.c
CLEANFILES += genmoddep genmoddep-util_genmoddep.o
MOSTLYCLEANFILES += genmoddep-util_genmoddep.d

genmoddep: genmoddep-util_genmoddep.o
	$(BUILD_CC) -o $@ $^ $(BUILD_LDFLAGS) $(genmoddep_LDFLAGS)

genmoddep-util_genmoddep.o: util/genmoddep.c
	$(BUILD_CC) -Iutil -I$(srcdir)/util $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(genmoddep_CFLAGS) -c -o $@ $<

genmoddep-util_genmoddep.d: util/genmoddep.c
	set -e; 	  $(BUILD_CC) -Iutil -I$(srcdir)/util $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(genmoddep_CFLAGS) -M $< 	  | sed 's,genmoddep\.o[ :]*,genmoddep-util_genmoddep.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include genmoddep-util_genmoddep.d


# Scripts.
sbin_SCRIPTS = grub-install

# For grub-install.
grub_install_SOURCES = util/powerpc/ieee1275/grub-install.in
CLEANFILES += grub-install

grub-install: util/powerpc/ieee1275/grub-install.in config.status
	./config.status --file=grub-install:util/powerpc/ieee1275/grub-install.in
	chmod +x $@


# Modules.
pkgdata_MODULES = halt.mod \
	_linux.mod \
	linux.mod \
	normal.mod \
	reboot.mod \
	suspend.mod

# For _linux.mod.
_linux_mod_SOURCES = loader/powerpc/ieee1275/linux.c
CLEANFILES += _linux.mod mod-_linux.o mod-_linux.c pre-_linux.o _linux_mod-loader_powerpc_ieee1275_linux.o und-_linux.lst
ifneq ($(_linux_mod_EXPORTS),no)
CLEANFILES += def-_linux.lst
DEFSYMFILES += def-_linux.lst
endif
MOSTLYCLEANFILES += _linux_mod-loader_powerpc_ieee1275_linux.d
UNDSYMFILES += und-_linux.lst

_linux.mod: pre-_linux.o mod-_linux.o
	-rm -f $@
	$(CC) $(_linux_mod_LDFLAGS) $(LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-_linux.o: _linux_mod-loader_powerpc_ieee1275_linux.o
	-rm -f $@
	$(CC) $(_linux_mod_LDFLAGS) $(LDFLAGS) -Wl,-r,-d -o $@ $^

mod-_linux.o: mod-_linux.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(_linux_mod_CFLAGS) -c -o $@ $<

mod-_linux.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh '_linux' $< > $@ || (rm -f $@; exit 1)

ifneq ($(_linux_mod_EXPORTS),no)
def-_linux.lst: pre-_linux.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 _linux/' > $@
endif

und-_linux.lst: pre-_linux.o
	echo '_linux' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

_linux_mod-loader_powerpc_ieee1275_linux.o: loader/powerpc/ieee1275/linux.c
	$(CC) -Iloader/powerpc/ieee1275 -I$(srcdir)/loader/powerpc/ieee1275 $(CPPFLAGS) $(CFLAGS) $(_linux_mod_CFLAGS) -c -o $@ $<

_linux_mod-loader_powerpc_ieee1275_linux.d: loader/powerpc/ieee1275/linux.c
	set -e; 	  $(CC) -Iloader/powerpc/ieee1275 -I$(srcdir)/loader/powerpc/ieee1275 $(CPPFLAGS) $(CFLAGS) $(_linux_mod_CFLAGS) -M $< 	  | sed 's,linux\.o[ :]*,_linux_mod-loader_powerpc_ieee1275_linux.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include _linux_mod-loader_powerpc_ieee1275_linux.d

CLEANFILES += cmd-_linux_mod-loader_powerpc_ieee1275_linux.lst fs-_linux_mod-loader_powerpc_ieee1275_linux.lst
COMMANDFILES += cmd-_linux_mod-loader_powerpc_ieee1275_linux.lst
FSFILES += fs-_linux_mod-loader_powerpc_ieee1275_linux.lst

cmd-_linux_mod-loader_powerpc_ieee1275_linux.lst: loader/powerpc/ieee1275/linux.c gencmdlist.sh
	set -e; 	  $(CC) -Iloader/powerpc/ieee1275 -I$(srcdir)/loader/powerpc/ieee1275 $(CPPFLAGS) $(CFLAGS) $(_linux_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh _linux > $@ || (rm -f $@; exit 1)

fs-_linux_mod-loader_powerpc_ieee1275_linux.lst: loader/powerpc/ieee1275/linux.c genfslist.sh
	set -e; 	  $(CC) -Iloader/powerpc/ieee1275 -I$(srcdir)/loader/powerpc/ieee1275 $(CPPFLAGS) $(CFLAGS) $(_linux_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh _linux > $@ || (rm -f $@; exit 1)


_linux_mod_CFLAGS = $(COMMON_CFLAGS)
_linux_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For linux.mod.
linux_mod_SOURCES = loader/powerpc/ieee1275/linux_normal.c
CLEANFILES += linux.mod mod-linux.o mod-linux.c pre-linux.o linux_mod-loader_powerpc_ieee1275_linux_normal.o und-linux.lst
ifneq ($(linux_mod_EXPORTS),no)
CLEANFILES += def-linux.lst
DEFSYMFILES += def-linux.lst
endif
MOSTLYCLEANFILES += linux_mod-loader_powerpc_ieee1275_linux_normal.d
UNDSYMFILES += und-linux.lst

linux.mod: pre-linux.o mod-linux.o
	-rm -f $@
	$(CC) $(linux_mod_LDFLAGS) $(LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-linux.o: linux_mod-loader_powerpc_ieee1275_linux_normal.o
	-rm -f $@
	$(CC) $(linux_mod_LDFLAGS) $(LDFLAGS) -Wl,-r,-d -o $@ $^

mod-linux.o: mod-linux.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(linux_mod_CFLAGS) -c -o $@ $<

mod-linux.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'linux' $< > $@ || (rm -f $@; exit 1)

ifneq ($(linux_mod_EXPORTS),no)
def-linux.lst: pre-linux.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 linux/' > $@
endif

und-linux.lst: pre-linux.o
	echo 'linux' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

linux_mod-loader_powerpc_ieee1275_linux_normal.o: loader/powerpc/ieee1275/linux_normal.c
	$(CC) -Iloader/powerpc/ieee1275 -I$(srcdir)/loader/powerpc/ieee1275 $(CPPFLAGS) $(CFLAGS) $(linux_mod_CFLAGS) -c -o $@ $<

linux_mod-loader_powerpc_ieee1275_linux_normal.d: loader/powerpc/ieee1275/linux_normal.c
	set -e; 	  $(CC) -Iloader/powerpc/ieee1275 -I$(srcdir)/loader/powerpc/ieee1275 $(CPPFLAGS) $(CFLAGS) $(linux_mod_CFLAGS) -M $< 	  | sed 's,linux_normal\.o[ :]*,linux_mod-loader_powerpc_ieee1275_linux_normal.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include linux_mod-loader_powerpc_ieee1275_linux_normal.d

CLEANFILES += cmd-linux_mod-loader_powerpc_ieee1275_linux_normal.lst fs-linux_mod-loader_powerpc_ieee1275_linux_normal.lst
COMMANDFILES += cmd-linux_mod-loader_powerpc_ieee1275_linux_normal.lst
FSFILES += fs-linux_mod-loader_powerpc_ieee1275_linux_normal.lst

cmd-linux_mod-loader_powerpc_ieee1275_linux_normal.lst: loader/powerpc/ieee1275/linux_normal.c gencmdlist.sh
	set -e; 	  $(CC) -Iloader/powerpc/ieee1275 -I$(srcdir)/loader/powerpc/ieee1275 $(CPPFLAGS) $(CFLAGS) $(linux_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh linux > $@ || (rm -f $@; exit 1)

fs-linux_mod-loader_powerpc_ieee1275_linux_normal.lst: loader/powerpc/ieee1275/linux_normal.c genfslist.sh
	set -e; 	  $(CC) -Iloader/powerpc/ieee1275 -I$(srcdir)/loader/powerpc/ieee1275 $(CPPFLAGS) $(CFLAGS) $(linux_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh linux > $@ || (rm -f $@; exit 1)


linux_mod_CFLAGS = $(COMMON_CFLAGS)
linux_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For normal.mod.
normal_mod_SOURCES = normal/arg.c normal/cmdline.c normal/command.c	\
	normal/completion.c normal/execute.c		 		\
	normal/function.c normal/lexer.c normal/main.c normal/menu.c	\
	normal/menu_entry.c normal/misc.c grub_script.tab.c 		\
	normal/script.c normal/powerpc/setjmp.S
CLEANFILES += normal.mod mod-normal.o mod-normal.c pre-normal.o normal_mod-normal_arg.o normal_mod-normal_cmdline.o normal_mod-normal_command.o normal_mod-normal_completion.o normal_mod-normal_execute.o normal_mod-normal_function.o normal_mod-normal_lexer.o normal_mod-normal_main.o normal_mod-normal_menu.o normal_mod-normal_menu_entry.o normal_mod-normal_misc.o normal_mod-grub_script_tab.o normal_mod-normal_script.o normal_mod-normal_powerpc_setjmp.o und-normal.lst
ifneq ($(normal_mod_EXPORTS),no)
CLEANFILES += def-normal.lst
DEFSYMFILES += def-normal.lst
endif
MOSTLYCLEANFILES += normal_mod-normal_arg.d normal_mod-normal_cmdline.d normal_mod-normal_command.d normal_mod-normal_completion.d normal_mod-normal_execute.d normal_mod-normal_function.d normal_mod-normal_lexer.d normal_mod-normal_main.d normal_mod-normal_menu.d normal_mod-normal_menu_entry.d normal_mod-normal_misc.d normal_mod-grub_script_tab.d normal_mod-normal_script.d normal_mod-normal_powerpc_setjmp.d
UNDSYMFILES += und-normal.lst

normal.mod: pre-normal.o mod-normal.o
	-rm -f $@
	$(CC) $(normal_mod_LDFLAGS) $(LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-normal.o: normal_mod-normal_arg.o normal_mod-normal_cmdline.o normal_mod-normal_command.o normal_mod-normal_completion.o normal_mod-normal_execute.o normal_mod-normal_function.o normal_mod-normal_lexer.o normal_mod-normal_main.o normal_mod-normal_menu.o normal_mod-normal_menu_entry.o normal_mod-normal_misc.o normal_mod-grub_script_tab.o normal_mod-normal_script.o normal_mod-normal_powerpc_setjmp.o
	-rm -f $@
	$(CC) $(normal_mod_LDFLAGS) $(LDFLAGS) -Wl,-r,-d -o $@ $^

mod-normal.o: mod-normal.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -c -o $@ $<

mod-normal.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'normal' $< > $@ || (rm -f $@; exit 1)

ifneq ($(normal_mod_EXPORTS),no)
def-normal.lst: pre-normal.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 normal/' > $@
endif

und-normal.lst: pre-normal.o
	echo 'normal' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

normal_mod-normal_arg.o: normal/arg.c
	$(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -c -o $@ $<

normal_mod-normal_arg.d: normal/arg.c
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -M $< 	  | sed 's,arg\.o[ :]*,normal_mod-normal_arg.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include normal_mod-normal_arg.d

CLEANFILES += cmd-normal_mod-normal_arg.lst fs-normal_mod-normal_arg.lst
COMMANDFILES += cmd-normal_mod-normal_arg.lst
FSFILES += fs-normal_mod-normal_arg.lst

cmd-normal_mod-normal_arg.lst: normal/arg.c gencmdlist.sh
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-normal_arg.lst: normal/arg.c genfslist.sh
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_cmdline.o: normal/cmdline.c
	$(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -c -o $@ $<

normal_mod-normal_cmdline.d: normal/cmdline.c
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -M $< 	  | sed 's,cmdline\.o[ :]*,normal_mod-normal_cmdline.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include normal_mod-normal_cmdline.d

CLEANFILES += cmd-normal_mod-normal_cmdline.lst fs-normal_mod-normal_cmdline.lst
COMMANDFILES += cmd-normal_mod-normal_cmdline.lst
FSFILES += fs-normal_mod-normal_cmdline.lst

cmd-normal_mod-normal_cmdline.lst: normal/cmdline.c gencmdlist.sh
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-normal_cmdline.lst: normal/cmdline.c genfslist.sh
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_command.o: normal/command.c
	$(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -c -o $@ $<

normal_mod-normal_command.d: normal/command.c
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -M $< 	  | sed 's,command\.o[ :]*,normal_mod-normal_command.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include normal_mod-normal_command.d

CLEANFILES += cmd-normal_mod-normal_command.lst fs-normal_mod-normal_command.lst
COMMANDFILES += cmd-normal_mod-normal_command.lst
FSFILES += fs-normal_mod-normal_command.lst

cmd-normal_mod-normal_command.lst: normal/command.c gencmdlist.sh
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-normal_command.lst: normal/command.c genfslist.sh
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_completion.o: normal/completion.c
	$(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -c -o $@ $<

normal_mod-normal_completion.d: normal/completion.c
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -M $< 	  | sed 's,completion\.o[ :]*,normal_mod-normal_completion.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include normal_mod-normal_completion.d

CLEANFILES += cmd-normal_mod-normal_completion.lst fs-normal_mod-normal_completion.lst
COMMANDFILES += cmd-normal_mod-normal_completion.lst
FSFILES += fs-normal_mod-normal_completion.lst

cmd-normal_mod-normal_completion.lst: normal/completion.c gencmdlist.sh
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-normal_completion.lst: normal/completion.c genfslist.sh
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_execute.o: normal/execute.c
	$(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -c -o $@ $<

normal_mod-normal_execute.d: normal/execute.c
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -M $< 	  | sed 's,execute\.o[ :]*,normal_mod-normal_execute.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include normal_mod-normal_execute.d

CLEANFILES += cmd-normal_mod-normal_execute.lst fs-normal_mod-normal_execute.lst
COMMANDFILES += cmd-normal_mod-normal_execute.lst
FSFILES += fs-normal_mod-normal_execute.lst

cmd-normal_mod-normal_execute.lst: normal/execute.c gencmdlist.sh
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-normal_execute.lst: normal/execute.c genfslist.sh
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_function.o: normal/function.c
	$(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -c -o $@ $<

normal_mod-normal_function.d: normal/function.c
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -M $< 	  | sed 's,function\.o[ :]*,normal_mod-normal_function.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include normal_mod-normal_function.d

CLEANFILES += cmd-normal_mod-normal_function.lst fs-normal_mod-normal_function.lst
COMMANDFILES += cmd-normal_mod-normal_function.lst
FSFILES += fs-normal_mod-normal_function.lst

cmd-normal_mod-normal_function.lst: normal/function.c gencmdlist.sh
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-normal_function.lst: normal/function.c genfslist.sh
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_lexer.o: normal/lexer.c
	$(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -c -o $@ $<

normal_mod-normal_lexer.d: normal/lexer.c
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -M $< 	  | sed 's,lexer\.o[ :]*,normal_mod-normal_lexer.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include normal_mod-normal_lexer.d

CLEANFILES += cmd-normal_mod-normal_lexer.lst fs-normal_mod-normal_lexer.lst
COMMANDFILES += cmd-normal_mod-normal_lexer.lst
FSFILES += fs-normal_mod-normal_lexer.lst

cmd-normal_mod-normal_lexer.lst: normal/lexer.c gencmdlist.sh
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-normal_lexer.lst: normal/lexer.c genfslist.sh
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_main.o: normal/main.c
	$(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -c -o $@ $<

normal_mod-normal_main.d: normal/main.c
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -M $< 	  | sed 's,main\.o[ :]*,normal_mod-normal_main.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include normal_mod-normal_main.d

CLEANFILES += cmd-normal_mod-normal_main.lst fs-normal_mod-normal_main.lst
COMMANDFILES += cmd-normal_mod-normal_main.lst
FSFILES += fs-normal_mod-normal_main.lst

cmd-normal_mod-normal_main.lst: normal/main.c gencmdlist.sh
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-normal_main.lst: normal/main.c genfslist.sh
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_menu.o: normal/menu.c
	$(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -c -o $@ $<

normal_mod-normal_menu.d: normal/menu.c
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -M $< 	  | sed 's,menu\.o[ :]*,normal_mod-normal_menu.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include normal_mod-normal_menu.d

CLEANFILES += cmd-normal_mod-normal_menu.lst fs-normal_mod-normal_menu.lst
COMMANDFILES += cmd-normal_mod-normal_menu.lst
FSFILES += fs-normal_mod-normal_menu.lst

cmd-normal_mod-normal_menu.lst: normal/menu.c gencmdlist.sh
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-normal_menu.lst: normal/menu.c genfslist.sh
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_menu_entry.o: normal/menu_entry.c
	$(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -c -o $@ $<

normal_mod-normal_menu_entry.d: normal/menu_entry.c
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -M $< 	  | sed 's,menu_entry\.o[ :]*,normal_mod-normal_menu_entry.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include normal_mod-normal_menu_entry.d

CLEANFILES += cmd-normal_mod-normal_menu_entry.lst fs-normal_mod-normal_menu_entry.lst
COMMANDFILES += cmd-normal_mod-normal_menu_entry.lst
FSFILES += fs-normal_mod-normal_menu_entry.lst

cmd-normal_mod-normal_menu_entry.lst: normal/menu_entry.c gencmdlist.sh
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-normal_menu_entry.lst: normal/menu_entry.c genfslist.sh
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_misc.o: normal/misc.c
	$(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -c -o $@ $<

normal_mod-normal_misc.d: normal/misc.c
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -M $< 	  | sed 's,misc\.o[ :]*,normal_mod-normal_misc.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include normal_mod-normal_misc.d

CLEANFILES += cmd-normal_mod-normal_misc.lst fs-normal_mod-normal_misc.lst
COMMANDFILES += cmd-normal_mod-normal_misc.lst
FSFILES += fs-normal_mod-normal_misc.lst

cmd-normal_mod-normal_misc.lst: normal/misc.c gencmdlist.sh
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-normal_misc.lst: normal/misc.c genfslist.sh
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-grub_script_tab.o: grub_script.tab.c
	$(CC) -I. -I$(srcdir)/. $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -c -o $@ $<

normal_mod-grub_script_tab.d: grub_script.tab.c
	set -e; 	  $(CC) -I. -I$(srcdir)/. $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -M $< 	  | sed 's,grub_script\.tab\.o[ :]*,normal_mod-grub_script_tab.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include normal_mod-grub_script_tab.d

CLEANFILES += cmd-normal_mod-grub_script_tab.lst fs-normal_mod-grub_script_tab.lst
COMMANDFILES += cmd-normal_mod-grub_script_tab.lst
FSFILES += fs-normal_mod-grub_script_tab.lst

cmd-normal_mod-grub_script_tab.lst: grub_script.tab.c gencmdlist.sh
	set -e; 	  $(CC) -I. -I$(srcdir)/. $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-grub_script_tab.lst: grub_script.tab.c genfslist.sh
	set -e; 	  $(CC) -I. -I$(srcdir)/. $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_script.o: normal/script.c
	$(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -c -o $@ $<

normal_mod-normal_script.d: normal/script.c
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -M $< 	  | sed 's,script\.o[ :]*,normal_mod-normal_script.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include normal_mod-normal_script.d

CLEANFILES += cmd-normal_mod-normal_script.lst fs-normal_mod-normal_script.lst
COMMANDFILES += cmd-normal_mod-normal_script.lst
FSFILES += fs-normal_mod-normal_script.lst

cmd-normal_mod-normal_script.lst: normal/script.c gencmdlist.sh
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-normal_script.lst: normal/script.c genfslist.sh
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_powerpc_setjmp.o: normal/powerpc/setjmp.S
	$(CC) -Inormal/powerpc -I$(srcdir)/normal/powerpc $(CPPFLAGS) $(ASFLAGS) $(normal_mod_ASFLAGS) -c -o $@ $<

normal_mod-normal_powerpc_setjmp.d: normal/powerpc/setjmp.S
	set -e; 	  $(CC) -Inormal/powerpc -I$(srcdir)/normal/powerpc $(CPPFLAGS) $(ASFLAGS) $(normal_mod_ASFLAGS) -M $< 	  | sed 's,setjmp\.o[ :]*,normal_mod-normal_powerpc_setjmp.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include normal_mod-normal_powerpc_setjmp.d

CLEANFILES += cmd-normal_mod-normal_powerpc_setjmp.lst fs-normal_mod-normal_powerpc_setjmp.lst
COMMANDFILES += cmd-normal_mod-normal_powerpc_setjmp.lst
FSFILES += fs-normal_mod-normal_powerpc_setjmp.lst

cmd-normal_mod-normal_powerpc_setjmp.lst: normal/powerpc/setjmp.S gencmdlist.sh
	set -e; 	  $(CC) -Inormal/powerpc -I$(srcdir)/normal/powerpc $(CPPFLAGS) $(ASFLAGS) $(normal_mod_ASFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-normal_powerpc_setjmp.lst: normal/powerpc/setjmp.S genfslist.sh
	set -e; 	  $(CC) -Inormal/powerpc -I$(srcdir)/normal/powerpc $(CPPFLAGS) $(ASFLAGS) $(normal_mod_ASFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod_CFLAGS = $(COMMON_CFLAGS)
normal_mod_LDFLAGS = $(COMMON_LDFLAGS)
normal_mod_ASFLAGS = $(COMMON_ASFLAGS)

# For suspend.mod
suspend_mod_SOURCES = commands/ieee1275/suspend.c
CLEANFILES += suspend.mod mod-suspend.o mod-suspend.c pre-suspend.o suspend_mod-commands_ieee1275_suspend.o und-suspend.lst
ifneq ($(suspend_mod_EXPORTS),no)
CLEANFILES += def-suspend.lst
DEFSYMFILES += def-suspend.lst
endif
MOSTLYCLEANFILES += suspend_mod-commands_ieee1275_suspend.d
UNDSYMFILES += und-suspend.lst

suspend.mod: pre-suspend.o mod-suspend.o
	-rm -f $@
	$(CC) $(suspend_mod_LDFLAGS) $(LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-suspend.o: suspend_mod-commands_ieee1275_suspend.o
	-rm -f $@
	$(CC) $(suspend_mod_LDFLAGS) $(LDFLAGS) -Wl,-r,-d -o $@ $^

mod-suspend.o: mod-suspend.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(suspend_mod_CFLAGS) -c -o $@ $<

mod-suspend.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'suspend' $< > $@ || (rm -f $@; exit 1)

ifneq ($(suspend_mod_EXPORTS),no)
def-suspend.lst: pre-suspend.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 suspend/' > $@
endif

und-suspend.lst: pre-suspend.o
	echo 'suspend' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

suspend_mod-commands_ieee1275_suspend.o: commands/ieee1275/suspend.c
	$(CC) -Icommands/ieee1275 -I$(srcdir)/commands/ieee1275 $(CPPFLAGS) $(CFLAGS) $(suspend_mod_CFLAGS) -c -o $@ $<

suspend_mod-commands_ieee1275_suspend.d: commands/ieee1275/suspend.c
	set -e; 	  $(CC) -Icommands/ieee1275 -I$(srcdir)/commands/ieee1275 $(CPPFLAGS) $(CFLAGS) $(suspend_mod_CFLAGS) -M $< 	  | sed 's,suspend\.o[ :]*,suspend_mod-commands_ieee1275_suspend.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include suspend_mod-commands_ieee1275_suspend.d

CLEANFILES += cmd-suspend_mod-commands_ieee1275_suspend.lst fs-suspend_mod-commands_ieee1275_suspend.lst
COMMANDFILES += cmd-suspend_mod-commands_ieee1275_suspend.lst
FSFILES += fs-suspend_mod-commands_ieee1275_suspend.lst

cmd-suspend_mod-commands_ieee1275_suspend.lst: commands/ieee1275/suspend.c gencmdlist.sh
	set -e; 	  $(CC) -Icommands/ieee1275 -I$(srcdir)/commands/ieee1275 $(CPPFLAGS) $(CFLAGS) $(suspend_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh suspend > $@ || (rm -f $@; exit 1)

fs-suspend_mod-commands_ieee1275_suspend.lst: commands/ieee1275/suspend.c genfslist.sh
	set -e; 	  $(CC) -Icommands/ieee1275 -I$(srcdir)/commands/ieee1275 $(CPPFLAGS) $(CFLAGS) $(suspend_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh suspend > $@ || (rm -f $@; exit 1)


suspend_mod_CFLAGS = $(COMMON_CFLAGS)
suspend_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For reboot.mod
reboot_mod_SOURCES = commands/ieee1275/reboot.c
CLEANFILES += reboot.mod mod-reboot.o mod-reboot.c pre-reboot.o reboot_mod-commands_ieee1275_reboot.o und-reboot.lst
ifneq ($(reboot_mod_EXPORTS),no)
CLEANFILES += def-reboot.lst
DEFSYMFILES += def-reboot.lst
endif
MOSTLYCLEANFILES += reboot_mod-commands_ieee1275_reboot.d
UNDSYMFILES += und-reboot.lst

reboot.mod: pre-reboot.o mod-reboot.o
	-rm -f $@
	$(CC) $(reboot_mod_LDFLAGS) $(LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-reboot.o: reboot_mod-commands_ieee1275_reboot.o
	-rm -f $@
	$(CC) $(reboot_mod_LDFLAGS) $(LDFLAGS) -Wl,-r,-d -o $@ $^

mod-reboot.o: mod-reboot.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(reboot_mod_CFLAGS) -c -o $@ $<

mod-reboot.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'reboot' $< > $@ || (rm -f $@; exit 1)

ifneq ($(reboot_mod_EXPORTS),no)
def-reboot.lst: pre-reboot.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 reboot/' > $@
endif

und-reboot.lst: pre-reboot.o
	echo 'reboot' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

reboot_mod-commands_ieee1275_reboot.o: commands/ieee1275/reboot.c
	$(CC) -Icommands/ieee1275 -I$(srcdir)/commands/ieee1275 $(CPPFLAGS) $(CFLAGS) $(reboot_mod_CFLAGS) -c -o $@ $<

reboot_mod-commands_ieee1275_reboot.d: commands/ieee1275/reboot.c
	set -e; 	  $(CC) -Icommands/ieee1275 -I$(srcdir)/commands/ieee1275 $(CPPFLAGS) $(CFLAGS) $(reboot_mod_CFLAGS) -M $< 	  | sed 's,reboot\.o[ :]*,reboot_mod-commands_ieee1275_reboot.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include reboot_mod-commands_ieee1275_reboot.d

CLEANFILES += cmd-reboot_mod-commands_ieee1275_reboot.lst fs-reboot_mod-commands_ieee1275_reboot.lst
COMMANDFILES += cmd-reboot_mod-commands_ieee1275_reboot.lst
FSFILES += fs-reboot_mod-commands_ieee1275_reboot.lst

cmd-reboot_mod-commands_ieee1275_reboot.lst: commands/ieee1275/reboot.c gencmdlist.sh
	set -e; 	  $(CC) -Icommands/ieee1275 -I$(srcdir)/commands/ieee1275 $(CPPFLAGS) $(CFLAGS) $(reboot_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh reboot > $@ || (rm -f $@; exit 1)

fs-reboot_mod-commands_ieee1275_reboot.lst: commands/ieee1275/reboot.c genfslist.sh
	set -e; 	  $(CC) -Icommands/ieee1275 -I$(srcdir)/commands/ieee1275 $(CPPFLAGS) $(CFLAGS) $(reboot_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh reboot > $@ || (rm -f $@; exit 1)


reboot_mod_CFLAGS = $(COMMON_CFLAGS)
reboot_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For halt.mod
halt_mod_SOURCES = commands/ieee1275/halt.c
CLEANFILES += halt.mod mod-halt.o mod-halt.c pre-halt.o halt_mod-commands_ieee1275_halt.o und-halt.lst
ifneq ($(halt_mod_EXPORTS),no)
CLEANFILES += def-halt.lst
DEFSYMFILES += def-halt.lst
endif
MOSTLYCLEANFILES += halt_mod-commands_ieee1275_halt.d
UNDSYMFILES += und-halt.lst

halt.mod: pre-halt.o mod-halt.o
	-rm -f $@
	$(CC) $(halt_mod_LDFLAGS) $(LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-halt.o: halt_mod-commands_ieee1275_halt.o
	-rm -f $@
	$(CC) $(halt_mod_LDFLAGS) $(LDFLAGS) -Wl,-r,-d -o $@ $^

mod-halt.o: mod-halt.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(halt_mod_CFLAGS) -c -o $@ $<

mod-halt.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'halt' $< > $@ || (rm -f $@; exit 1)

ifneq ($(halt_mod_EXPORTS),no)
def-halt.lst: pre-halt.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 halt/' > $@
endif

und-halt.lst: pre-halt.o
	echo 'halt' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

halt_mod-commands_ieee1275_halt.o: commands/ieee1275/halt.c
	$(CC) -Icommands/ieee1275 -I$(srcdir)/commands/ieee1275 $(CPPFLAGS) $(CFLAGS) $(halt_mod_CFLAGS) -c -o $@ $<

halt_mod-commands_ieee1275_halt.d: commands/ieee1275/halt.c
	set -e; 	  $(CC) -Icommands/ieee1275 -I$(srcdir)/commands/ieee1275 $(CPPFLAGS) $(CFLAGS) $(halt_mod_CFLAGS) -M $< 	  | sed 's,halt\.o[ :]*,halt_mod-commands_ieee1275_halt.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include halt_mod-commands_ieee1275_halt.d

CLEANFILES += cmd-halt_mod-commands_ieee1275_halt.lst fs-halt_mod-commands_ieee1275_halt.lst
COMMANDFILES += cmd-halt_mod-commands_ieee1275_halt.lst
FSFILES += fs-halt_mod-commands_ieee1275_halt.lst

cmd-halt_mod-commands_ieee1275_halt.lst: commands/ieee1275/halt.c gencmdlist.sh
	set -e; 	  $(CC) -Icommands/ieee1275 -I$(srcdir)/commands/ieee1275 $(CPPFLAGS) $(CFLAGS) $(halt_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh halt > $@ || (rm -f $@; exit 1)

fs-halt_mod-commands_ieee1275_halt.lst: commands/ieee1275/halt.c genfslist.sh
	set -e; 	  $(CC) -Icommands/ieee1275 -I$(srcdir)/commands/ieee1275 $(CPPFLAGS) $(CFLAGS) $(halt_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh halt > $@ || (rm -f $@; exit 1)


halt_mod_CFLAGS = $(COMMON_CFLAGS)
halt_mod_LDFLAGS = $(COMMON_LDFLAGS)

include $(srcdir)/conf/common.mk
