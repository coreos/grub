# -*- makefile -*-

COMMON_ASFLAGS = -nostdinc -fno-builtin
COMMON_CFLAGS = -fno-builtin -mrtd -mregparm=3

# Images.
pkgdata_IMAGES = boot.img diskboot.img kernel.img pxeboot.img

# For boot.img.
boot_img_SOURCES = boot/i386/pc/boot.S
CLEANFILES += boot.img boot.exec boot_img-boot_i386_pc_boot.o
MOSTLYCLEANFILES += boot_img-boot_i386_pc_boot.d

boot.img: boot.exec
	$(OBJCOPY) -O binary -R .note -R .comment $< $@

boot.exec: boot_img-boot_i386_pc_boot.o
	$(CC) -o $@ $^ $(LDFLAGS) $(boot_img_LDFLAGS)

boot_img-boot_i386_pc_boot.o: boot/i386/pc/boot.S
	$(CC) -Iboot/i386/pc -I$(srcdir)/boot/i386/pc $(CPPFLAGS) -DASM_FILE=1 $(ASFLAGS) $(boot_img_ASFLAGS) -c -o $@ $<

boot_img-boot_i386_pc_boot.d: boot/i386/pc/boot.S
	set -e; 	  $(CC) -Iboot/i386/pc -I$(srcdir)/boot/i386/pc $(CPPFLAGS) -DASM_FILE=1 $(ASFLAGS) $(boot_img_ASFLAGS) -M $< 	  | sed 's,boot\.o[ :]*,boot_img-boot_i386_pc_boot.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include boot_img-boot_i386_pc_boot.d

boot_img_ASFLAGS = $(COMMON_ASFLAGS)
boot_img_LDFLAGS = -nostdlib -Wl,-N,-Ttext,7C00

# For pxeboot.img
pxeboot_img_SOURCES = boot/i386/pc/pxeboot.S
CLEANFILES += pxeboot.img pxeboot.exec pxeboot_img-boot_i386_pc_pxeboot.o
MOSTLYCLEANFILES += pxeboot_img-boot_i386_pc_pxeboot.d

pxeboot.img: pxeboot.exec
	$(OBJCOPY) -O binary -R .note -R .comment $< $@

pxeboot.exec: pxeboot_img-boot_i386_pc_pxeboot.o
	$(CC) -o $@ $^ $(LDFLAGS) $(pxeboot_img_LDFLAGS)

pxeboot_img-boot_i386_pc_pxeboot.o: boot/i386/pc/pxeboot.S
	$(CC) -Iboot/i386/pc -I$(srcdir)/boot/i386/pc $(CPPFLAGS) -DASM_FILE=1 $(ASFLAGS) $(pxeboot_img_ASFLAGS) -c -o $@ $<

pxeboot_img-boot_i386_pc_pxeboot.d: boot/i386/pc/pxeboot.S
	set -e; 	  $(CC) -Iboot/i386/pc -I$(srcdir)/boot/i386/pc $(CPPFLAGS) -DASM_FILE=1 $(ASFLAGS) $(pxeboot_img_ASFLAGS) -M $< 	  | sed 's,pxeboot\.o[ :]*,pxeboot_img-boot_i386_pc_pxeboot.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pxeboot_img-boot_i386_pc_pxeboot.d

pxeboot_img_ASFLAGS = $(COMMON_ASFLAGS)
pxeboot_img_LDFLAGS = -nostdlib -Wl,-N,-Ttext,8000

# For diskboot.img.
diskboot_img_SOURCES = boot/i386/pc/diskboot.S
CLEANFILES += diskboot.img diskboot.exec diskboot_img-boot_i386_pc_diskboot.o
MOSTLYCLEANFILES += diskboot_img-boot_i386_pc_diskboot.d

diskboot.img: diskboot.exec
	$(OBJCOPY) -O binary -R .note -R .comment $< $@

diskboot.exec: diskboot_img-boot_i386_pc_diskboot.o
	$(CC) -o $@ $^ $(LDFLAGS) $(diskboot_img_LDFLAGS)

diskboot_img-boot_i386_pc_diskboot.o: boot/i386/pc/diskboot.S
	$(CC) -Iboot/i386/pc -I$(srcdir)/boot/i386/pc $(CPPFLAGS) -DASM_FILE=1 $(ASFLAGS) $(diskboot_img_ASFLAGS) -c -o $@ $<

diskboot_img-boot_i386_pc_diskboot.d: boot/i386/pc/diskboot.S
	set -e; 	  $(CC) -Iboot/i386/pc -I$(srcdir)/boot/i386/pc $(CPPFLAGS) -DASM_FILE=1 $(ASFLAGS) $(diskboot_img_ASFLAGS) -M $< 	  | sed 's,diskboot\.o[ :]*,diskboot_img-boot_i386_pc_diskboot.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include diskboot_img-boot_i386_pc_diskboot.d

diskboot_img_ASFLAGS = $(COMMON_ASFLAGS)
diskboot_img_LDFLAGS = -nostdlib -Wl,-N,-Ttext,8000

# For kernel.img.
kernel_img_SOURCES = kern/i386/pc/startup.S kern/main.c kern/device.c \
	kern/disk.c kern/dl.c kern/file.c kern/fs.c kern/err.c \
	kern/misc.c kern/mm.c kern/loader.c kern/rescue.c kern/term.c \
	kern/i386/dl.c kern/i386/pc/init.c kern/partition.c \
	kern/env.c disk/i386/pc/biosdisk.c \
	term/i386/pc/console.c \
	symlist.c
CLEANFILES += kernel.img kernel.exec kernel_img-kern_i386_pc_startup.o kernel_img-kern_main.o kernel_img-kern_device.o kernel_img-kern_disk.o kernel_img-kern_dl.o kernel_img-kern_file.o kernel_img-kern_fs.o kernel_img-kern_err.o kernel_img-kern_misc.o kernel_img-kern_mm.o kernel_img-kern_loader.o kernel_img-kern_rescue.o kernel_img-kern_term.o kernel_img-kern_i386_dl.o kernel_img-kern_i386_pc_init.o kernel_img-kern_partition.o kernel_img-kern_env.o kernel_img-disk_i386_pc_biosdisk.o kernel_img-term_i386_pc_console.o kernel_img-symlist.o
MOSTLYCLEANFILES += kernel_img-kern_i386_pc_startup.d kernel_img-kern_main.d kernel_img-kern_device.d kernel_img-kern_disk.d kernel_img-kern_dl.d kernel_img-kern_file.d kernel_img-kern_fs.d kernel_img-kern_err.d kernel_img-kern_misc.d kernel_img-kern_mm.d kernel_img-kern_loader.d kernel_img-kern_rescue.d kernel_img-kern_term.d kernel_img-kern_i386_dl.d kernel_img-kern_i386_pc_init.d kernel_img-kern_partition.d kernel_img-kern_env.d kernel_img-disk_i386_pc_biosdisk.d kernel_img-term_i386_pc_console.d kernel_img-symlist.d

kernel.img: kernel.exec
	$(OBJCOPY) -O binary -R .note -R .comment $< $@

kernel.exec: kernel_img-kern_i386_pc_startup.o kernel_img-kern_main.o kernel_img-kern_device.o kernel_img-kern_disk.o kernel_img-kern_dl.o kernel_img-kern_file.o kernel_img-kern_fs.o kernel_img-kern_err.o kernel_img-kern_misc.o kernel_img-kern_mm.o kernel_img-kern_loader.o kernel_img-kern_rescue.o kernel_img-kern_term.o kernel_img-kern_i386_dl.o kernel_img-kern_i386_pc_init.o kernel_img-kern_partition.o kernel_img-kern_env.o kernel_img-disk_i386_pc_biosdisk.o kernel_img-term_i386_pc_console.o kernel_img-symlist.o
	$(CC) -o $@ $^ $(LDFLAGS) $(kernel_img_LDFLAGS)

kernel_img-kern_i386_pc_startup.o: kern/i386/pc/startup.S
	$(CC) -Ikern/i386/pc -I$(srcdir)/kern/i386/pc $(CPPFLAGS) -DASM_FILE=1 $(ASFLAGS) $(kernel_img_ASFLAGS) -c -o $@ $<

kernel_img-kern_i386_pc_startup.d: kern/i386/pc/startup.S
	set -e; 	  $(CC) -Ikern/i386/pc -I$(srcdir)/kern/i386/pc $(CPPFLAGS) -DASM_FILE=1 $(ASFLAGS) $(kernel_img_ASFLAGS) -M $< 	  | sed 's,startup\.o[ :]*,kernel_img-kern_i386_pc_startup.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include kernel_img-kern_i386_pc_startup.d

kernel_img-kern_main.o: kern/main.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS)  $(CFLAGS) $(kernel_img_CFLAGS) -c -o $@ $<

kernel_img-kern_main.d: kern/main.c
	set -e; 	  $(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS)  $(CFLAGS) $(kernel_img_CFLAGS) -M $< 	  | sed 's,main\.o[ :]*,kernel_img-kern_main.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include kernel_img-kern_main.d

kernel_img-kern_device.o: kern/device.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS)  $(CFLAGS) $(kernel_img_CFLAGS) -c -o $@ $<

kernel_img-kern_device.d: kern/device.c
	set -e; 	  $(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS)  $(CFLAGS) $(kernel_img_CFLAGS) -M $< 	  | sed 's,device\.o[ :]*,kernel_img-kern_device.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include kernel_img-kern_device.d

kernel_img-kern_disk.o: kern/disk.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS)  $(CFLAGS) $(kernel_img_CFLAGS) -c -o $@ $<

kernel_img-kern_disk.d: kern/disk.c
	set -e; 	  $(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS)  $(CFLAGS) $(kernel_img_CFLAGS) -M $< 	  | sed 's,disk\.o[ :]*,kernel_img-kern_disk.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include kernel_img-kern_disk.d

kernel_img-kern_dl.o: kern/dl.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS)  $(CFLAGS) $(kernel_img_CFLAGS) -c -o $@ $<

kernel_img-kern_dl.d: kern/dl.c
	set -e; 	  $(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS)  $(CFLAGS) $(kernel_img_CFLAGS) -M $< 	  | sed 's,dl\.o[ :]*,kernel_img-kern_dl.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include kernel_img-kern_dl.d

kernel_img-kern_file.o: kern/file.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS)  $(CFLAGS) $(kernel_img_CFLAGS) -c -o $@ $<

kernel_img-kern_file.d: kern/file.c
	set -e; 	  $(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS)  $(CFLAGS) $(kernel_img_CFLAGS) -M $< 	  | sed 's,file\.o[ :]*,kernel_img-kern_file.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include kernel_img-kern_file.d

kernel_img-kern_fs.o: kern/fs.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS)  $(CFLAGS) $(kernel_img_CFLAGS) -c -o $@ $<

kernel_img-kern_fs.d: kern/fs.c
	set -e; 	  $(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS)  $(CFLAGS) $(kernel_img_CFLAGS) -M $< 	  | sed 's,fs\.o[ :]*,kernel_img-kern_fs.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include kernel_img-kern_fs.d

kernel_img-kern_err.o: kern/err.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS)  $(CFLAGS) $(kernel_img_CFLAGS) -c -o $@ $<

kernel_img-kern_err.d: kern/err.c
	set -e; 	  $(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS)  $(CFLAGS) $(kernel_img_CFLAGS) -M $< 	  | sed 's,err\.o[ :]*,kernel_img-kern_err.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include kernel_img-kern_err.d

kernel_img-kern_misc.o: kern/misc.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS)  $(CFLAGS) $(kernel_img_CFLAGS) -c -o $@ $<

kernel_img-kern_misc.d: kern/misc.c
	set -e; 	  $(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS)  $(CFLAGS) $(kernel_img_CFLAGS) -M $< 	  | sed 's,misc\.o[ :]*,kernel_img-kern_misc.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include kernel_img-kern_misc.d

kernel_img-kern_mm.o: kern/mm.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS)  $(CFLAGS) $(kernel_img_CFLAGS) -c -o $@ $<

kernel_img-kern_mm.d: kern/mm.c
	set -e; 	  $(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS)  $(CFLAGS) $(kernel_img_CFLAGS) -M $< 	  | sed 's,mm\.o[ :]*,kernel_img-kern_mm.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include kernel_img-kern_mm.d

kernel_img-kern_loader.o: kern/loader.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS)  $(CFLAGS) $(kernel_img_CFLAGS) -c -o $@ $<

kernel_img-kern_loader.d: kern/loader.c
	set -e; 	  $(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS)  $(CFLAGS) $(kernel_img_CFLAGS) -M $< 	  | sed 's,loader\.o[ :]*,kernel_img-kern_loader.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include kernel_img-kern_loader.d

kernel_img-kern_rescue.o: kern/rescue.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS)  $(CFLAGS) $(kernel_img_CFLAGS) -c -o $@ $<

kernel_img-kern_rescue.d: kern/rescue.c
	set -e; 	  $(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS)  $(CFLAGS) $(kernel_img_CFLAGS) -M $< 	  | sed 's,rescue\.o[ :]*,kernel_img-kern_rescue.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include kernel_img-kern_rescue.d

kernel_img-kern_term.o: kern/term.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS)  $(CFLAGS) $(kernel_img_CFLAGS) -c -o $@ $<

kernel_img-kern_term.d: kern/term.c
	set -e; 	  $(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS)  $(CFLAGS) $(kernel_img_CFLAGS) -M $< 	  | sed 's,term\.o[ :]*,kernel_img-kern_term.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include kernel_img-kern_term.d

kernel_img-kern_i386_dl.o: kern/i386/dl.c
	$(CC) -Ikern/i386 -I$(srcdir)/kern/i386 $(CPPFLAGS)  $(CFLAGS) $(kernel_img_CFLAGS) -c -o $@ $<

kernel_img-kern_i386_dl.d: kern/i386/dl.c
	set -e; 	  $(CC) -Ikern/i386 -I$(srcdir)/kern/i386 $(CPPFLAGS)  $(CFLAGS) $(kernel_img_CFLAGS) -M $< 	  | sed 's,dl\.o[ :]*,kernel_img-kern_i386_dl.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include kernel_img-kern_i386_dl.d

kernel_img-kern_i386_pc_init.o: kern/i386/pc/init.c
	$(CC) -Ikern/i386/pc -I$(srcdir)/kern/i386/pc $(CPPFLAGS)  $(CFLAGS) $(kernel_img_CFLAGS) -c -o $@ $<

kernel_img-kern_i386_pc_init.d: kern/i386/pc/init.c
	set -e; 	  $(CC) -Ikern/i386/pc -I$(srcdir)/kern/i386/pc $(CPPFLAGS)  $(CFLAGS) $(kernel_img_CFLAGS) -M $< 	  | sed 's,init\.o[ :]*,kernel_img-kern_i386_pc_init.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include kernel_img-kern_i386_pc_init.d

kernel_img-kern_partition.o: kern/partition.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS)  $(CFLAGS) $(kernel_img_CFLAGS) -c -o $@ $<

kernel_img-kern_partition.d: kern/partition.c
	set -e; 	  $(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS)  $(CFLAGS) $(kernel_img_CFLAGS) -M $< 	  | sed 's,partition\.o[ :]*,kernel_img-kern_partition.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include kernel_img-kern_partition.d

kernel_img-kern_env.o: kern/env.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS)  $(CFLAGS) $(kernel_img_CFLAGS) -c -o $@ $<

kernel_img-kern_env.d: kern/env.c
	set -e; 	  $(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS)  $(CFLAGS) $(kernel_img_CFLAGS) -M $< 	  | sed 's,env\.o[ :]*,kernel_img-kern_env.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include kernel_img-kern_env.d

kernel_img-disk_i386_pc_biosdisk.o: disk/i386/pc/biosdisk.c
	$(CC) -Idisk/i386/pc -I$(srcdir)/disk/i386/pc $(CPPFLAGS)  $(CFLAGS) $(kernel_img_CFLAGS) -c -o $@ $<

kernel_img-disk_i386_pc_biosdisk.d: disk/i386/pc/biosdisk.c
	set -e; 	  $(CC) -Idisk/i386/pc -I$(srcdir)/disk/i386/pc $(CPPFLAGS)  $(CFLAGS) $(kernel_img_CFLAGS) -M $< 	  | sed 's,biosdisk\.o[ :]*,kernel_img-disk_i386_pc_biosdisk.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include kernel_img-disk_i386_pc_biosdisk.d

kernel_img-term_i386_pc_console.o: term/i386/pc/console.c
	$(CC) -Iterm/i386/pc -I$(srcdir)/term/i386/pc $(CPPFLAGS)  $(CFLAGS) $(kernel_img_CFLAGS) -c -o $@ $<

kernel_img-term_i386_pc_console.d: term/i386/pc/console.c
	set -e; 	  $(CC) -Iterm/i386/pc -I$(srcdir)/term/i386/pc $(CPPFLAGS)  $(CFLAGS) $(kernel_img_CFLAGS) -M $< 	  | sed 's,console\.o[ :]*,kernel_img-term_i386_pc_console.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include kernel_img-term_i386_pc_console.d

kernel_img-symlist.o: symlist.c
	$(CC) -I. -I$(srcdir)/. $(CPPFLAGS)  $(CFLAGS) $(kernel_img_CFLAGS) -c -o $@ $<

kernel_img-symlist.d: symlist.c
	set -e; 	  $(CC) -I. -I$(srcdir)/. $(CPPFLAGS)  $(CFLAGS) $(kernel_img_CFLAGS) -M $< 	  | sed 's,symlist\.o[ :]*,kernel_img-symlist.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include kernel_img-symlist.d

kernel_img_HEADERS = arg.h boot.h device.h disk.h dl.h elf.h env.h err.h \
	file.h fs.h kernel.h loader.h misc.h mm.h net.h rescue.h symbol.h \
	term.h types.h machine/biosdisk.h machine/boot.h \
	machine/console.h machine/init.h machine/memory.h \
	machine/loader.h partition.h pc_partition.h machine/time.h machine/vga.h
kernel_img_CFLAGS = $(COMMON_CFLAGS)
kernel_img_ASFLAGS = $(COMMON_ASFLAGS)
kernel_img_LDFLAGS = -nostdlib -Wl,-N,-Ttext,8200

MOSTLYCLEANFILES += symlist.c kernel_syms.lst
DEFSYMFILES += kernel_syms.lst

symlist.c: $(addprefix include/grub/,$(kernel_img_HEADERS)) gensymlist.sh
	sh $(srcdir)/gensymlist.sh $(filter %.h,$^) > $@

kernel_syms.lst: $(addprefix include/grub/,$(kernel_img_HEADERS)) genkernsyms.sh
	sh $(srcdir)/genkernsyms.sh $(filter %h,$^) > $@

# Utilities.
bin_UTILITIES = grub-mkimage
sbin_UTILITIES = grub-setup grub-emu
noinst_UTILITIES = genmoddep

# For grub-mkimage.
grub_mkimage_SOURCES = util/i386/pc/grub-mkimage.c util/misc.c \
	util/resolve.c
CLEANFILES += grub-mkimage grub_mkimage-util_i386_pc_grub_mkimage.o grub_mkimage-util_misc.o grub_mkimage-util_resolve.o
MOSTLYCLEANFILES += grub_mkimage-util_i386_pc_grub_mkimage.d grub_mkimage-util_misc.d grub_mkimage-util_resolve.d

grub-mkimage: grub_mkimage-util_i386_pc_grub_mkimage.o grub_mkimage-util_misc.o grub_mkimage-util_resolve.o
	$(BUILD_CC) -o $@ $^ $(BUILD_LDFLAGS) $(grub_mkimage_LDFLAGS)

grub_mkimage-util_i386_pc_grub_mkimage.o: util/i386/pc/grub-mkimage.c
	$(BUILD_CC) -Iutil/i386/pc -I$(srcdir)/util/i386/pc $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_mkimage_CFLAGS) -c -o $@ $<

grub_mkimage-util_i386_pc_grub_mkimage.d: util/i386/pc/grub-mkimage.c
	set -e; 	  $(BUILD_CC) -Iutil/i386/pc -I$(srcdir)/util/i386/pc $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_mkimage_CFLAGS) -M $< 	  | sed 's,grub\-mkimage\.o[ :]*,grub_mkimage-util_i386_pc_grub_mkimage.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_mkimage-util_i386_pc_grub_mkimage.d

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

grub_mkimage_LDFLAGS = -llzo

# For grub-setup.
grub_setup_SOURCES = util/i386/pc/grub-setup.c util/i386/pc/biosdisk.c \
	util/misc.c util/i386/pc/getroot.c kern/device.c kern/disk.c \
	kern/err.c kern/misc.c fs/fat.c fs/ext2.c \
	kern/partition.c partmap/amiga.c partmap/apple.c partmap/pc.c partmap/sun.c \
	fs/ufs.c fs/minix.c fs/hfs.c fs/jfs.c kern/file.c kern/fs.c kern/env.c fs/fshelp.c
CLEANFILES += grub-setup grub_setup-util_i386_pc_grub_setup.o grub_setup-util_i386_pc_biosdisk.o grub_setup-util_misc.o grub_setup-util_i386_pc_getroot.o grub_setup-kern_device.o grub_setup-kern_disk.o grub_setup-kern_err.o grub_setup-kern_misc.o grub_setup-fs_fat.o grub_setup-fs_ext2.o grub_setup-kern_partition.o grub_setup-partmap_amiga.o grub_setup-partmap_apple.o grub_setup-partmap_pc.o grub_setup-partmap_sun.o grub_setup-fs_ufs.o grub_setup-fs_minix.o grub_setup-fs_hfs.o grub_setup-fs_jfs.o grub_setup-kern_file.o grub_setup-kern_fs.o grub_setup-kern_env.o grub_setup-fs_fshelp.o
MOSTLYCLEANFILES += grub_setup-util_i386_pc_grub_setup.d grub_setup-util_i386_pc_biosdisk.d grub_setup-util_misc.d grub_setup-util_i386_pc_getroot.d grub_setup-kern_device.d grub_setup-kern_disk.d grub_setup-kern_err.d grub_setup-kern_misc.d grub_setup-fs_fat.d grub_setup-fs_ext2.d grub_setup-kern_partition.d grub_setup-partmap_amiga.d grub_setup-partmap_apple.d grub_setup-partmap_pc.d grub_setup-partmap_sun.d grub_setup-fs_ufs.d grub_setup-fs_minix.d grub_setup-fs_hfs.d grub_setup-fs_jfs.d grub_setup-kern_file.d grub_setup-kern_fs.d grub_setup-kern_env.d grub_setup-fs_fshelp.d

grub-setup: grub_setup-util_i386_pc_grub_setup.o grub_setup-util_i386_pc_biosdisk.o grub_setup-util_misc.o grub_setup-util_i386_pc_getroot.o grub_setup-kern_device.o grub_setup-kern_disk.o grub_setup-kern_err.o grub_setup-kern_misc.o grub_setup-fs_fat.o grub_setup-fs_ext2.o grub_setup-kern_partition.o grub_setup-partmap_amiga.o grub_setup-partmap_apple.o grub_setup-partmap_pc.o grub_setup-partmap_sun.o grub_setup-fs_ufs.o grub_setup-fs_minix.o grub_setup-fs_hfs.o grub_setup-fs_jfs.o grub_setup-kern_file.o grub_setup-kern_fs.o grub_setup-kern_env.o grub_setup-fs_fshelp.o
	$(BUILD_CC) -o $@ $^ $(BUILD_LDFLAGS) $(grub_setup_LDFLAGS)

grub_setup-util_i386_pc_grub_setup.o: util/i386/pc/grub-setup.c
	$(BUILD_CC) -Iutil/i386/pc -I$(srcdir)/util/i386/pc $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -c -o $@ $<

grub_setup-util_i386_pc_grub_setup.d: util/i386/pc/grub-setup.c
	set -e; 	  $(BUILD_CC) -Iutil/i386/pc -I$(srcdir)/util/i386/pc $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -M $< 	  | sed 's,grub\-setup\.o[ :]*,grub_setup-util_i386_pc_grub_setup.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_setup-util_i386_pc_grub_setup.d

grub_setup-util_i386_pc_biosdisk.o: util/i386/pc/biosdisk.c
	$(BUILD_CC) -Iutil/i386/pc -I$(srcdir)/util/i386/pc $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -c -o $@ $<

grub_setup-util_i386_pc_biosdisk.d: util/i386/pc/biosdisk.c
	set -e; 	  $(BUILD_CC) -Iutil/i386/pc -I$(srcdir)/util/i386/pc $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -M $< 	  | sed 's,biosdisk\.o[ :]*,grub_setup-util_i386_pc_biosdisk.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_setup-util_i386_pc_biosdisk.d

grub_setup-util_misc.o: util/misc.c
	$(BUILD_CC) -Iutil -I$(srcdir)/util $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -c -o $@ $<

grub_setup-util_misc.d: util/misc.c
	set -e; 	  $(BUILD_CC) -Iutil -I$(srcdir)/util $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -M $< 	  | sed 's,misc\.o[ :]*,grub_setup-util_misc.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_setup-util_misc.d

grub_setup-util_i386_pc_getroot.o: util/i386/pc/getroot.c
	$(BUILD_CC) -Iutil/i386/pc -I$(srcdir)/util/i386/pc $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -c -o $@ $<

grub_setup-util_i386_pc_getroot.d: util/i386/pc/getroot.c
	set -e; 	  $(BUILD_CC) -Iutil/i386/pc -I$(srcdir)/util/i386/pc $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -M $< 	  | sed 's,getroot\.o[ :]*,grub_setup-util_i386_pc_getroot.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_setup-util_i386_pc_getroot.d

grub_setup-kern_device.o: kern/device.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -c -o $@ $<

grub_setup-kern_device.d: kern/device.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -M $< 	  | sed 's,device\.o[ :]*,grub_setup-kern_device.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_setup-kern_device.d

grub_setup-kern_disk.o: kern/disk.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -c -o $@ $<

grub_setup-kern_disk.d: kern/disk.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -M $< 	  | sed 's,disk\.o[ :]*,grub_setup-kern_disk.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_setup-kern_disk.d

grub_setup-kern_err.o: kern/err.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -c -o $@ $<

grub_setup-kern_err.d: kern/err.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -M $< 	  | sed 's,err\.o[ :]*,grub_setup-kern_err.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_setup-kern_err.d

grub_setup-kern_misc.o: kern/misc.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -c -o $@ $<

grub_setup-kern_misc.d: kern/misc.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -M $< 	  | sed 's,misc\.o[ :]*,grub_setup-kern_misc.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_setup-kern_misc.d

grub_setup-fs_fat.o: fs/fat.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -c -o $@ $<

grub_setup-fs_fat.d: fs/fat.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -M $< 	  | sed 's,fat\.o[ :]*,grub_setup-fs_fat.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_setup-fs_fat.d

grub_setup-fs_ext2.o: fs/ext2.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -c -o $@ $<

grub_setup-fs_ext2.d: fs/ext2.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -M $< 	  | sed 's,ext2\.o[ :]*,grub_setup-fs_ext2.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_setup-fs_ext2.d

grub_setup-kern_partition.o: kern/partition.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -c -o $@ $<

grub_setup-kern_partition.d: kern/partition.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -M $< 	  | sed 's,partition\.o[ :]*,grub_setup-kern_partition.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_setup-kern_partition.d

grub_setup-partmap_amiga.o: partmap/amiga.c
	$(BUILD_CC) -Ipartmap -I$(srcdir)/partmap $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -c -o $@ $<

grub_setup-partmap_amiga.d: partmap/amiga.c
	set -e; 	  $(BUILD_CC) -Ipartmap -I$(srcdir)/partmap $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -M $< 	  | sed 's,amiga\.o[ :]*,grub_setup-partmap_amiga.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_setup-partmap_amiga.d

grub_setup-partmap_apple.o: partmap/apple.c
	$(BUILD_CC) -Ipartmap -I$(srcdir)/partmap $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -c -o $@ $<

grub_setup-partmap_apple.d: partmap/apple.c
	set -e; 	  $(BUILD_CC) -Ipartmap -I$(srcdir)/partmap $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -M $< 	  | sed 's,apple\.o[ :]*,grub_setup-partmap_apple.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_setup-partmap_apple.d

grub_setup-partmap_pc.o: partmap/pc.c
	$(BUILD_CC) -Ipartmap -I$(srcdir)/partmap $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -c -o $@ $<

grub_setup-partmap_pc.d: partmap/pc.c
	set -e; 	  $(BUILD_CC) -Ipartmap -I$(srcdir)/partmap $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -M $< 	  | sed 's,pc\.o[ :]*,grub_setup-partmap_pc.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_setup-partmap_pc.d

grub_setup-partmap_sun.o: partmap/sun.c
	$(BUILD_CC) -Ipartmap -I$(srcdir)/partmap $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -c -o $@ $<

grub_setup-partmap_sun.d: partmap/sun.c
	set -e; 	  $(BUILD_CC) -Ipartmap -I$(srcdir)/partmap $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -M $< 	  | sed 's,sun\.o[ :]*,grub_setup-partmap_sun.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_setup-partmap_sun.d

grub_setup-fs_ufs.o: fs/ufs.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -c -o $@ $<

grub_setup-fs_ufs.d: fs/ufs.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -M $< 	  | sed 's,ufs\.o[ :]*,grub_setup-fs_ufs.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_setup-fs_ufs.d

grub_setup-fs_minix.o: fs/minix.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -c -o $@ $<

grub_setup-fs_minix.d: fs/minix.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -M $< 	  | sed 's,minix\.o[ :]*,grub_setup-fs_minix.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_setup-fs_minix.d

grub_setup-fs_hfs.o: fs/hfs.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -c -o $@ $<

grub_setup-fs_hfs.d: fs/hfs.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -M $< 	  | sed 's,hfs\.o[ :]*,grub_setup-fs_hfs.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_setup-fs_hfs.d

grub_setup-fs_jfs.o: fs/jfs.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -c -o $@ $<

grub_setup-fs_jfs.d: fs/jfs.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -M $< 	  | sed 's,jfs\.o[ :]*,grub_setup-fs_jfs.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_setup-fs_jfs.d

grub_setup-kern_file.o: kern/file.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -c -o $@ $<

grub_setup-kern_file.d: kern/file.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -M $< 	  | sed 's,file\.o[ :]*,grub_setup-kern_file.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_setup-kern_file.d

grub_setup-kern_fs.o: kern/fs.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -c -o $@ $<

grub_setup-kern_fs.d: kern/fs.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -M $< 	  | sed 's,fs\.o[ :]*,grub_setup-kern_fs.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_setup-kern_fs.d

grub_setup-kern_env.o: kern/env.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -c -o $@ $<

grub_setup-kern_env.d: kern/env.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -M $< 	  | sed 's,env\.o[ :]*,grub_setup-kern_env.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_setup-kern_env.d

grub_setup-fs_fshelp.o: fs/fshelp.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -c -o $@ $<

grub_setup-fs_fshelp.d: fs/fshelp.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -M $< 	  | sed 's,fshelp\.o[ :]*,grub_setup-fs_fshelp.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_setup-fs_fshelp.d


# For grub_emu.
grub_emu_SOURCES = commands/boot.c commands/cat.c commands/cmp.c 	\
	commands/configfile.c commands/default.c commands/help.c	\
	commands/terminal.c commands/ls.c commands/timeout.c		\
	commands/i386/pc/halt.c commands/i386/pc/reboot.c		\
	disk/loopback.c							\
	fs/ext2.c fs/fat.c fs/fshelp.c fs/hfs.c fs/iso9660.c fs/jfs.c	\
	fs/minix.c fs/ufs.c						\
	kern/device.c kern/disk.c kern/dl.c kern/env.c kern/err.c 	\
	kern/file.c kern/fs.c kern/loader.c kern/main.c kern/misc.c	\
	kern/partition.c kern/rescue.c kern/term.c			\
	normal/arg.c normal/cmdline.c normal/command.c normal/context.c	\
	normal/main.c normal/menu.c normal/menu_entry.c			\
	partmap/amiga.c	partmap/apple.c partmap/pc.c partmap/sun.c	\
	util/console.c util/grub-emu.c util/misc.c			\
	util/i386/pc/biosdisk.c util/i386/pc/getroot.c			\
	util/i386/pc/misc.c
CLEANFILES += grub-emu grub_emu-commands_boot.o grub_emu-commands_cat.o grub_emu-commands_cmp.o grub_emu-commands_configfile.o grub_emu-commands_default.o grub_emu-commands_help.o grub_emu-commands_terminal.o grub_emu-commands_ls.o grub_emu-commands_timeout.o grub_emu-commands_i386_pc_halt.o grub_emu-commands_i386_pc_reboot.o grub_emu-disk_loopback.o grub_emu-fs_ext2.o grub_emu-fs_fat.o grub_emu-fs_fshelp.o grub_emu-fs_hfs.o grub_emu-fs_iso9660.o grub_emu-fs_jfs.o grub_emu-fs_minix.o grub_emu-fs_ufs.o grub_emu-kern_device.o grub_emu-kern_disk.o grub_emu-kern_dl.o grub_emu-kern_env.o grub_emu-kern_err.o grub_emu-kern_file.o grub_emu-kern_fs.o grub_emu-kern_loader.o grub_emu-kern_main.o grub_emu-kern_misc.o grub_emu-kern_partition.o grub_emu-kern_rescue.o grub_emu-kern_term.o grub_emu-normal_arg.o grub_emu-normal_cmdline.o grub_emu-normal_command.o grub_emu-normal_context.o grub_emu-normal_main.o grub_emu-normal_menu.o grub_emu-normal_menu_entry.o grub_emu-partmap_amiga.o grub_emu-partmap_apple.o grub_emu-partmap_pc.o grub_emu-partmap_sun.o grub_emu-util_console.o grub_emu-util_grub_emu.o grub_emu-util_misc.o grub_emu-util_i386_pc_biosdisk.o grub_emu-util_i386_pc_getroot.o grub_emu-util_i386_pc_misc.o
MOSTLYCLEANFILES += grub_emu-commands_boot.d grub_emu-commands_cat.d grub_emu-commands_cmp.d grub_emu-commands_configfile.d grub_emu-commands_default.d grub_emu-commands_help.d grub_emu-commands_terminal.d grub_emu-commands_ls.d grub_emu-commands_timeout.d grub_emu-commands_i386_pc_halt.d grub_emu-commands_i386_pc_reboot.d grub_emu-disk_loopback.d grub_emu-fs_ext2.d grub_emu-fs_fat.d grub_emu-fs_fshelp.d grub_emu-fs_hfs.d grub_emu-fs_iso9660.d grub_emu-fs_jfs.d grub_emu-fs_minix.d grub_emu-fs_ufs.d grub_emu-kern_device.d grub_emu-kern_disk.d grub_emu-kern_dl.d grub_emu-kern_env.d grub_emu-kern_err.d grub_emu-kern_file.d grub_emu-kern_fs.d grub_emu-kern_loader.d grub_emu-kern_main.d grub_emu-kern_misc.d grub_emu-kern_partition.d grub_emu-kern_rescue.d grub_emu-kern_term.d grub_emu-normal_arg.d grub_emu-normal_cmdline.d grub_emu-normal_command.d grub_emu-normal_context.d grub_emu-normal_main.d grub_emu-normal_menu.d grub_emu-normal_menu_entry.d grub_emu-partmap_amiga.d grub_emu-partmap_apple.d grub_emu-partmap_pc.d grub_emu-partmap_sun.d grub_emu-util_console.d grub_emu-util_grub_emu.d grub_emu-util_misc.d grub_emu-util_i386_pc_biosdisk.d grub_emu-util_i386_pc_getroot.d grub_emu-util_i386_pc_misc.d

grub-emu: grub_emu-commands_boot.o grub_emu-commands_cat.o grub_emu-commands_cmp.o grub_emu-commands_configfile.o grub_emu-commands_default.o grub_emu-commands_help.o grub_emu-commands_terminal.o grub_emu-commands_ls.o grub_emu-commands_timeout.o grub_emu-commands_i386_pc_halt.o grub_emu-commands_i386_pc_reboot.o grub_emu-disk_loopback.o grub_emu-fs_ext2.o grub_emu-fs_fat.o grub_emu-fs_fshelp.o grub_emu-fs_hfs.o grub_emu-fs_iso9660.o grub_emu-fs_jfs.o grub_emu-fs_minix.o grub_emu-fs_ufs.o grub_emu-kern_device.o grub_emu-kern_disk.o grub_emu-kern_dl.o grub_emu-kern_env.o grub_emu-kern_err.o grub_emu-kern_file.o grub_emu-kern_fs.o grub_emu-kern_loader.o grub_emu-kern_main.o grub_emu-kern_misc.o grub_emu-kern_partition.o grub_emu-kern_rescue.o grub_emu-kern_term.o grub_emu-normal_arg.o grub_emu-normal_cmdline.o grub_emu-normal_command.o grub_emu-normal_context.o grub_emu-normal_main.o grub_emu-normal_menu.o grub_emu-normal_menu_entry.o grub_emu-partmap_amiga.o grub_emu-partmap_apple.o grub_emu-partmap_pc.o grub_emu-partmap_sun.o grub_emu-util_console.o grub_emu-util_grub_emu.o grub_emu-util_misc.o grub_emu-util_i386_pc_biosdisk.o grub_emu-util_i386_pc_getroot.o grub_emu-util_i386_pc_misc.o
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

grub_emu-commands_default.o: commands/default.c
	$(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-commands_default.d: commands/default.c
	set -e; 	  $(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,default\.o[ :]*,grub_emu-commands_default.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-commands_default.d

grub_emu-commands_help.o: commands/help.c
	$(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-commands_help.d: commands/help.c
	set -e; 	  $(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,help\.o[ :]*,grub_emu-commands_help.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-commands_help.d

grub_emu-commands_terminal.o: commands/terminal.c
	$(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-commands_terminal.d: commands/terminal.c
	set -e; 	  $(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,terminal\.o[ :]*,grub_emu-commands_terminal.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-commands_terminal.d

grub_emu-commands_ls.o: commands/ls.c
	$(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-commands_ls.d: commands/ls.c
	set -e; 	  $(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,ls\.o[ :]*,grub_emu-commands_ls.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-commands_ls.d

grub_emu-commands_timeout.o: commands/timeout.c
	$(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-commands_timeout.d: commands/timeout.c
	set -e; 	  $(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,timeout\.o[ :]*,grub_emu-commands_timeout.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-commands_timeout.d

grub_emu-commands_i386_pc_halt.o: commands/i386/pc/halt.c
	$(BUILD_CC) -Icommands/i386/pc -I$(srcdir)/commands/i386/pc $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-commands_i386_pc_halt.d: commands/i386/pc/halt.c
	set -e; 	  $(BUILD_CC) -Icommands/i386/pc -I$(srcdir)/commands/i386/pc $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,halt\.o[ :]*,grub_emu-commands_i386_pc_halt.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-commands_i386_pc_halt.d

grub_emu-commands_i386_pc_reboot.o: commands/i386/pc/reboot.c
	$(BUILD_CC) -Icommands/i386/pc -I$(srcdir)/commands/i386/pc $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-commands_i386_pc_reboot.d: commands/i386/pc/reboot.c
	set -e; 	  $(BUILD_CC) -Icommands/i386/pc -I$(srcdir)/commands/i386/pc $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,reboot\.o[ :]*,grub_emu-commands_i386_pc_reboot.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-commands_i386_pc_reboot.d

grub_emu-disk_loopback.o: disk/loopback.c
	$(BUILD_CC) -Idisk -I$(srcdir)/disk $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-disk_loopback.d: disk/loopback.c
	set -e; 	  $(BUILD_CC) -Idisk -I$(srcdir)/disk $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,loopback\.o[ :]*,grub_emu-disk_loopback.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-disk_loopback.d

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

grub_emu-fs_ufs.o: fs/ufs.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-fs_ufs.d: fs/ufs.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,ufs\.o[ :]*,grub_emu-fs_ufs.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-fs_ufs.d

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

grub_emu-normal_context.o: normal/context.c
	$(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-normal_context.d: normal/context.c
	set -e; 	  $(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,context\.o[ :]*,grub_emu-normal_context.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-normal_context.d

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

grub_emu-util_i386_pc_misc.o: util/i386/pc/misc.c
	$(BUILD_CC) -Iutil/i386/pc -I$(srcdir)/util/i386/pc $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-util_i386_pc_misc.d: util/i386/pc/misc.c
	set -e; 	  $(BUILD_CC) -Iutil/i386/pc -I$(srcdir)/util/i386/pc $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,misc\.o[ :]*,grub_emu-util_i386_pc_misc.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-util_i386_pc_misc.d


grub_emu_LDFLAGS = -lncurses

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
pkgdata_MODULES = _chain.mod _linux.mod linux.mod fat.mod ufs.mod	\
	ext2.mod minix.mod hfs.mod jfs.mod normal.mod hello.mod vga.mod	\
	font.mod _multiboot.mod ls.mod boot.mod cmp.mod cat.mod		\
	terminal.mod fshelp.mod chain.mod multiboot.mod amiga.mod	\
	apple.mod pc.mod sun.mod loopback.mod reboot.mod halt.mod	\
	help.mod default.mod timeout.mod configfile.mod

# For _chain.mod.
_chain_mod_SOURCES = loader/i386/pc/chainloader.c
CLEANFILES += _chain.mod mod-_chain.o mod-_chain.c pre-_chain.o _chain_mod-loader_i386_pc_chainloader.o def-_chain.lst und-_chain.lst
MOSTLYCLEANFILES += _chain_mod-loader_i386_pc_chainloader.d
DEFSYMFILES += def-_chain.lst
UNDSYMFILES += und-_chain.lst

_chain.mod: pre-_chain.o mod-_chain.o
	-rm -f $@
	$(LD) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-_chain.o: _chain_mod-loader_i386_pc_chainloader.o
	-rm -f $@
	$(LD) -r -d -o $@ $^

mod-_chain.o: mod-_chain.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(_chain_mod_CFLAGS) -c -o $@ $<

mod-_chain.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh '_chain' $< > $@ || (rm -f $@; exit 1)

def-_chain.lst: pre-_chain.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 _chain/' > $@

und-_chain.lst: pre-_chain.o
	echo '_chain' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

_chain_mod-loader_i386_pc_chainloader.o: loader/i386/pc/chainloader.c
	$(CC) -Iloader/i386/pc -I$(srcdir)/loader/i386/pc $(CPPFLAGS) $(CFLAGS) $(_chain_mod_CFLAGS) -c -o $@ $<

_chain_mod-loader_i386_pc_chainloader.d: loader/i386/pc/chainloader.c
	set -e; 	  $(CC) -Iloader/i386/pc -I$(srcdir)/loader/i386/pc $(CPPFLAGS) $(CFLAGS) $(_chain_mod_CFLAGS) -M $< 	  | sed 's,chainloader\.o[ :]*,_chain_mod-loader_i386_pc_chainloader.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include _chain_mod-loader_i386_pc_chainloader.d

_chain_mod_CFLAGS = $(COMMON_CFLAGS)

# For chain.mod.
chain_mod_SOURCES = loader/i386/pc/chainloader_normal.c
CLEANFILES += chain.mod mod-chain.o mod-chain.c pre-chain.o chain_mod-loader_i386_pc_chainloader_normal.o def-chain.lst und-chain.lst
MOSTLYCLEANFILES += chain_mod-loader_i386_pc_chainloader_normal.d
DEFSYMFILES += def-chain.lst
UNDSYMFILES += und-chain.lst

chain.mod: pre-chain.o mod-chain.o
	-rm -f $@
	$(LD) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-chain.o: chain_mod-loader_i386_pc_chainloader_normal.o
	-rm -f $@
	$(LD) -r -d -o $@ $^

mod-chain.o: mod-chain.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(chain_mod_CFLAGS) -c -o $@ $<

mod-chain.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'chain' $< > $@ || (rm -f $@; exit 1)

def-chain.lst: pre-chain.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 chain/' > $@

und-chain.lst: pre-chain.o
	echo 'chain' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

chain_mod-loader_i386_pc_chainloader_normal.o: loader/i386/pc/chainloader_normal.c
	$(CC) -Iloader/i386/pc -I$(srcdir)/loader/i386/pc $(CPPFLAGS) $(CFLAGS) $(chain_mod_CFLAGS) -c -o $@ $<

chain_mod-loader_i386_pc_chainloader_normal.d: loader/i386/pc/chainloader_normal.c
	set -e; 	  $(CC) -Iloader/i386/pc -I$(srcdir)/loader/i386/pc $(CPPFLAGS) $(CFLAGS) $(chain_mod_CFLAGS) -M $< 	  | sed 's,chainloader_normal\.o[ :]*,chain_mod-loader_i386_pc_chainloader_normal.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include chain_mod-loader_i386_pc_chainloader_normal.d

chain_mod_CFLAGS = $(COMMON_CFLAGS)

# For fshelp.mod.
fshelp_mod_SOURCES = fs/fshelp.c
CLEANFILES += fshelp.mod mod-fshelp.o mod-fshelp.c pre-fshelp.o fshelp_mod-fs_fshelp.o def-fshelp.lst und-fshelp.lst
MOSTLYCLEANFILES += fshelp_mod-fs_fshelp.d
DEFSYMFILES += def-fshelp.lst
UNDSYMFILES += und-fshelp.lst

fshelp.mod: pre-fshelp.o mod-fshelp.o
	-rm -f $@
	$(LD) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-fshelp.o: fshelp_mod-fs_fshelp.o
	-rm -f $@
	$(LD) -r -d -o $@ $^

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

fshelp_mod_CFLAGS = $(COMMON_CFLAGS)

# For fat.mod.
fat_mod_SOURCES = fs/fat.c
CLEANFILES += fat.mod mod-fat.o mod-fat.c pre-fat.o fat_mod-fs_fat.o def-fat.lst und-fat.lst
MOSTLYCLEANFILES += fat_mod-fs_fat.d
DEFSYMFILES += def-fat.lst
UNDSYMFILES += und-fat.lst

fat.mod: pre-fat.o mod-fat.o
	-rm -f $@
	$(LD) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-fat.o: fat_mod-fs_fat.o
	-rm -f $@
	$(LD) -r -d -o $@ $^

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

fat_mod_CFLAGS = $(COMMON_CFLAGS)

# For ext2.mod.
ext2_mod_SOURCES = fs/ext2.c
CLEANFILES += ext2.mod mod-ext2.o mod-ext2.c pre-ext2.o ext2_mod-fs_ext2.o def-ext2.lst und-ext2.lst
MOSTLYCLEANFILES += ext2_mod-fs_ext2.d
DEFSYMFILES += def-ext2.lst
UNDSYMFILES += und-ext2.lst

ext2.mod: pre-ext2.o mod-ext2.o
	-rm -f $@
	$(LD) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-ext2.o: ext2_mod-fs_ext2.o
	-rm -f $@
	$(LD) -r -d -o $@ $^

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

ext2_mod_CFLAGS = $(COMMON_CFLAGS)

# For ufs.mod.
ufs_mod_SOURCES = fs/ufs.c
CLEANFILES += ufs.mod mod-ufs.o mod-ufs.c pre-ufs.o ufs_mod-fs_ufs.o def-ufs.lst und-ufs.lst
MOSTLYCLEANFILES += ufs_mod-fs_ufs.d
DEFSYMFILES += def-ufs.lst
UNDSYMFILES += und-ufs.lst

ufs.mod: pre-ufs.o mod-ufs.o
	-rm -f $@
	$(LD) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-ufs.o: ufs_mod-fs_ufs.o
	-rm -f $@
	$(LD) -r -d -o $@ $^

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

ufs_mod_CFLAGS = $(COMMON_CFLAGS)

# For minix.mod.
minix_mod_SOURCES = fs/minix.c
CLEANFILES += minix.mod mod-minix.o mod-minix.c pre-minix.o minix_mod-fs_minix.o def-minix.lst und-minix.lst
MOSTLYCLEANFILES += minix_mod-fs_minix.d
DEFSYMFILES += def-minix.lst
UNDSYMFILES += und-minix.lst

minix.mod: pre-minix.o mod-minix.o
	-rm -f $@
	$(LD) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-minix.o: minix_mod-fs_minix.o
	-rm -f $@
	$(LD) -r -d -o $@ $^

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

minix_mod_CFLAGS = $(COMMON_CFLAGS)

# For hfs.mod.
hfs_mod_SOURCES = fs/hfs.c
CLEANFILES += hfs.mod mod-hfs.o mod-hfs.c pre-hfs.o hfs_mod-fs_hfs.o def-hfs.lst und-hfs.lst
MOSTLYCLEANFILES += hfs_mod-fs_hfs.d
DEFSYMFILES += def-hfs.lst
UNDSYMFILES += und-hfs.lst

hfs.mod: pre-hfs.o mod-hfs.o
	-rm -f $@
	$(LD) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-hfs.o: hfs_mod-fs_hfs.o
	-rm -f $@
	$(LD) -r -d -o $@ $^

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

hfs_mod_CFLAGS = $(COMMON_CFLAGS)

# For jfs.mod.
jfs_mod_SOURCES = fs/jfs.c
CLEANFILES += jfs.mod mod-jfs.o mod-jfs.c pre-jfs.o jfs_mod-fs_jfs.o def-jfs.lst und-jfs.lst
MOSTLYCLEANFILES += jfs_mod-fs_jfs.d
DEFSYMFILES += def-jfs.lst
UNDSYMFILES += und-jfs.lst

jfs.mod: pre-jfs.o mod-jfs.o
	-rm -f $@
	$(LD) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-jfs.o: jfs_mod-fs_jfs.o
	-rm -f $@
	$(LD) -r -d -o $@ $^

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

jfs_mod_CFLAGS = $(COMMON_CFLAGS)

# For iso9660.mod.
iso9660_mod_SOURCES = fs/iso9660.c
iso9660_mod_CFLAGS = $(COMMON_CFLAGS)

# For _linux.mod.
_linux_mod_SOURCES = loader/i386/pc/linux.c
CLEANFILES += _linux.mod mod-_linux.o mod-_linux.c pre-_linux.o _linux_mod-loader_i386_pc_linux.o def-_linux.lst und-_linux.lst
MOSTLYCLEANFILES += _linux_mod-loader_i386_pc_linux.d
DEFSYMFILES += def-_linux.lst
UNDSYMFILES += und-_linux.lst

_linux.mod: pre-_linux.o mod-_linux.o
	-rm -f $@
	$(LD) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-_linux.o: _linux_mod-loader_i386_pc_linux.o
	-rm -f $@
	$(LD) -r -d -o $@ $^

mod-_linux.o: mod-_linux.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(_linux_mod_CFLAGS) -c -o $@ $<

mod-_linux.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh '_linux' $< > $@ || (rm -f $@; exit 1)

def-_linux.lst: pre-_linux.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 _linux/' > $@

und-_linux.lst: pre-_linux.o
	echo '_linux' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

_linux_mod-loader_i386_pc_linux.o: loader/i386/pc/linux.c
	$(CC) -Iloader/i386/pc -I$(srcdir)/loader/i386/pc $(CPPFLAGS) $(CFLAGS) $(_linux_mod_CFLAGS) -c -o $@ $<

_linux_mod-loader_i386_pc_linux.d: loader/i386/pc/linux.c
	set -e; 	  $(CC) -Iloader/i386/pc -I$(srcdir)/loader/i386/pc $(CPPFLAGS) $(CFLAGS) $(_linux_mod_CFLAGS) -M $< 	  | sed 's,linux\.o[ :]*,_linux_mod-loader_i386_pc_linux.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include _linux_mod-loader_i386_pc_linux.d

_linux_mod_CFLAGS = $(COMMON_CFLAGS)
 
# For linux.mod.
linux_mod_SOURCES = loader/i386/pc/linux_normal.c
CLEANFILES += linux.mod mod-linux.o mod-linux.c pre-linux.o linux_mod-loader_i386_pc_linux_normal.o def-linux.lst und-linux.lst
MOSTLYCLEANFILES += linux_mod-loader_i386_pc_linux_normal.d
DEFSYMFILES += def-linux.lst
UNDSYMFILES += und-linux.lst

linux.mod: pre-linux.o mod-linux.o
	-rm -f $@
	$(LD) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-linux.o: linux_mod-loader_i386_pc_linux_normal.o
	-rm -f $@
	$(LD) -r -d -o $@ $^

mod-linux.o: mod-linux.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(linux_mod_CFLAGS) -c -o $@ $<

mod-linux.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'linux' $< > $@ || (rm -f $@; exit 1)

def-linux.lst: pre-linux.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 linux/' > $@

und-linux.lst: pre-linux.o
	echo 'linux' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

linux_mod-loader_i386_pc_linux_normal.o: loader/i386/pc/linux_normal.c
	$(CC) -Iloader/i386/pc -I$(srcdir)/loader/i386/pc $(CPPFLAGS) $(CFLAGS) $(linux_mod_CFLAGS) -c -o $@ $<

linux_mod-loader_i386_pc_linux_normal.d: loader/i386/pc/linux_normal.c
	set -e; 	  $(CC) -Iloader/i386/pc -I$(srcdir)/loader/i386/pc $(CPPFLAGS) $(CFLAGS) $(linux_mod_CFLAGS) -M $< 	  | sed 's,linux_normal\.o[ :]*,linux_mod-loader_i386_pc_linux_normal.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include linux_mod-loader_i386_pc_linux_normal.d

linux_mod_CFLAGS = $(COMMON_CFLAGS)

# For normal.mod.
normal_mod_SOURCES = normal/arg.c normal/cmdline.c normal/command.c	\
	normal/context.c normal/main.c normal/menu.c 			\
	normal/menu_entry.c normal/i386/setjmp.S
CLEANFILES += normal.mod mod-normal.o mod-normal.c pre-normal.o normal_mod-normal_arg.o normal_mod-normal_cmdline.o normal_mod-normal_command.o normal_mod-normal_context.o normal_mod-normal_main.o normal_mod-normal_menu.o normal_mod-normal_menu_entry.o normal_mod-normal_i386_setjmp.o def-normal.lst und-normal.lst
MOSTLYCLEANFILES += normal_mod-normal_arg.d normal_mod-normal_cmdline.d normal_mod-normal_command.d normal_mod-normal_context.d normal_mod-normal_main.d normal_mod-normal_menu.d normal_mod-normal_menu_entry.d normal_mod-normal_i386_setjmp.d
DEFSYMFILES += def-normal.lst
UNDSYMFILES += und-normal.lst

normal.mod: pre-normal.o mod-normal.o
	-rm -f $@
	$(LD) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-normal.o: normal_mod-normal_arg.o normal_mod-normal_cmdline.o normal_mod-normal_command.o normal_mod-normal_context.o normal_mod-normal_main.o normal_mod-normal_menu.o normal_mod-normal_menu_entry.o normal_mod-normal_i386_setjmp.o
	-rm -f $@
	$(LD) -r -d -o $@ $^

mod-normal.o: mod-normal.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -c -o $@ $<

mod-normal.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'normal' $< > $@ || (rm -f $@; exit 1)

def-normal.lst: pre-normal.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 normal/' > $@

und-normal.lst: pre-normal.o
	echo 'normal' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

normal_mod-normal_arg.o: normal/arg.c
	$(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -c -o $@ $<

normal_mod-normal_arg.d: normal/arg.c
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -M $< 	  | sed 's,arg\.o[ :]*,normal_mod-normal_arg.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include normal_mod-normal_arg.d

normal_mod-normal_cmdline.o: normal/cmdline.c
	$(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -c -o $@ $<

normal_mod-normal_cmdline.d: normal/cmdline.c
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -M $< 	  | sed 's,cmdline\.o[ :]*,normal_mod-normal_cmdline.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include normal_mod-normal_cmdline.d

normal_mod-normal_command.o: normal/command.c
	$(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -c -o $@ $<

normal_mod-normal_command.d: normal/command.c
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -M $< 	  | sed 's,command\.o[ :]*,normal_mod-normal_command.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include normal_mod-normal_command.d

normal_mod-normal_context.o: normal/context.c
	$(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -c -o $@ $<

normal_mod-normal_context.d: normal/context.c
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -M $< 	  | sed 's,context\.o[ :]*,normal_mod-normal_context.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include normal_mod-normal_context.d

normal_mod-normal_main.o: normal/main.c
	$(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -c -o $@ $<

normal_mod-normal_main.d: normal/main.c
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -M $< 	  | sed 's,main\.o[ :]*,normal_mod-normal_main.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include normal_mod-normal_main.d

normal_mod-normal_menu.o: normal/menu.c
	$(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -c -o $@ $<

normal_mod-normal_menu.d: normal/menu.c
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -M $< 	  | sed 's,menu\.o[ :]*,normal_mod-normal_menu.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include normal_mod-normal_menu.d

normal_mod-normal_menu_entry.o: normal/menu_entry.c
	$(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -c -o $@ $<

normal_mod-normal_menu_entry.d: normal/menu_entry.c
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -M $< 	  | sed 's,menu_entry\.o[ :]*,normal_mod-normal_menu_entry.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include normal_mod-normal_menu_entry.d

normal_mod-normal_i386_setjmp.o: normal/i386/setjmp.S
	$(CC) -Inormal/i386 -I$(srcdir)/normal/i386 $(CPPFLAGS) $(ASFLAGS) $(normal_mod_ASFLAGS) -c -o $@ $<

normal_mod-normal_i386_setjmp.d: normal/i386/setjmp.S
	set -e; 	  $(CC) -Inormal/i386 -I$(srcdir)/normal/i386 $(CPPFLAGS) $(ASFLAGS) $(normal_mod_ASFLAGS) -M $< 	  | sed 's,setjmp\.o[ :]*,normal_mod-normal_i386_setjmp.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include normal_mod-normal_i386_setjmp.d

normal_mod_CFLAGS = $(COMMON_CFLAGS)
normal_mod_ASFLAGS = $(COMMON_ASFLAGS)

# For hello.mod.
hello_mod_SOURCES = hello/hello.c
CLEANFILES += hello.mod mod-hello.o mod-hello.c pre-hello.o hello_mod-hello_hello.o def-hello.lst und-hello.lst
MOSTLYCLEANFILES += hello_mod-hello_hello.d
DEFSYMFILES += def-hello.lst
UNDSYMFILES += und-hello.lst

hello.mod: pre-hello.o mod-hello.o
	-rm -f $@
	$(LD) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-hello.o: hello_mod-hello_hello.o
	-rm -f $@
	$(LD) -r -d -o $@ $^

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

hello_mod_CFLAGS = $(COMMON_CFLAGS)

# For boot.mod.
boot_mod_SOURCES = commands/boot.c
CLEANFILES += boot.mod mod-boot.o mod-boot.c pre-boot.o boot_mod-commands_boot.o def-boot.lst und-boot.lst
MOSTLYCLEANFILES += boot_mod-commands_boot.d
DEFSYMFILES += def-boot.lst
UNDSYMFILES += und-boot.lst

boot.mod: pre-boot.o mod-boot.o
	-rm -f $@
	$(LD) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-boot.o: boot_mod-commands_boot.o
	-rm -f $@
	$(LD) -r -d -o $@ $^

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

boot_mod_CFLAGS = $(COMMON_CFLAGS)

# For terminal.mod.
terminal_mod_SOURCES = commands/terminal.c
CLEANFILES += terminal.mod mod-terminal.o mod-terminal.c pre-terminal.o terminal_mod-commands_terminal.o def-terminal.lst und-terminal.lst
MOSTLYCLEANFILES += terminal_mod-commands_terminal.d
DEFSYMFILES += def-terminal.lst
UNDSYMFILES += und-terminal.lst

terminal.mod: pre-terminal.o mod-terminal.o
	-rm -f $@
	$(LD) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-terminal.o: terminal_mod-commands_terminal.o
	-rm -f $@
	$(LD) -r -d -o $@ $^

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

terminal_mod_CFLAGS = $(COMMON_CFLAGS)

# For ls.mod.
ls_mod_SOURCES = commands/ls.c
CLEANFILES += ls.mod mod-ls.o mod-ls.c pre-ls.o ls_mod-commands_ls.o def-ls.lst und-ls.lst
MOSTLYCLEANFILES += ls_mod-commands_ls.d
DEFSYMFILES += def-ls.lst
UNDSYMFILES += und-ls.lst

ls.mod: pre-ls.o mod-ls.o
	-rm -f $@
	$(LD) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-ls.o: ls_mod-commands_ls.o
	-rm -f $@
	$(LD) -r -d -o $@ $^

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

ls_mod_CFLAGS = $(COMMON_CFLAGS)

# For cmp.mod.
cmp_mod_SOURCES = commands/cmp.c
CLEANFILES += cmp.mod mod-cmp.o mod-cmp.c pre-cmp.o cmp_mod-commands_cmp.o def-cmp.lst und-cmp.lst
MOSTLYCLEANFILES += cmp_mod-commands_cmp.d
DEFSYMFILES += def-cmp.lst
UNDSYMFILES += und-cmp.lst

cmp.mod: pre-cmp.o mod-cmp.o
	-rm -f $@
	$(LD) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-cmp.o: cmp_mod-commands_cmp.o
	-rm -f $@
	$(LD) -r -d -o $@ $^

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

cmp_mod_CFLAGS = $(COMMON_CFLAGS)

# For cat.mod.
cat_mod_SOURCES = commands/cat.c
CLEANFILES += cat.mod mod-cat.o mod-cat.c pre-cat.o cat_mod-commands_cat.o def-cat.lst und-cat.lst
MOSTLYCLEANFILES += cat_mod-commands_cat.d
DEFSYMFILES += def-cat.lst
UNDSYMFILES += und-cat.lst

cat.mod: pre-cat.o mod-cat.o
	-rm -f $@
	$(LD) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-cat.o: cat_mod-commands_cat.o
	-rm -f $@
	$(LD) -r -d -o $@ $^

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

cat_mod_CFLAGS = $(COMMON_CFLAGS)

# For help.mod.
help_mod_SOURCES = commands/help.c
CLEANFILES += help.mod mod-help.o mod-help.c pre-help.o help_mod-commands_help.o def-help.lst und-help.lst
MOSTLYCLEANFILES += help_mod-commands_help.d
DEFSYMFILES += def-help.lst
UNDSYMFILES += und-help.lst

help.mod: pre-help.o mod-help.o
	-rm -f $@
	$(LD) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-help.o: help_mod-commands_help.o
	-rm -f $@
	$(LD) -r -d -o $@ $^

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

help_mod_CFLAGS = $(COMMON_CFLAGS)

# For reboot.mod.
reboot_mod_SOURCES = commands/i386/pc/reboot.c
CLEANFILES += reboot.mod mod-reboot.o mod-reboot.c pre-reboot.o reboot_mod-commands_i386_pc_reboot.o def-reboot.lst und-reboot.lst
MOSTLYCLEANFILES += reboot_mod-commands_i386_pc_reboot.d
DEFSYMFILES += def-reboot.lst
UNDSYMFILES += und-reboot.lst

reboot.mod: pre-reboot.o mod-reboot.o
	-rm -f $@
	$(LD) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-reboot.o: reboot_mod-commands_i386_pc_reboot.o
	-rm -f $@
	$(LD) -r -d -o $@ $^

mod-reboot.o: mod-reboot.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(reboot_mod_CFLAGS) -c -o $@ $<

mod-reboot.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'reboot' $< > $@ || (rm -f $@; exit 1)

def-reboot.lst: pre-reboot.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 reboot/' > $@

und-reboot.lst: pre-reboot.o
	echo 'reboot' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

reboot_mod-commands_i386_pc_reboot.o: commands/i386/pc/reboot.c
	$(CC) -Icommands/i386/pc -I$(srcdir)/commands/i386/pc $(CPPFLAGS) $(CFLAGS) $(reboot_mod_CFLAGS) -c -o $@ $<

reboot_mod-commands_i386_pc_reboot.d: commands/i386/pc/reboot.c
	set -e; 	  $(CC) -Icommands/i386/pc -I$(srcdir)/commands/i386/pc $(CPPFLAGS) $(CFLAGS) $(reboot_mod_CFLAGS) -M $< 	  | sed 's,reboot\.o[ :]*,reboot_mod-commands_i386_pc_reboot.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include reboot_mod-commands_i386_pc_reboot.d

reboot_mod_CFLAGS = $(COMMON_CFLAGS)

# For halt.mod.
halt_mod_SOURCES = commands/i386/pc/halt.c
CLEANFILES += halt.mod mod-halt.o mod-halt.c pre-halt.o halt_mod-commands_i386_pc_halt.o def-halt.lst und-halt.lst
MOSTLYCLEANFILES += halt_mod-commands_i386_pc_halt.d
DEFSYMFILES += def-halt.lst
UNDSYMFILES += und-halt.lst

halt.mod: pre-halt.o mod-halt.o
	-rm -f $@
	$(LD) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-halt.o: halt_mod-commands_i386_pc_halt.o
	-rm -f $@
	$(LD) -r -d -o $@ $^

mod-halt.o: mod-halt.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(halt_mod_CFLAGS) -c -o $@ $<

mod-halt.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'halt' $< > $@ || (rm -f $@; exit 1)

def-halt.lst: pre-halt.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 halt/' > $@

und-halt.lst: pre-halt.o
	echo 'halt' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

halt_mod-commands_i386_pc_halt.o: commands/i386/pc/halt.c
	$(CC) -Icommands/i386/pc -I$(srcdir)/commands/i386/pc $(CPPFLAGS) $(CFLAGS) $(halt_mod_CFLAGS) -c -o $@ $<

halt_mod-commands_i386_pc_halt.d: commands/i386/pc/halt.c
	set -e; 	  $(CC) -Icommands/i386/pc -I$(srcdir)/commands/i386/pc $(CPPFLAGS) $(CFLAGS) $(halt_mod_CFLAGS) -M $< 	  | sed 's,halt\.o[ :]*,halt_mod-commands_i386_pc_halt.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include halt_mod-commands_i386_pc_halt.d

halt_mod_CFLAGS = $(COMMON_CFLAGS)

# For vga.mod.
vga_mod_SOURCES = term/i386/pc/vga.c
CLEANFILES += vga.mod mod-vga.o mod-vga.c pre-vga.o vga_mod-term_i386_pc_vga.o def-vga.lst und-vga.lst
MOSTLYCLEANFILES += vga_mod-term_i386_pc_vga.d
DEFSYMFILES += def-vga.lst
UNDSYMFILES += und-vga.lst

vga.mod: pre-vga.o mod-vga.o
	-rm -f $@
	$(LD) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-vga.o: vga_mod-term_i386_pc_vga.o
	-rm -f $@
	$(LD) -r -d -o $@ $^

mod-vga.o: mod-vga.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(vga_mod_CFLAGS) -c -o $@ $<

mod-vga.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'vga' $< > $@ || (rm -f $@; exit 1)

def-vga.lst: pre-vga.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 vga/' > $@

und-vga.lst: pre-vga.o
	echo 'vga' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

vga_mod-term_i386_pc_vga.o: term/i386/pc/vga.c
	$(CC) -Iterm/i386/pc -I$(srcdir)/term/i386/pc $(CPPFLAGS) $(CFLAGS) $(vga_mod_CFLAGS) -c -o $@ $<

vga_mod-term_i386_pc_vga.d: term/i386/pc/vga.c
	set -e; 	  $(CC) -Iterm/i386/pc -I$(srcdir)/term/i386/pc $(CPPFLAGS) $(CFLAGS) $(vga_mod_CFLAGS) -M $< 	  | sed 's,vga\.o[ :]*,vga_mod-term_i386_pc_vga.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include vga_mod-term_i386_pc_vga.d

vga_mod_CFLAGS = $(COMMON_CFLAGS)

# For font.mod.
font_mod_SOURCES = font/manager.c
CLEANFILES += font.mod mod-font.o mod-font.c pre-font.o font_mod-font_manager.o def-font.lst und-font.lst
MOSTLYCLEANFILES += font_mod-font_manager.d
DEFSYMFILES += def-font.lst
UNDSYMFILES += und-font.lst

font.mod: pre-font.o mod-font.o
	-rm -f $@
	$(LD) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-font.o: font_mod-font_manager.o
	-rm -f $@
	$(LD) -r -d -o $@ $^

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

font_mod_CFLAGS = $(COMMON_CFLAGS)

# For _multiboot.mod.
_multiboot_mod_SOURCES = loader/i386/pc/multiboot.c
CLEANFILES += _multiboot.mod mod-_multiboot.o mod-_multiboot.c pre-_multiboot.o _multiboot_mod-loader_i386_pc_multiboot.o def-_multiboot.lst und-_multiboot.lst
MOSTLYCLEANFILES += _multiboot_mod-loader_i386_pc_multiboot.d
DEFSYMFILES += def-_multiboot.lst
UNDSYMFILES += und-_multiboot.lst

_multiboot.mod: pre-_multiboot.o mod-_multiboot.o
	-rm -f $@
	$(LD) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-_multiboot.o: _multiboot_mod-loader_i386_pc_multiboot.o
	-rm -f $@
	$(LD) -r -d -o $@ $^

mod-_multiboot.o: mod-_multiboot.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(_multiboot_mod_CFLAGS) -c -o $@ $<

mod-_multiboot.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh '_multiboot' $< > $@ || (rm -f $@; exit 1)

def-_multiboot.lst: pre-_multiboot.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 _multiboot/' > $@

und-_multiboot.lst: pre-_multiboot.o
	echo '_multiboot' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

_multiboot_mod-loader_i386_pc_multiboot.o: loader/i386/pc/multiboot.c
	$(CC) -Iloader/i386/pc -I$(srcdir)/loader/i386/pc $(CPPFLAGS) $(CFLAGS) $(_multiboot_mod_CFLAGS) -c -o $@ $<

_multiboot_mod-loader_i386_pc_multiboot.d: loader/i386/pc/multiboot.c
	set -e; 	  $(CC) -Iloader/i386/pc -I$(srcdir)/loader/i386/pc $(CPPFLAGS) $(CFLAGS) $(_multiboot_mod_CFLAGS) -M $< 	  | sed 's,multiboot\.o[ :]*,_multiboot_mod-loader_i386_pc_multiboot.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include _multiboot_mod-loader_i386_pc_multiboot.d

_multiboot_mod_CFLAGS = $(COMMON_CFLAGS)

# For multiboot.mod.
multiboot_mod_SOURCES = loader/i386/pc/multiboot_normal.c
CLEANFILES += multiboot.mod mod-multiboot.o mod-multiboot.c pre-multiboot.o multiboot_mod-loader_i386_pc_multiboot_normal.o def-multiboot.lst und-multiboot.lst
MOSTLYCLEANFILES += multiboot_mod-loader_i386_pc_multiboot_normal.d
DEFSYMFILES += def-multiboot.lst
UNDSYMFILES += und-multiboot.lst

multiboot.mod: pre-multiboot.o mod-multiboot.o
	-rm -f $@
	$(LD) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-multiboot.o: multiboot_mod-loader_i386_pc_multiboot_normal.o
	-rm -f $@
	$(LD) -r -d -o $@ $^

mod-multiboot.o: mod-multiboot.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(multiboot_mod_CFLAGS) -c -o $@ $<

mod-multiboot.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'multiboot' $< > $@ || (rm -f $@; exit 1)

def-multiboot.lst: pre-multiboot.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 multiboot/' > $@

und-multiboot.lst: pre-multiboot.o
	echo 'multiboot' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

multiboot_mod-loader_i386_pc_multiboot_normal.o: loader/i386/pc/multiboot_normal.c
	$(CC) -Iloader/i386/pc -I$(srcdir)/loader/i386/pc $(CPPFLAGS) $(CFLAGS) $(multiboot_mod_CFLAGS) -c -o $@ $<

multiboot_mod-loader_i386_pc_multiboot_normal.d: loader/i386/pc/multiboot_normal.c
	set -e; 	  $(CC) -Iloader/i386/pc -I$(srcdir)/loader/i386/pc $(CPPFLAGS) $(CFLAGS) $(multiboot_mod_CFLAGS) -M $< 	  | sed 's,multiboot_normal\.o[ :]*,multiboot_mod-loader_i386_pc_multiboot_normal.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include multiboot_mod-loader_i386_pc_multiboot_normal.d

multiboot_mod_CFLAGS = $(COMMON_CFLAGS)

# For amiga.mod
amiga_mod_SOURCES = partmap/amiga.c
CLEANFILES += amiga.mod mod-amiga.o mod-amiga.c pre-amiga.o amiga_mod-partmap_amiga.o def-amiga.lst und-amiga.lst
MOSTLYCLEANFILES += amiga_mod-partmap_amiga.d
DEFSYMFILES += def-amiga.lst
UNDSYMFILES += und-amiga.lst

amiga.mod: pre-amiga.o mod-amiga.o
	-rm -f $@
	$(LD) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-amiga.o: amiga_mod-partmap_amiga.o
	-rm -f $@
	$(LD) -r -d -o $@ $^

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

amiga_mod_CFLAGS = $(COMMON_CFLAGS)

# For apple.mod
apple_mod_SOURCES = partmap/apple.c
CLEANFILES += apple.mod mod-apple.o mod-apple.c pre-apple.o apple_mod-partmap_apple.o def-apple.lst und-apple.lst
MOSTLYCLEANFILES += apple_mod-partmap_apple.d
DEFSYMFILES += def-apple.lst
UNDSYMFILES += und-apple.lst

apple.mod: pre-apple.o mod-apple.o
	-rm -f $@
	$(LD) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-apple.o: apple_mod-partmap_apple.o
	-rm -f $@
	$(LD) -r -d -o $@ $^

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

apple_mod_CFLAGS = $(COMMON_CFLAGS)

# For pc.mod
pc_mod_SOURCES = partmap/pc.c
CLEANFILES += pc.mod mod-pc.o mod-pc.c pre-pc.o pc_mod-partmap_pc.o def-pc.lst und-pc.lst
MOSTLYCLEANFILES += pc_mod-partmap_pc.d
DEFSYMFILES += def-pc.lst
UNDSYMFILES += und-pc.lst

pc.mod: pre-pc.o mod-pc.o
	-rm -f $@
	$(LD) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-pc.o: pc_mod-partmap_pc.o
	-rm -f $@
	$(LD) -r -d -o $@ $^

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

pc_mod_CFLAGS = $(COMMON_CFLAGS)

# For sun.mod
sun_mod_SOURCES = partmap/sun.c
CLEANFILES += sun.mod mod-sun.o mod-sun.c pre-sun.o sun_mod-partmap_sun.o def-sun.lst und-sun.lst
MOSTLYCLEANFILES += sun_mod-partmap_sun.d
DEFSYMFILES += def-sun.lst
UNDSYMFILES += und-sun.lst

sun.mod: pre-sun.o mod-sun.o
	-rm -f $@
	$(LD) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-sun.o: sun_mod-partmap_sun.o
	-rm -f $@
	$(LD) -r -d -o $@ $^

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

sun_mod_CFLAGS = $(COMMON_CFLAGS)

# For loopback.mod
loopback_mod_SOURCES = disk/loopback.c
CLEANFILES += loopback.mod mod-loopback.o mod-loopback.c pre-loopback.o loopback_mod-disk_loopback.o def-loopback.lst und-loopback.lst
MOSTLYCLEANFILES += loopback_mod-disk_loopback.d
DEFSYMFILES += def-loopback.lst
UNDSYMFILES += und-loopback.lst

loopback.mod: pre-loopback.o mod-loopback.o
	-rm -f $@
	$(LD) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-loopback.o: loopback_mod-disk_loopback.o
	-rm -f $@
	$(LD) -r -d -o $@ $^

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

loopback_mod_CFLAGS = $(COMMON_CFLAGS)

# For default.mod
default_mod_SOURCES = commands/default.c
CLEANFILES += default.mod mod-default.o mod-default.c pre-default.o default_mod-commands_default.o def-default.lst und-default.lst
MOSTLYCLEANFILES += default_mod-commands_default.d
DEFSYMFILES += def-default.lst
UNDSYMFILES += und-default.lst

default.mod: pre-default.o mod-default.o
	-rm -f $@
	$(LD) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-default.o: default_mod-commands_default.o
	-rm -f $@
	$(LD) -r -d -o $@ $^

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

default_mod_CFLAGS = $(COMMON_CFLAGS)

# For timeout.mod
timeout_mod_SOURCES = commands/timeout.c
CLEANFILES += timeout.mod mod-timeout.o mod-timeout.c pre-timeout.o timeout_mod-commands_timeout.o def-timeout.lst und-timeout.lst
MOSTLYCLEANFILES += timeout_mod-commands_timeout.d
DEFSYMFILES += def-timeout.lst
UNDSYMFILES += und-timeout.lst

timeout.mod: pre-timeout.o mod-timeout.o
	-rm -f $@
	$(LD) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-timeout.o: timeout_mod-commands_timeout.o
	-rm -f $@
	$(LD) -r -d -o $@ $^

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

timeout_mod_CFLAGS = $(COMMON_CFLAGS)

# For configfile.mod
configfile_mod_SOURCES = commands/configfile.c
CLEANFILES += configfile.mod mod-configfile.o mod-configfile.c pre-configfile.o configfile_mod-commands_configfile.o def-configfile.lst und-configfile.lst
MOSTLYCLEANFILES += configfile_mod-commands_configfile.d
DEFSYMFILES += def-configfile.lst
UNDSYMFILES += und-configfile.lst

configfile.mod: pre-configfile.o mod-configfile.o
	-rm -f $@
	$(LD) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-configfile.o: configfile_mod-commands_configfile.o
	-rm -f $@
	$(LD) -r -d -o $@ $^

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

configfile_mod_CFLAGS = $(COMMON_CFLAGS)
CLEANFILES += moddep.lst
pkgdata_DATA += moddep.lst
moddep.lst: $(DEFSYMFILES) $(UNDSYMFILES) genmoddep
	cat $(DEFSYMFILES) /dev/null | ./genmoddep $(UNDSYMFILES) > $@ \
	  || (rm -f $@; exit 1)
