
# -*- makefile -*-

COMMON_ASFLAGS = -nostdinc -fno-builtin -D__ASSEMBLY__
COMMON_CFLAGS = -fno-builtin -D__ASSEMBLY__

# Images.

MOSTLYCLEANFILES += symlist.c kernel_syms.lst
DEFSYMFILES += kernel_syms.lst

symlist.c: $(addprefix include/grub/,$(kernel_img_HEADERS)) gensymlist.sh
	sh $(srcdir)/gensymlist.sh $(filter %.h,$^) > $@

kernel_syms.lst: $(addprefix include/grub/,$(kernel_img_HEADERS)) genkernsyms.sh
	sh $(srcdir)/genkernsyms.sh $(filter %h,$^) > $@

# Utilities.
sbin_UTILITIES = grubof
bin_UTILITIES = grub-emu
noinst_UTILITIES = genmoddep

# For grub-emu
grub_emu_SOURCES = kern/main.c kern/device.c				\
	kern/disk.c kern/dl.c kern/file.c kern/fs.c kern/err.c		\
        kern/misc.c kern/loader.c kern/rescue.c kern/term.c		\
	disk/powerpc/ieee1275/partition.c fs/fshelp.c			\
	util/i386/pc/biosdisk.c fs/fat.c fs/ext2.c fs/ufs.c fs/minix.c fs/hfs.c	\
	fs/jfs.c normal/cmdline.c normal/command.c normal/main.c normal/menu.c	\
	normal/arg.c	\
	util/console.c util/grub-emu.c util/misc.c util/i386/pc/getroot.c \
	kern/env.c commands/ls.c		\
	commands/terminal.c commands/boot.c commands/cmp.c commands/cat.c
CLEANFILES += grub-emu grub_emu-kern_main.o grub_emu-kern_device.o grub_emu-kern_disk.o grub_emu-kern_dl.o grub_emu-kern_file.o grub_emu-kern_fs.o grub_emu-kern_err.o grub_emu-kern_misc.o grub_emu-kern_loader.o grub_emu-kern_rescue.o grub_emu-kern_term.o grub_emu-disk_powerpc_ieee1275_partition.o grub_emu-fs_fshelp.o grub_emu-util_i386_pc_biosdisk.o grub_emu-fs_fat.o grub_emu-fs_ext2.o grub_emu-fs_ufs.o grub_emu-fs_minix.o grub_emu-fs_hfs.o grub_emu-fs_jfs.o grub_emu-normal_cmdline.o grub_emu-normal_command.o grub_emu-normal_main.o grub_emu-normal_menu.o grub_emu-normal_arg.o grub_emu-util_console.o grub_emu-util_grub_emu.o grub_emu-util_misc.o grub_emu-util_i386_pc_getroot.o grub_emu-kern_env.o grub_emu-commands_ls.o grub_emu-commands_terminal.o grub_emu-commands_boot.o grub_emu-commands_cmp.o grub_emu-commands_cat.o
MOSTLYCLEANFILES += grub_emu-kern_main.d grub_emu-kern_device.d grub_emu-kern_disk.d grub_emu-kern_dl.d grub_emu-kern_file.d grub_emu-kern_fs.d grub_emu-kern_err.d grub_emu-kern_misc.d grub_emu-kern_loader.d grub_emu-kern_rescue.d grub_emu-kern_term.d grub_emu-disk_powerpc_ieee1275_partition.d grub_emu-fs_fshelp.d grub_emu-util_i386_pc_biosdisk.d grub_emu-fs_fat.d grub_emu-fs_ext2.d grub_emu-fs_ufs.d grub_emu-fs_minix.d grub_emu-fs_hfs.d grub_emu-fs_jfs.d grub_emu-normal_cmdline.d grub_emu-normal_command.d grub_emu-normal_main.d grub_emu-normal_menu.d grub_emu-normal_arg.d grub_emu-util_console.d grub_emu-util_grub_emu.d grub_emu-util_misc.d grub_emu-util_i386_pc_getroot.d grub_emu-kern_env.d grub_emu-commands_ls.d grub_emu-commands_terminal.d grub_emu-commands_boot.d grub_emu-commands_cmp.d grub_emu-commands_cat.d

grub-emu: grub_emu-kern_main.o grub_emu-kern_device.o grub_emu-kern_disk.o grub_emu-kern_dl.o grub_emu-kern_file.o grub_emu-kern_fs.o grub_emu-kern_err.o grub_emu-kern_misc.o grub_emu-kern_loader.o grub_emu-kern_rescue.o grub_emu-kern_term.o grub_emu-disk_powerpc_ieee1275_partition.o grub_emu-fs_fshelp.o grub_emu-util_i386_pc_biosdisk.o grub_emu-fs_fat.o grub_emu-fs_ext2.o grub_emu-fs_ufs.o grub_emu-fs_minix.o grub_emu-fs_hfs.o grub_emu-fs_jfs.o grub_emu-normal_cmdline.o grub_emu-normal_command.o grub_emu-normal_main.o grub_emu-normal_menu.o grub_emu-normal_arg.o grub_emu-util_console.o grub_emu-util_grub_emu.o grub_emu-util_misc.o grub_emu-util_i386_pc_getroot.o grub_emu-kern_env.o grub_emu-commands_ls.o grub_emu-commands_terminal.o grub_emu-commands_boot.o grub_emu-commands_cmp.o grub_emu-commands_cat.o
	$(BUILD_CC) -o $@ $^ $(BUILD_LDFLAGS) $(grub_emu_LDFLAGS)

grub_emu-kern_main.o: kern/main.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-kern_main.d: kern/main.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,main\.o[ :]*,grub_emu-kern_main.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-kern_main.d

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

grub_emu-kern_err.o: kern/err.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-kern_err.d: kern/err.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,err\.o[ :]*,grub_emu-kern_err.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-kern_err.d

grub_emu-kern_misc.o: kern/misc.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-kern_misc.d: kern/misc.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,misc\.o[ :]*,grub_emu-kern_misc.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-kern_misc.d

grub_emu-kern_loader.o: kern/loader.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-kern_loader.d: kern/loader.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,loader\.o[ :]*,grub_emu-kern_loader.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-kern_loader.d

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

grub_emu-disk_powerpc_ieee1275_partition.o: disk/powerpc/ieee1275/partition.c
	$(BUILD_CC) -Idisk/powerpc/ieee1275 -I$(srcdir)/disk/powerpc/ieee1275 $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-disk_powerpc_ieee1275_partition.d: disk/powerpc/ieee1275/partition.c
	set -e; 	  $(BUILD_CC) -Idisk/powerpc/ieee1275 -I$(srcdir)/disk/powerpc/ieee1275 $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,partition\.o[ :]*,grub_emu-disk_powerpc_ieee1275_partition.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-disk_powerpc_ieee1275_partition.d

grub_emu-fs_fshelp.o: fs/fshelp.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-fs_fshelp.d: fs/fshelp.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,fshelp\.o[ :]*,grub_emu-fs_fshelp.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-fs_fshelp.d

grub_emu-util_i386_pc_biosdisk.o: util/i386/pc/biosdisk.c
	$(BUILD_CC) -Iutil/i386/pc -I$(srcdir)/util/i386/pc $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-util_i386_pc_biosdisk.d: util/i386/pc/biosdisk.c
	set -e; 	  $(BUILD_CC) -Iutil/i386/pc -I$(srcdir)/util/i386/pc $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,biosdisk\.o[ :]*,grub_emu-util_i386_pc_biosdisk.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-util_i386_pc_biosdisk.d

grub_emu-fs_fat.o: fs/fat.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-fs_fat.d: fs/fat.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,fat\.o[ :]*,grub_emu-fs_fat.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-fs_fat.d

grub_emu-fs_ext2.o: fs/ext2.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-fs_ext2.d: fs/ext2.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,ext2\.o[ :]*,grub_emu-fs_ext2.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-fs_ext2.d

grub_emu-fs_ufs.o: fs/ufs.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-fs_ufs.d: fs/ufs.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,ufs\.o[ :]*,grub_emu-fs_ufs.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-fs_ufs.d

grub_emu-fs_minix.o: fs/minix.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-fs_minix.d: fs/minix.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,minix\.o[ :]*,grub_emu-fs_minix.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-fs_minix.d

grub_emu-fs_hfs.o: fs/hfs.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-fs_hfs.d: fs/hfs.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,hfs\.o[ :]*,grub_emu-fs_hfs.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-fs_hfs.d

grub_emu-fs_jfs.o: fs/jfs.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-fs_jfs.d: fs/jfs.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,jfs\.o[ :]*,grub_emu-fs_jfs.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-fs_jfs.d

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

grub_emu-normal_arg.o: normal/arg.c
	$(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-normal_arg.d: normal/arg.c
	set -e; 	  $(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,arg\.o[ :]*,grub_emu-normal_arg.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-normal_arg.d

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

grub_emu-util_i386_pc_getroot.o: util/i386/pc/getroot.c
	$(BUILD_CC) -Iutil/i386/pc -I$(srcdir)/util/i386/pc $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-util_i386_pc_getroot.d: util/i386/pc/getroot.c
	set -e; 	  $(BUILD_CC) -Iutil/i386/pc -I$(srcdir)/util/i386/pc $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,getroot\.o[ :]*,grub_emu-util_i386_pc_getroot.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-util_i386_pc_getroot.d

grub_emu-kern_env.o: kern/env.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-kern_env.d: kern/env.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,env\.o[ :]*,grub_emu-kern_env.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-kern_env.d

grub_emu-commands_ls.o: commands/ls.c
	$(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-commands_ls.d: commands/ls.c
	set -e; 	  $(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,ls\.o[ :]*,grub_emu-commands_ls.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-commands_ls.d

grub_emu-commands_terminal.o: commands/terminal.c
	$(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-commands_terminal.d: commands/terminal.c
	set -e; 	  $(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,terminal\.o[ :]*,grub_emu-commands_terminal.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-commands_terminal.d

grub_emu-commands_boot.o: commands/boot.c
	$(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-commands_boot.d: commands/boot.c
	set -e; 	  $(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,boot\.o[ :]*,grub_emu-commands_boot.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-commands_boot.d

grub_emu-commands_cmp.o: commands/cmp.c
	$(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-commands_cmp.d: commands/cmp.c
	set -e; 	  $(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,cmp\.o[ :]*,grub_emu-commands_cmp.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-commands_cmp.d

grub_emu-commands_cat.o: commands/cat.c
	$(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-commands_cat.d: commands/cat.c
	set -e; 	  $(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,cat\.o[ :]*,grub_emu-commands_cat.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-commands_cat.d

grub_emu_LDFLAGS = -lncurses

grubof_SOURCES = boot/powerpc/ieee1275/cmain.c boot/powerpc/ieee1275/ieee1275.c \
	boot/powerpc/ieee1275/crt0.S kern/main.c kern/device.c \
	kern/disk.c kern/dl.c kern/file.c kern/fs.c kern/err.c fs/fshelp.c \
	kern/misc.c kern/mm.c kern/loader.c kern/rescue.c kern/term.c \
	kern/powerpc/ieee1275/init.c term/powerpc/ieee1275/ofconsole.c \
	kern/powerpc/ieee1275/openfw.c fs/ext2.c fs/ufs.c fs/minix.c fs/hfs.c \
	fs/jfs.c normal/cmdline.c normal/command.c normal/main.c normal/menu.c \
	disk/powerpc/ieee1275/ofdisk.c disk/powerpc/ieee1275/partition.c \
	kern/env.c normal/arg.c loader/powerpc/ieee1275/linux.c \
	loader/powerpc/ieee1275/linux_normal.c commands/boot.c
CLEANFILES += grubof grubof-boot_powerpc_ieee1275_cmain.o grubof-boot_powerpc_ieee1275_ieee1275.o grubof-boot_powerpc_ieee1275_crt0.o grubof-kern_main.o grubof-kern_device.o grubof-kern_disk.o grubof-kern_dl.o grubof-kern_file.o grubof-kern_fs.o grubof-kern_err.o grubof-fs_fshelp.o grubof-kern_misc.o grubof-kern_mm.o grubof-kern_loader.o grubof-kern_rescue.o grubof-kern_term.o grubof-kern_powerpc_ieee1275_init.o grubof-term_powerpc_ieee1275_ofconsole.o grubof-kern_powerpc_ieee1275_openfw.o grubof-fs_ext2.o grubof-fs_ufs.o grubof-fs_minix.o grubof-fs_hfs.o grubof-fs_jfs.o grubof-normal_cmdline.o grubof-normal_command.o grubof-normal_main.o grubof-normal_menu.o grubof-disk_powerpc_ieee1275_ofdisk.o grubof-disk_powerpc_ieee1275_partition.o grubof-kern_env.o grubof-normal_arg.o grubof-loader_powerpc_ieee1275_linux.o grubof-loader_powerpc_ieee1275_linux_normal.o grubof-commands_boot.o
MOSTLYCLEANFILES += grubof-boot_powerpc_ieee1275_cmain.d grubof-boot_powerpc_ieee1275_ieee1275.d grubof-boot_powerpc_ieee1275_crt0.d grubof-kern_main.d grubof-kern_device.d grubof-kern_disk.d grubof-kern_dl.d grubof-kern_file.d grubof-kern_fs.d grubof-kern_err.d grubof-fs_fshelp.d grubof-kern_misc.d grubof-kern_mm.d grubof-kern_loader.d grubof-kern_rescue.d grubof-kern_term.d grubof-kern_powerpc_ieee1275_init.d grubof-term_powerpc_ieee1275_ofconsole.d grubof-kern_powerpc_ieee1275_openfw.d grubof-fs_ext2.d grubof-fs_ufs.d grubof-fs_minix.d grubof-fs_hfs.d grubof-fs_jfs.d grubof-normal_cmdline.d grubof-normal_command.d grubof-normal_main.d grubof-normal_menu.d grubof-disk_powerpc_ieee1275_ofdisk.d grubof-disk_powerpc_ieee1275_partition.d grubof-kern_env.d grubof-normal_arg.d grubof-loader_powerpc_ieee1275_linux.d grubof-loader_powerpc_ieee1275_linux_normal.d grubof-commands_boot.d

grubof: grubof-boot_powerpc_ieee1275_cmain.o grubof-boot_powerpc_ieee1275_ieee1275.o grubof-boot_powerpc_ieee1275_crt0.o grubof-kern_main.o grubof-kern_device.o grubof-kern_disk.o grubof-kern_dl.o grubof-kern_file.o grubof-kern_fs.o grubof-kern_err.o grubof-fs_fshelp.o grubof-kern_misc.o grubof-kern_mm.o grubof-kern_loader.o grubof-kern_rescue.o grubof-kern_term.o grubof-kern_powerpc_ieee1275_init.o grubof-term_powerpc_ieee1275_ofconsole.o grubof-kern_powerpc_ieee1275_openfw.o grubof-fs_ext2.o grubof-fs_ufs.o grubof-fs_minix.o grubof-fs_hfs.o grubof-fs_jfs.o grubof-normal_cmdline.o grubof-normal_command.o grubof-normal_main.o grubof-normal_menu.o grubof-disk_powerpc_ieee1275_ofdisk.o grubof-disk_powerpc_ieee1275_partition.o grubof-kern_env.o grubof-normal_arg.o grubof-loader_powerpc_ieee1275_linux.o grubof-loader_powerpc_ieee1275_linux_normal.o grubof-commands_boot.o
	$(BUILD_CC) -o $@ $^ $(BUILD_LDFLAGS) $(grubof_LDFLAGS)

grubof-boot_powerpc_ieee1275_cmain.o: boot/powerpc/ieee1275/cmain.c
	$(BUILD_CC) -Iboot/powerpc/ieee1275 -I$(srcdir)/boot/powerpc/ieee1275 $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -c -o $@ $<

grubof-boot_powerpc_ieee1275_cmain.d: boot/powerpc/ieee1275/cmain.c
	set -e; 	  $(BUILD_CC) -Iboot/powerpc/ieee1275 -I$(srcdir)/boot/powerpc/ieee1275 $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -M $< 	  | sed 's,cmain\.o[ :]*,grubof-boot_powerpc_ieee1275_cmain.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-boot_powerpc_ieee1275_cmain.d

grubof-boot_powerpc_ieee1275_ieee1275.o: boot/powerpc/ieee1275/ieee1275.c
	$(BUILD_CC) -Iboot/powerpc/ieee1275 -I$(srcdir)/boot/powerpc/ieee1275 $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -c -o $@ $<

grubof-boot_powerpc_ieee1275_ieee1275.d: boot/powerpc/ieee1275/ieee1275.c
	set -e; 	  $(BUILD_CC) -Iboot/powerpc/ieee1275 -I$(srcdir)/boot/powerpc/ieee1275 $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -M $< 	  | sed 's,ieee1275\.o[ :]*,grubof-boot_powerpc_ieee1275_ieee1275.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-boot_powerpc_ieee1275_ieee1275.d

grubof-boot_powerpc_ieee1275_crt0.o: boot/powerpc/ieee1275/crt0.S
	$(BUILD_CC) -Iboot/powerpc/ieee1275 -I$(srcdir)/boot/powerpc/ieee1275 $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -c -o $@ $<

grubof-boot_powerpc_ieee1275_crt0.d: boot/powerpc/ieee1275/crt0.S
	set -e; 	  $(BUILD_CC) -Iboot/powerpc/ieee1275 -I$(srcdir)/boot/powerpc/ieee1275 $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -M $< 	  | sed 's,crt0\.o[ :]*,grubof-boot_powerpc_ieee1275_crt0.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-boot_powerpc_ieee1275_crt0.d

grubof-kern_main.o: kern/main.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_main.d: kern/main.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -M $< 	  | sed 's,main\.o[ :]*,grubof-kern_main.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_main.d

grubof-kern_device.o: kern/device.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_device.d: kern/device.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -M $< 	  | sed 's,device\.o[ :]*,grubof-kern_device.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_device.d

grubof-kern_disk.o: kern/disk.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_disk.d: kern/disk.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -M $< 	  | sed 's,disk\.o[ :]*,grubof-kern_disk.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_disk.d

grubof-kern_dl.o: kern/dl.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_dl.d: kern/dl.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -M $< 	  | sed 's,dl\.o[ :]*,grubof-kern_dl.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_dl.d

grubof-kern_file.o: kern/file.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_file.d: kern/file.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -M $< 	  | sed 's,file\.o[ :]*,grubof-kern_file.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_file.d

grubof-kern_fs.o: kern/fs.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_fs.d: kern/fs.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -M $< 	  | sed 's,fs\.o[ :]*,grubof-kern_fs.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_fs.d

grubof-kern_err.o: kern/err.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_err.d: kern/err.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -M $< 	  | sed 's,err\.o[ :]*,grubof-kern_err.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_err.d

grubof-fs_fshelp.o: fs/fshelp.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -c -o $@ $<

grubof-fs_fshelp.d: fs/fshelp.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -M $< 	  | sed 's,fshelp\.o[ :]*,grubof-fs_fshelp.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-fs_fshelp.d

grubof-kern_misc.o: kern/misc.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_misc.d: kern/misc.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -M $< 	  | sed 's,misc\.o[ :]*,grubof-kern_misc.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_misc.d

grubof-kern_mm.o: kern/mm.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_mm.d: kern/mm.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -M $< 	  | sed 's,mm\.o[ :]*,grubof-kern_mm.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_mm.d

grubof-kern_loader.o: kern/loader.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_loader.d: kern/loader.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -M $< 	  | sed 's,loader\.o[ :]*,grubof-kern_loader.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_loader.d

grubof-kern_rescue.o: kern/rescue.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_rescue.d: kern/rescue.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -M $< 	  | sed 's,rescue\.o[ :]*,grubof-kern_rescue.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_rescue.d

grubof-kern_term.o: kern/term.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_term.d: kern/term.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -M $< 	  | sed 's,term\.o[ :]*,grubof-kern_term.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_term.d

grubof-kern_powerpc_ieee1275_init.o: kern/powerpc/ieee1275/init.c
	$(BUILD_CC) -Ikern/powerpc/ieee1275 -I$(srcdir)/kern/powerpc/ieee1275 $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_powerpc_ieee1275_init.d: kern/powerpc/ieee1275/init.c
	set -e; 	  $(BUILD_CC) -Ikern/powerpc/ieee1275 -I$(srcdir)/kern/powerpc/ieee1275 $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -M $< 	  | sed 's,init\.o[ :]*,grubof-kern_powerpc_ieee1275_init.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_powerpc_ieee1275_init.d

grubof-term_powerpc_ieee1275_ofconsole.o: term/powerpc/ieee1275/ofconsole.c
	$(BUILD_CC) -Iterm/powerpc/ieee1275 -I$(srcdir)/term/powerpc/ieee1275 $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -c -o $@ $<

grubof-term_powerpc_ieee1275_ofconsole.d: term/powerpc/ieee1275/ofconsole.c
	set -e; 	  $(BUILD_CC) -Iterm/powerpc/ieee1275 -I$(srcdir)/term/powerpc/ieee1275 $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -M $< 	  | sed 's,ofconsole\.o[ :]*,grubof-term_powerpc_ieee1275_ofconsole.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-term_powerpc_ieee1275_ofconsole.d

grubof-kern_powerpc_ieee1275_openfw.o: kern/powerpc/ieee1275/openfw.c
	$(BUILD_CC) -Ikern/powerpc/ieee1275 -I$(srcdir)/kern/powerpc/ieee1275 $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_powerpc_ieee1275_openfw.d: kern/powerpc/ieee1275/openfw.c
	set -e; 	  $(BUILD_CC) -Ikern/powerpc/ieee1275 -I$(srcdir)/kern/powerpc/ieee1275 $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -M $< 	  | sed 's,openfw\.o[ :]*,grubof-kern_powerpc_ieee1275_openfw.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_powerpc_ieee1275_openfw.d

grubof-fs_ext2.o: fs/ext2.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -c -o $@ $<

grubof-fs_ext2.d: fs/ext2.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -M $< 	  | sed 's,ext2\.o[ :]*,grubof-fs_ext2.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-fs_ext2.d

grubof-fs_ufs.o: fs/ufs.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -c -o $@ $<

grubof-fs_ufs.d: fs/ufs.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -M $< 	  | sed 's,ufs\.o[ :]*,grubof-fs_ufs.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-fs_ufs.d

grubof-fs_minix.o: fs/minix.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -c -o $@ $<

grubof-fs_minix.d: fs/minix.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -M $< 	  | sed 's,minix\.o[ :]*,grubof-fs_minix.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-fs_minix.d

grubof-fs_hfs.o: fs/hfs.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -c -o $@ $<

grubof-fs_hfs.d: fs/hfs.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -M $< 	  | sed 's,hfs\.o[ :]*,grubof-fs_hfs.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-fs_hfs.d

grubof-fs_jfs.o: fs/jfs.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -c -o $@ $<

grubof-fs_jfs.d: fs/jfs.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -M $< 	  | sed 's,jfs\.o[ :]*,grubof-fs_jfs.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-fs_jfs.d

grubof-normal_cmdline.o: normal/cmdline.c
	$(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -c -o $@ $<

grubof-normal_cmdline.d: normal/cmdline.c
	set -e; 	  $(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -M $< 	  | sed 's,cmdline\.o[ :]*,grubof-normal_cmdline.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-normal_cmdline.d

grubof-normal_command.o: normal/command.c
	$(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -c -o $@ $<

grubof-normal_command.d: normal/command.c
	set -e; 	  $(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -M $< 	  | sed 's,command\.o[ :]*,grubof-normal_command.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-normal_command.d

grubof-normal_main.o: normal/main.c
	$(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -c -o $@ $<

grubof-normal_main.d: normal/main.c
	set -e; 	  $(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -M $< 	  | sed 's,main\.o[ :]*,grubof-normal_main.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-normal_main.d

grubof-normal_menu.o: normal/menu.c
	$(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -c -o $@ $<

grubof-normal_menu.d: normal/menu.c
	set -e; 	  $(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -M $< 	  | sed 's,menu\.o[ :]*,grubof-normal_menu.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-normal_menu.d

grubof-disk_powerpc_ieee1275_ofdisk.o: disk/powerpc/ieee1275/ofdisk.c
	$(BUILD_CC) -Idisk/powerpc/ieee1275 -I$(srcdir)/disk/powerpc/ieee1275 $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -c -o $@ $<

grubof-disk_powerpc_ieee1275_ofdisk.d: disk/powerpc/ieee1275/ofdisk.c
	set -e; 	  $(BUILD_CC) -Idisk/powerpc/ieee1275 -I$(srcdir)/disk/powerpc/ieee1275 $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -M $< 	  | sed 's,ofdisk\.o[ :]*,grubof-disk_powerpc_ieee1275_ofdisk.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-disk_powerpc_ieee1275_ofdisk.d

grubof-disk_powerpc_ieee1275_partition.o: disk/powerpc/ieee1275/partition.c
	$(BUILD_CC) -Idisk/powerpc/ieee1275 -I$(srcdir)/disk/powerpc/ieee1275 $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -c -o $@ $<

grubof-disk_powerpc_ieee1275_partition.d: disk/powerpc/ieee1275/partition.c
	set -e; 	  $(BUILD_CC) -Idisk/powerpc/ieee1275 -I$(srcdir)/disk/powerpc/ieee1275 $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -M $< 	  | sed 's,partition\.o[ :]*,grubof-disk_powerpc_ieee1275_partition.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-disk_powerpc_ieee1275_partition.d

grubof-kern_env.o: kern/env.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_env.d: kern/env.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -M $< 	  | sed 's,env\.o[ :]*,grubof-kern_env.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_env.d

grubof-normal_arg.o: normal/arg.c
	$(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -c -o $@ $<

grubof-normal_arg.d: normal/arg.c
	set -e; 	  $(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -M $< 	  | sed 's,arg\.o[ :]*,grubof-normal_arg.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-normal_arg.d

grubof-loader_powerpc_ieee1275_linux.o: loader/powerpc/ieee1275/linux.c
	$(BUILD_CC) -Iloader/powerpc/ieee1275 -I$(srcdir)/loader/powerpc/ieee1275 $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -c -o $@ $<

grubof-loader_powerpc_ieee1275_linux.d: loader/powerpc/ieee1275/linux.c
	set -e; 	  $(BUILD_CC) -Iloader/powerpc/ieee1275 -I$(srcdir)/loader/powerpc/ieee1275 $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -M $< 	  | sed 's,linux\.o[ :]*,grubof-loader_powerpc_ieee1275_linux.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-loader_powerpc_ieee1275_linux.d

grubof-loader_powerpc_ieee1275_linux_normal.o: loader/powerpc/ieee1275/linux_normal.c
	$(BUILD_CC) -Iloader/powerpc/ieee1275 -I$(srcdir)/loader/powerpc/ieee1275 $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -c -o $@ $<

grubof-loader_powerpc_ieee1275_linux_normal.d: loader/powerpc/ieee1275/linux_normal.c
	set -e; 	  $(BUILD_CC) -Iloader/powerpc/ieee1275 -I$(srcdir)/loader/powerpc/ieee1275 $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -M $< 	  | sed 's,linux_normal\.o[ :]*,grubof-loader_powerpc_ieee1275_linux_normal.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-loader_powerpc_ieee1275_linux_normal.d

grubof-commands_boot.o: commands/boot.c
	$(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -c -o $@ $<

grubof-commands_boot.d: commands/boot.c
	set -e; 	  $(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grubof_CFLAGS) -M $< 	  | sed 's,boot\.o[ :]*,grubof-commands_boot.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-commands_boot.d

grubof_HEADERS = grub/powerpc/ieee1275/ieee1275.h
grubof_CFLAGS = $(COMMON_CFLAGS)
grubof_ASFLAGS = $(COMMON_ASFLAGS)
grubof_LDFLAGS = -nostdlib -static-libgcc -lgcc -Wl,-N,-S,-Ttext,0x200000,-Bstatic

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


# Modules.
CLEANFILES += moddep.lst
pkgdata_DATA += moddep.lst
moddep.lst: $(DEFSYMFILES) $(UNDSYMFILES) genmoddep
	cat $(DEFSYMFILES) /dev/null | ./genmoddep $(UNDSYMFILES) > $@ \
	  || (rm -f $@; exit 1)
