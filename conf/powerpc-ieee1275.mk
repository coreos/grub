
# -*- makefile -*-

COMMON_ASFLAGS = -nostdinc -fno-builtin -D__ASSEMBLY__
COMMON_CFLAGS = -fno-builtin -D__ASSEMBLY__

# Images.

MOSTLYCLEANFILES += symlist.c kernel_syms.lst
DEFSYMFILES += kernel_syms.lst

symlist.c: $(addprefix include/pupa/,$(kernel_img_HEADERS)) gensymlist.sh
	sh $(srcdir)/gensymlist.sh $(filter %.h,$^) > $@

kernel_syms.lst: $(addprefix include/pupa/,$(kernel_img_HEADERS)) genkernsyms.sh
	sh $(srcdir)/genkernsyms.sh $(filter %h,$^) > $@

# Utilities.
sbin_UTILITIES = pupaof
bin_UTILITIES = pupa-emu
noinst_UTILITIES = genmoddep

# For pupa-emu
pupa_emu_SOURCES = kern/main.c kern/device.c				\
	kern/disk.c kern/dl.c kern/file.c kern/fs.c kern/err.c		\
        kern/misc.c kern/loader.c kern/rescue.c kern/term.c		\
	disk/powerpc/ieee1275/partition.c 					\
	util/i386/pc/biosdisk.c fs/fat.c fs/ext2.c			\
	normal/cmdline.c normal/command.c normal/main.c normal/menu.c	\
	util/console.c util/pupa-emu.c util/misc.c util/i386/pc/getroot.c \
	kern/env.c 
CLEANFILES += pupa-emu pupa_emu-kern_main.o pupa_emu-kern_device.o pupa_emu-kern_disk.o pupa_emu-kern_dl.o pupa_emu-kern_file.o pupa_emu-kern_fs.o pupa_emu-kern_err.o pupa_emu-kern_misc.o pupa_emu-kern_loader.o pupa_emu-kern_rescue.o pupa_emu-kern_term.o pupa_emu-disk_powerpc_ieee1275_partition.o pupa_emu-util_i386_pc_biosdisk.o pupa_emu-fs_fat.o pupa_emu-fs_ext2.o pupa_emu-normal_cmdline.o pupa_emu-normal_command.o pupa_emu-normal_main.o pupa_emu-normal_menu.o pupa_emu-util_console.o pupa_emu-util_pupa_emu.o pupa_emu-util_misc.o pupa_emu-util_i386_pc_getroot.o pupa_emu-kern_env.o
MOSTLYCLEANFILES += pupa_emu-kern_main.d pupa_emu-kern_device.d pupa_emu-kern_disk.d pupa_emu-kern_dl.d pupa_emu-kern_file.d pupa_emu-kern_fs.d pupa_emu-kern_err.d pupa_emu-kern_misc.d pupa_emu-kern_loader.d pupa_emu-kern_rescue.d pupa_emu-kern_term.d pupa_emu-disk_powerpc_ieee1275_partition.d pupa_emu-util_i386_pc_biosdisk.d pupa_emu-fs_fat.d pupa_emu-fs_ext2.d pupa_emu-normal_cmdline.d pupa_emu-normal_command.d pupa_emu-normal_main.d pupa_emu-normal_menu.d pupa_emu-util_console.d pupa_emu-util_pupa_emu.d pupa_emu-util_misc.d pupa_emu-util_i386_pc_getroot.d pupa_emu-kern_env.d

pupa-emu: pupa_emu-kern_main.o pupa_emu-kern_device.o pupa_emu-kern_disk.o pupa_emu-kern_dl.o pupa_emu-kern_file.o pupa_emu-kern_fs.o pupa_emu-kern_err.o pupa_emu-kern_misc.o pupa_emu-kern_loader.o pupa_emu-kern_rescue.o pupa_emu-kern_term.o pupa_emu-disk_powerpc_ieee1275_partition.o pupa_emu-util_i386_pc_biosdisk.o pupa_emu-fs_fat.o pupa_emu-fs_ext2.o pupa_emu-normal_cmdline.o pupa_emu-normal_command.o pupa_emu-normal_main.o pupa_emu-normal_menu.o pupa_emu-util_console.o pupa_emu-util_pupa_emu.o pupa_emu-util_misc.o pupa_emu-util_i386_pc_getroot.o pupa_emu-kern_env.o
	$(BUILD_CC) -o $@ $^ $(BUILD_LDFLAGS) $(pupa_emu_LDFLAGS)

pupa_emu-kern_main.o: kern/main.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_emu_CFLAGS) -c -o $@ $<

pupa_emu-kern_main.d: kern/main.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_emu_CFLAGS) -M $< 	  | sed 's,main\.o[ :]*,pupa_emu-kern_main.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupa_emu-kern_main.d

pupa_emu-kern_device.o: kern/device.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_emu_CFLAGS) -c -o $@ $<

pupa_emu-kern_device.d: kern/device.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_emu_CFLAGS) -M $< 	  | sed 's,device\.o[ :]*,pupa_emu-kern_device.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupa_emu-kern_device.d

pupa_emu-kern_disk.o: kern/disk.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_emu_CFLAGS) -c -o $@ $<

pupa_emu-kern_disk.d: kern/disk.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_emu_CFLAGS) -M $< 	  | sed 's,disk\.o[ :]*,pupa_emu-kern_disk.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupa_emu-kern_disk.d

pupa_emu-kern_dl.o: kern/dl.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_emu_CFLAGS) -c -o $@ $<

pupa_emu-kern_dl.d: kern/dl.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_emu_CFLAGS) -M $< 	  | sed 's,dl\.o[ :]*,pupa_emu-kern_dl.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupa_emu-kern_dl.d

pupa_emu-kern_file.o: kern/file.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_emu_CFLAGS) -c -o $@ $<

pupa_emu-kern_file.d: kern/file.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_emu_CFLAGS) -M $< 	  | sed 's,file\.o[ :]*,pupa_emu-kern_file.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupa_emu-kern_file.d

pupa_emu-kern_fs.o: kern/fs.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_emu_CFLAGS) -c -o $@ $<

pupa_emu-kern_fs.d: kern/fs.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_emu_CFLAGS) -M $< 	  | sed 's,fs\.o[ :]*,pupa_emu-kern_fs.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupa_emu-kern_fs.d

pupa_emu-kern_err.o: kern/err.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_emu_CFLAGS) -c -o $@ $<

pupa_emu-kern_err.d: kern/err.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_emu_CFLAGS) -M $< 	  | sed 's,err\.o[ :]*,pupa_emu-kern_err.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupa_emu-kern_err.d

pupa_emu-kern_misc.o: kern/misc.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_emu_CFLAGS) -c -o $@ $<

pupa_emu-kern_misc.d: kern/misc.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_emu_CFLAGS) -M $< 	  | sed 's,misc\.o[ :]*,pupa_emu-kern_misc.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupa_emu-kern_misc.d

pupa_emu-kern_loader.o: kern/loader.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_emu_CFLAGS) -c -o $@ $<

pupa_emu-kern_loader.d: kern/loader.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_emu_CFLAGS) -M $< 	  | sed 's,loader\.o[ :]*,pupa_emu-kern_loader.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupa_emu-kern_loader.d

pupa_emu-kern_rescue.o: kern/rescue.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_emu_CFLAGS) -c -o $@ $<

pupa_emu-kern_rescue.d: kern/rescue.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_emu_CFLAGS) -M $< 	  | sed 's,rescue\.o[ :]*,pupa_emu-kern_rescue.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupa_emu-kern_rescue.d

pupa_emu-kern_term.o: kern/term.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_emu_CFLAGS) -c -o $@ $<

pupa_emu-kern_term.d: kern/term.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_emu_CFLAGS) -M $< 	  | sed 's,term\.o[ :]*,pupa_emu-kern_term.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupa_emu-kern_term.d

pupa_emu-disk_powerpc_ieee1275_partition.o: disk/powerpc/ieee1275/partition.c
	$(BUILD_CC) -Idisk/powerpc/ieee1275 -I$(srcdir)/disk/powerpc/ieee1275 $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_emu_CFLAGS) -c -o $@ $<

pupa_emu-disk_powerpc_ieee1275_partition.d: disk/powerpc/ieee1275/partition.c
	set -e; 	  $(BUILD_CC) -Idisk/powerpc/ieee1275 -I$(srcdir)/disk/powerpc/ieee1275 $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_emu_CFLAGS) -M $< 	  | sed 's,partition\.o[ :]*,pupa_emu-disk_powerpc_ieee1275_partition.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupa_emu-disk_powerpc_ieee1275_partition.d

pupa_emu-util_i386_pc_biosdisk.o: util/i386/pc/biosdisk.c
	$(BUILD_CC) -Iutil/i386/pc -I$(srcdir)/util/i386/pc $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_emu_CFLAGS) -c -o $@ $<

pupa_emu-util_i386_pc_biosdisk.d: util/i386/pc/biosdisk.c
	set -e; 	  $(BUILD_CC) -Iutil/i386/pc -I$(srcdir)/util/i386/pc $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_emu_CFLAGS) -M $< 	  | sed 's,biosdisk\.o[ :]*,pupa_emu-util_i386_pc_biosdisk.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupa_emu-util_i386_pc_biosdisk.d

pupa_emu-fs_fat.o: fs/fat.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_emu_CFLAGS) -c -o $@ $<

pupa_emu-fs_fat.d: fs/fat.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_emu_CFLAGS) -M $< 	  | sed 's,fat\.o[ :]*,pupa_emu-fs_fat.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupa_emu-fs_fat.d

pupa_emu-fs_ext2.o: fs/ext2.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_emu_CFLAGS) -c -o $@ $<

pupa_emu-fs_ext2.d: fs/ext2.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_emu_CFLAGS) -M $< 	  | sed 's,ext2\.o[ :]*,pupa_emu-fs_ext2.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupa_emu-fs_ext2.d

pupa_emu-normal_cmdline.o: normal/cmdline.c
	$(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_emu_CFLAGS) -c -o $@ $<

pupa_emu-normal_cmdline.d: normal/cmdline.c
	set -e; 	  $(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_emu_CFLAGS) -M $< 	  | sed 's,cmdline\.o[ :]*,pupa_emu-normal_cmdline.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupa_emu-normal_cmdline.d

pupa_emu-normal_command.o: normal/command.c
	$(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_emu_CFLAGS) -c -o $@ $<

pupa_emu-normal_command.d: normal/command.c
	set -e; 	  $(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_emu_CFLAGS) -M $< 	  | sed 's,command\.o[ :]*,pupa_emu-normal_command.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupa_emu-normal_command.d

pupa_emu-normal_main.o: normal/main.c
	$(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_emu_CFLAGS) -c -o $@ $<

pupa_emu-normal_main.d: normal/main.c
	set -e; 	  $(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_emu_CFLAGS) -M $< 	  | sed 's,main\.o[ :]*,pupa_emu-normal_main.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupa_emu-normal_main.d

pupa_emu-normal_menu.o: normal/menu.c
	$(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_emu_CFLAGS) -c -o $@ $<

pupa_emu-normal_menu.d: normal/menu.c
	set -e; 	  $(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_emu_CFLAGS) -M $< 	  | sed 's,menu\.o[ :]*,pupa_emu-normal_menu.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupa_emu-normal_menu.d

pupa_emu-util_console.o: util/console.c
	$(BUILD_CC) -Iutil -I$(srcdir)/util $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_emu_CFLAGS) -c -o $@ $<

pupa_emu-util_console.d: util/console.c
	set -e; 	  $(BUILD_CC) -Iutil -I$(srcdir)/util $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_emu_CFLAGS) -M $< 	  | sed 's,console\.o[ :]*,pupa_emu-util_console.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupa_emu-util_console.d

pupa_emu-util_pupa_emu.o: util/pupa-emu.c
	$(BUILD_CC) -Iutil -I$(srcdir)/util $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_emu_CFLAGS) -c -o $@ $<

pupa_emu-util_pupa_emu.d: util/pupa-emu.c
	set -e; 	  $(BUILD_CC) -Iutil -I$(srcdir)/util $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_emu_CFLAGS) -M $< 	  | sed 's,pupa\-emu\.o[ :]*,pupa_emu-util_pupa_emu.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupa_emu-util_pupa_emu.d

pupa_emu-util_misc.o: util/misc.c
	$(BUILD_CC) -Iutil -I$(srcdir)/util $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_emu_CFLAGS) -c -o $@ $<

pupa_emu-util_misc.d: util/misc.c
	set -e; 	  $(BUILD_CC) -Iutil -I$(srcdir)/util $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_emu_CFLAGS) -M $< 	  | sed 's,misc\.o[ :]*,pupa_emu-util_misc.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupa_emu-util_misc.d

pupa_emu-util_i386_pc_getroot.o: util/i386/pc/getroot.c
	$(BUILD_CC) -Iutil/i386/pc -I$(srcdir)/util/i386/pc $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_emu_CFLAGS) -c -o $@ $<

pupa_emu-util_i386_pc_getroot.d: util/i386/pc/getroot.c
	set -e; 	  $(BUILD_CC) -Iutil/i386/pc -I$(srcdir)/util/i386/pc $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_emu_CFLAGS) -M $< 	  | sed 's,getroot\.o[ :]*,pupa_emu-util_i386_pc_getroot.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupa_emu-util_i386_pc_getroot.d

pupa_emu-kern_env.o: kern/env.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_emu_CFLAGS) -c -o $@ $<

pupa_emu-kern_env.d: kern/env.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_emu_CFLAGS) -M $< 	  | sed 's,env\.o[ :]*,pupa_emu-kern_env.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupa_emu-kern_env.d

pupa_emu_LDFLAGS = -lncurses

pupaof_SOURCES = boot/powerpc/ieee1275/cmain.c boot/powerpc/ieee1275/ieee1275.c \
	boot/powerpc/ieee1275/crt0.S kern/main.c kern/device.c \
	kern/disk.c kern/dl.c kern/file.c kern/fs.c kern/err.c \
	kern/misc.c kern/mm.c kern/loader.c kern/rescue.c kern/term.c \
	kern/powerpc/ieee1275/init.c term/powerpc/ieee1275/ofconsole.c \
	kern/powerpc/ieee1275/openfw.c fs/ext2.c normal/cmdline.c \
	normal/command.c normal/main.c normal/menu.c \
	disk/powerpc/ieee1275/ofdisk.c disk/powerpc/ieee1275/partition.c \
	kern/env.c normal/arg.c
CLEANFILES += pupaof pupaof-boot_powerpc_ieee1275_cmain.o pupaof-boot_powerpc_ieee1275_ieee1275.o pupaof-boot_powerpc_ieee1275_crt0.o pupaof-kern_main.o pupaof-kern_device.o pupaof-kern_disk.o pupaof-kern_dl.o pupaof-kern_file.o pupaof-kern_fs.o pupaof-kern_err.o pupaof-kern_misc.o pupaof-kern_mm.o pupaof-kern_loader.o pupaof-kern_rescue.o pupaof-kern_term.o pupaof-kern_powerpc_ieee1275_init.o pupaof-term_powerpc_ieee1275_ofconsole.o pupaof-kern_powerpc_ieee1275_openfw.o pupaof-fs_ext2.o pupaof-normal_cmdline.o pupaof-normal_command.o pupaof-normal_main.o pupaof-normal_menu.o pupaof-disk_powerpc_ieee1275_ofdisk.o pupaof-disk_powerpc_ieee1275_partition.o pupaof-kern_env.o pupaof-normal_arg.o
MOSTLYCLEANFILES += pupaof-boot_powerpc_ieee1275_cmain.d pupaof-boot_powerpc_ieee1275_ieee1275.d pupaof-boot_powerpc_ieee1275_crt0.d pupaof-kern_main.d pupaof-kern_device.d pupaof-kern_disk.d pupaof-kern_dl.d pupaof-kern_file.d pupaof-kern_fs.d pupaof-kern_err.d pupaof-kern_misc.d pupaof-kern_mm.d pupaof-kern_loader.d pupaof-kern_rescue.d pupaof-kern_term.d pupaof-kern_powerpc_ieee1275_init.d pupaof-term_powerpc_ieee1275_ofconsole.d pupaof-kern_powerpc_ieee1275_openfw.d pupaof-fs_ext2.d pupaof-normal_cmdline.d pupaof-normal_command.d pupaof-normal_main.d pupaof-normal_menu.d pupaof-disk_powerpc_ieee1275_ofdisk.d pupaof-disk_powerpc_ieee1275_partition.d pupaof-kern_env.d pupaof-normal_arg.d

pupaof: pupaof-boot_powerpc_ieee1275_cmain.o pupaof-boot_powerpc_ieee1275_ieee1275.o pupaof-boot_powerpc_ieee1275_crt0.o pupaof-kern_main.o pupaof-kern_device.o pupaof-kern_disk.o pupaof-kern_dl.o pupaof-kern_file.o pupaof-kern_fs.o pupaof-kern_err.o pupaof-kern_misc.o pupaof-kern_mm.o pupaof-kern_loader.o pupaof-kern_rescue.o pupaof-kern_term.o pupaof-kern_powerpc_ieee1275_init.o pupaof-term_powerpc_ieee1275_ofconsole.o pupaof-kern_powerpc_ieee1275_openfw.o pupaof-fs_ext2.o pupaof-normal_cmdline.o pupaof-normal_command.o pupaof-normal_main.o pupaof-normal_menu.o pupaof-disk_powerpc_ieee1275_ofdisk.o pupaof-disk_powerpc_ieee1275_partition.o pupaof-kern_env.o pupaof-normal_arg.o
	$(BUILD_CC) -o $@ $^ $(BUILD_LDFLAGS) $(pupaof_LDFLAGS)

pupaof-boot_powerpc_ieee1275_cmain.o: boot/powerpc/ieee1275/cmain.c
	$(BUILD_CC) -Iboot/powerpc/ieee1275 -I$(srcdir)/boot/powerpc/ieee1275 $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupaof_CFLAGS) -c -o $@ $<

pupaof-boot_powerpc_ieee1275_cmain.d: boot/powerpc/ieee1275/cmain.c
	set -e; 	  $(BUILD_CC) -Iboot/powerpc/ieee1275 -I$(srcdir)/boot/powerpc/ieee1275 $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupaof_CFLAGS) -M $< 	  | sed 's,cmain\.o[ :]*,pupaof-boot_powerpc_ieee1275_cmain.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupaof-boot_powerpc_ieee1275_cmain.d

pupaof-boot_powerpc_ieee1275_ieee1275.o: boot/powerpc/ieee1275/ieee1275.c
	$(BUILD_CC) -Iboot/powerpc/ieee1275 -I$(srcdir)/boot/powerpc/ieee1275 $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupaof_CFLAGS) -c -o $@ $<

pupaof-boot_powerpc_ieee1275_ieee1275.d: boot/powerpc/ieee1275/ieee1275.c
	set -e; 	  $(BUILD_CC) -Iboot/powerpc/ieee1275 -I$(srcdir)/boot/powerpc/ieee1275 $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupaof_CFLAGS) -M $< 	  | sed 's,ieee1275\.o[ :]*,pupaof-boot_powerpc_ieee1275_ieee1275.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupaof-boot_powerpc_ieee1275_ieee1275.d

pupaof-boot_powerpc_ieee1275_crt0.o: boot/powerpc/ieee1275/crt0.S
	$(BUILD_CC) -Iboot/powerpc/ieee1275 -I$(srcdir)/boot/powerpc/ieee1275 $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupaof_CFLAGS) -c -o $@ $<

pupaof-boot_powerpc_ieee1275_crt0.d: boot/powerpc/ieee1275/crt0.S
	set -e; 	  $(BUILD_CC) -Iboot/powerpc/ieee1275 -I$(srcdir)/boot/powerpc/ieee1275 $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupaof_CFLAGS) -M $< 	  | sed 's,crt0\.o[ :]*,pupaof-boot_powerpc_ieee1275_crt0.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupaof-boot_powerpc_ieee1275_crt0.d

pupaof-kern_main.o: kern/main.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupaof_CFLAGS) -c -o $@ $<

pupaof-kern_main.d: kern/main.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupaof_CFLAGS) -M $< 	  | sed 's,main\.o[ :]*,pupaof-kern_main.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupaof-kern_main.d

pupaof-kern_device.o: kern/device.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupaof_CFLAGS) -c -o $@ $<

pupaof-kern_device.d: kern/device.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupaof_CFLAGS) -M $< 	  | sed 's,device\.o[ :]*,pupaof-kern_device.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupaof-kern_device.d

pupaof-kern_disk.o: kern/disk.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupaof_CFLAGS) -c -o $@ $<

pupaof-kern_disk.d: kern/disk.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupaof_CFLAGS) -M $< 	  | sed 's,disk\.o[ :]*,pupaof-kern_disk.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupaof-kern_disk.d

pupaof-kern_dl.o: kern/dl.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupaof_CFLAGS) -c -o $@ $<

pupaof-kern_dl.d: kern/dl.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupaof_CFLAGS) -M $< 	  | sed 's,dl\.o[ :]*,pupaof-kern_dl.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupaof-kern_dl.d

pupaof-kern_file.o: kern/file.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupaof_CFLAGS) -c -o $@ $<

pupaof-kern_file.d: kern/file.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupaof_CFLAGS) -M $< 	  | sed 's,file\.o[ :]*,pupaof-kern_file.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupaof-kern_file.d

pupaof-kern_fs.o: kern/fs.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupaof_CFLAGS) -c -o $@ $<

pupaof-kern_fs.d: kern/fs.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupaof_CFLAGS) -M $< 	  | sed 's,fs\.o[ :]*,pupaof-kern_fs.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupaof-kern_fs.d

pupaof-kern_err.o: kern/err.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupaof_CFLAGS) -c -o $@ $<

pupaof-kern_err.d: kern/err.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupaof_CFLAGS) -M $< 	  | sed 's,err\.o[ :]*,pupaof-kern_err.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupaof-kern_err.d

pupaof-kern_misc.o: kern/misc.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupaof_CFLAGS) -c -o $@ $<

pupaof-kern_misc.d: kern/misc.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupaof_CFLAGS) -M $< 	  | sed 's,misc\.o[ :]*,pupaof-kern_misc.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupaof-kern_misc.d

pupaof-kern_mm.o: kern/mm.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupaof_CFLAGS) -c -o $@ $<

pupaof-kern_mm.d: kern/mm.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupaof_CFLAGS) -M $< 	  | sed 's,mm\.o[ :]*,pupaof-kern_mm.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupaof-kern_mm.d

pupaof-kern_loader.o: kern/loader.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupaof_CFLAGS) -c -o $@ $<

pupaof-kern_loader.d: kern/loader.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupaof_CFLAGS) -M $< 	  | sed 's,loader\.o[ :]*,pupaof-kern_loader.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupaof-kern_loader.d

pupaof-kern_rescue.o: kern/rescue.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupaof_CFLAGS) -c -o $@ $<

pupaof-kern_rescue.d: kern/rescue.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupaof_CFLAGS) -M $< 	  | sed 's,rescue\.o[ :]*,pupaof-kern_rescue.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupaof-kern_rescue.d

pupaof-kern_term.o: kern/term.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupaof_CFLAGS) -c -o $@ $<

pupaof-kern_term.d: kern/term.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupaof_CFLAGS) -M $< 	  | sed 's,term\.o[ :]*,pupaof-kern_term.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupaof-kern_term.d

pupaof-kern_powerpc_ieee1275_init.o: kern/powerpc/ieee1275/init.c
	$(BUILD_CC) -Ikern/powerpc/ieee1275 -I$(srcdir)/kern/powerpc/ieee1275 $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupaof_CFLAGS) -c -o $@ $<

pupaof-kern_powerpc_ieee1275_init.d: kern/powerpc/ieee1275/init.c
	set -e; 	  $(BUILD_CC) -Ikern/powerpc/ieee1275 -I$(srcdir)/kern/powerpc/ieee1275 $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupaof_CFLAGS) -M $< 	  | sed 's,init\.o[ :]*,pupaof-kern_powerpc_ieee1275_init.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupaof-kern_powerpc_ieee1275_init.d

pupaof-term_powerpc_ieee1275_ofconsole.o: term/powerpc/ieee1275/ofconsole.c
	$(BUILD_CC) -Iterm/powerpc/ieee1275 -I$(srcdir)/term/powerpc/ieee1275 $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupaof_CFLAGS) -c -o $@ $<

pupaof-term_powerpc_ieee1275_ofconsole.d: term/powerpc/ieee1275/ofconsole.c
	set -e; 	  $(BUILD_CC) -Iterm/powerpc/ieee1275 -I$(srcdir)/term/powerpc/ieee1275 $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupaof_CFLAGS) -M $< 	  | sed 's,ofconsole\.o[ :]*,pupaof-term_powerpc_ieee1275_ofconsole.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupaof-term_powerpc_ieee1275_ofconsole.d

pupaof-kern_powerpc_ieee1275_openfw.o: kern/powerpc/ieee1275/openfw.c
	$(BUILD_CC) -Ikern/powerpc/ieee1275 -I$(srcdir)/kern/powerpc/ieee1275 $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupaof_CFLAGS) -c -o $@ $<

pupaof-kern_powerpc_ieee1275_openfw.d: kern/powerpc/ieee1275/openfw.c
	set -e; 	  $(BUILD_CC) -Ikern/powerpc/ieee1275 -I$(srcdir)/kern/powerpc/ieee1275 $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupaof_CFLAGS) -M $< 	  | sed 's,openfw\.o[ :]*,pupaof-kern_powerpc_ieee1275_openfw.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupaof-kern_powerpc_ieee1275_openfw.d

pupaof-fs_ext2.o: fs/ext2.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupaof_CFLAGS) -c -o $@ $<

pupaof-fs_ext2.d: fs/ext2.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupaof_CFLAGS) -M $< 	  | sed 's,ext2\.o[ :]*,pupaof-fs_ext2.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupaof-fs_ext2.d

pupaof-normal_cmdline.o: normal/cmdline.c
	$(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupaof_CFLAGS) -c -o $@ $<

pupaof-normal_cmdline.d: normal/cmdline.c
	set -e; 	  $(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupaof_CFLAGS) -M $< 	  | sed 's,cmdline\.o[ :]*,pupaof-normal_cmdline.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupaof-normal_cmdline.d

pupaof-normal_command.o: normal/command.c
	$(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupaof_CFLAGS) -c -o $@ $<

pupaof-normal_command.d: normal/command.c
	set -e; 	  $(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupaof_CFLAGS) -M $< 	  | sed 's,command\.o[ :]*,pupaof-normal_command.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupaof-normal_command.d

pupaof-normal_main.o: normal/main.c
	$(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupaof_CFLAGS) -c -o $@ $<

pupaof-normal_main.d: normal/main.c
	set -e; 	  $(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupaof_CFLAGS) -M $< 	  | sed 's,main\.o[ :]*,pupaof-normal_main.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupaof-normal_main.d

pupaof-normal_menu.o: normal/menu.c
	$(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupaof_CFLAGS) -c -o $@ $<

pupaof-normal_menu.d: normal/menu.c
	set -e; 	  $(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupaof_CFLAGS) -M $< 	  | sed 's,menu\.o[ :]*,pupaof-normal_menu.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupaof-normal_menu.d

pupaof-disk_powerpc_ieee1275_ofdisk.o: disk/powerpc/ieee1275/ofdisk.c
	$(BUILD_CC) -Idisk/powerpc/ieee1275 -I$(srcdir)/disk/powerpc/ieee1275 $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupaof_CFLAGS) -c -o $@ $<

pupaof-disk_powerpc_ieee1275_ofdisk.d: disk/powerpc/ieee1275/ofdisk.c
	set -e; 	  $(BUILD_CC) -Idisk/powerpc/ieee1275 -I$(srcdir)/disk/powerpc/ieee1275 $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupaof_CFLAGS) -M $< 	  | sed 's,ofdisk\.o[ :]*,pupaof-disk_powerpc_ieee1275_ofdisk.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupaof-disk_powerpc_ieee1275_ofdisk.d

pupaof-disk_powerpc_ieee1275_partition.o: disk/powerpc/ieee1275/partition.c
	$(BUILD_CC) -Idisk/powerpc/ieee1275 -I$(srcdir)/disk/powerpc/ieee1275 $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupaof_CFLAGS) -c -o $@ $<

pupaof-disk_powerpc_ieee1275_partition.d: disk/powerpc/ieee1275/partition.c
	set -e; 	  $(BUILD_CC) -Idisk/powerpc/ieee1275 -I$(srcdir)/disk/powerpc/ieee1275 $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupaof_CFLAGS) -M $< 	  | sed 's,partition\.o[ :]*,pupaof-disk_powerpc_ieee1275_partition.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupaof-disk_powerpc_ieee1275_partition.d

pupaof-kern_env.o: kern/env.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupaof_CFLAGS) -c -o $@ $<

pupaof-kern_env.d: kern/env.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupaof_CFLAGS) -M $< 	  | sed 's,env\.o[ :]*,pupaof-kern_env.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupaof-kern_env.d

pupaof-normal_arg.o: normal/arg.c
	$(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupaof_CFLAGS) -c -o $@ $<

pupaof-normal_arg.d: normal/arg.c
	set -e; 	  $(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupaof_CFLAGS) -M $< 	  | sed 's,arg\.o[ :]*,pupaof-normal_arg.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupaof-normal_arg.d

pupaof_HEADERS = pupa/powerpc/ieee1275/ieee1275.h
pupaof_CFLAGS = $(COMMON_CFLAGS)
pupaof_ASFLAGS = $(COMMON_ASFLAGS)
pupaof_LDFLAGS = -Wl,-Ttext,0x200000,-Bstatic

# For genmoddep.
genmoddep_SOURCES = util/genmoddep.c
CLEANFILES += genmoddep genmoddep-util_genmoddep.o
MOSTLYCLEANFILES += genmoddep-util_genmoddep.d

genmoddep: genmoddep-util_genmoddep.o
	$(BUILD_CC) -o $@ $^ $(BUILD_LDFLAGS) $(genmoddep_LDFLAGS)

genmoddep-util_genmoddep.o: util/genmoddep.c
	$(BUILD_CC) -Iutil -I$(srcdir)/util $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(genmoddep_CFLAGS) -c -o $@ $<

genmoddep-util_genmoddep.d: util/genmoddep.c
	set -e; 	  $(BUILD_CC) -Iutil -I$(srcdir)/util $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(genmoddep_CFLAGS) -M $< 	  | sed 's,genmoddep\.o[ :]*,genmoddep-util_genmoddep.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include genmoddep-util_genmoddep.d


# Modules.
CLEANFILES += moddep.lst
pkgdata_DATA += moddep.lst
moddep.lst: $(DEFSYMFILES) $(UNDSYMFILES) genmoddep
	cat $(DEFSYMFILES) /dev/null | ./genmoddep $(UNDSYMFILES) > $@ \
	  || (rm -f $@; exit 1)
