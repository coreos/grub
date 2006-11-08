# -*- makefile -*-

COMMON_ASFLAGS = -nostdinc -fno-builtin -m32
COMMON_CFLAGS = -fno-builtin -mrtd -mregparm=3 -m32
COMMON_LDFLAGS = -m32 -nostdlib

# Images.
pkgdata_IMAGES = boot.img diskboot.img kernel.img pxeboot.img

# For boot.img.
boot_img_SOURCES = boot/i386/pc/boot.S
CLEANFILES += boot.img boot.exec boot_img-boot_i386_pc_boot.o
MOSTLYCLEANFILES += boot_img-boot_i386_pc_boot.d

boot.img: boot.exec
	$(OBJCOPY) -O binary -R .note -R .comment $< $@

boot.exec: boot_img-boot_i386_pc_boot.o
	$(TARGET_CC) -o $@ $^ $(TARGET_LDFLAGS) $(boot_img_LDFLAGS)

boot_img-boot_i386_pc_boot.o: boot/i386/pc/boot.S
	$(TARGET_CC) -Iboot/i386/pc -I$(srcdir)/boot/i386/pc $(TARGET_CPPFLAGS) -DASM_FILE=1 $(TARGET_ASFLAGS) $(boot_img_ASFLAGS) -MD -c -o $@ $<
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
	$(TARGET_CC) -o $@ $^ $(TARGET_LDFLAGS) $(pxeboot_img_LDFLAGS)

pxeboot_img-boot_i386_pc_pxeboot.o: boot/i386/pc/pxeboot.S
	$(TARGET_CC) -Iboot/i386/pc -I$(srcdir)/boot/i386/pc $(TARGET_CPPFLAGS) -DASM_FILE=1 $(TARGET_ASFLAGS) $(pxeboot_img_ASFLAGS) -MD -c -o $@ $<
-include pxeboot_img-boot_i386_pc_pxeboot.d

pxeboot_img_ASFLAGS = $(COMMON_ASFLAGS)
pxeboot_img_LDFLAGS = -nostdlib -Wl,-N,-Ttext,7C00

# For diskboot.img.
diskboot_img_SOURCES = boot/i386/pc/diskboot.S
CLEANFILES += diskboot.img diskboot.exec diskboot_img-boot_i386_pc_diskboot.o
MOSTLYCLEANFILES += diskboot_img-boot_i386_pc_diskboot.d

diskboot.img: diskboot.exec
	$(OBJCOPY) -O binary -R .note -R .comment $< $@

diskboot.exec: diskboot_img-boot_i386_pc_diskboot.o
	$(TARGET_CC) -o $@ $^ $(TARGET_LDFLAGS) $(diskboot_img_LDFLAGS)

diskboot_img-boot_i386_pc_diskboot.o: boot/i386/pc/diskboot.S
	$(TARGET_CC) -Iboot/i386/pc -I$(srcdir)/boot/i386/pc $(TARGET_CPPFLAGS) -DASM_FILE=1 $(TARGET_ASFLAGS) $(diskboot_img_ASFLAGS) -MD -c -o $@ $<
-include diskboot_img-boot_i386_pc_diskboot.d

diskboot_img_ASFLAGS = $(COMMON_ASFLAGS)
diskboot_img_LDFLAGS = -nostdlib -Wl,-N,-Ttext,8000

# For kernel.img.
kernel_img_SOURCES = kern/i386/pc/startup.S kern/main.c kern/device.c \
	kern/disk.c kern/dl.c kern/file.c kern/fs.c kern/err.c \
	kern/misc.c kern/mm.c kern/loader.c kern/rescue.c kern/term.c \
	kern/i386/dl.c kern/i386/pc/init.c kern/parser.c kern/partition.c \
	kern/env.c disk/i386/pc/biosdisk.c \
	term/i386/pc/console.c \
	symlist.c
CLEANFILES += kernel.img kernel.exec kernel_img-kern_i386_pc_startup.o kernel_img-kern_main.o kernel_img-kern_device.o kernel_img-kern_disk.o kernel_img-kern_dl.o kernel_img-kern_file.o kernel_img-kern_fs.o kernel_img-kern_err.o kernel_img-kern_misc.o kernel_img-kern_mm.o kernel_img-kern_loader.o kernel_img-kern_rescue.o kernel_img-kern_term.o kernel_img-kern_i386_dl.o kernel_img-kern_i386_pc_init.o kernel_img-kern_parser.o kernel_img-kern_partition.o kernel_img-kern_env.o kernel_img-disk_i386_pc_biosdisk.o kernel_img-term_i386_pc_console.o kernel_img-symlist.o
MOSTLYCLEANFILES += kernel_img-kern_i386_pc_startup.d kernel_img-kern_main.d kernel_img-kern_device.d kernel_img-kern_disk.d kernel_img-kern_dl.d kernel_img-kern_file.d kernel_img-kern_fs.d kernel_img-kern_err.d kernel_img-kern_misc.d kernel_img-kern_mm.d kernel_img-kern_loader.d kernel_img-kern_rescue.d kernel_img-kern_term.d kernel_img-kern_i386_dl.d kernel_img-kern_i386_pc_init.d kernel_img-kern_parser.d kernel_img-kern_partition.d kernel_img-kern_env.d kernel_img-disk_i386_pc_biosdisk.d kernel_img-term_i386_pc_console.d kernel_img-symlist.d

kernel.img: kernel.exec
	$(OBJCOPY) -O binary -R .note -R .comment $< $@

kernel.exec: kernel_img-kern_i386_pc_startup.o kernel_img-kern_main.o kernel_img-kern_device.o kernel_img-kern_disk.o kernel_img-kern_dl.o kernel_img-kern_file.o kernel_img-kern_fs.o kernel_img-kern_err.o kernel_img-kern_misc.o kernel_img-kern_mm.o kernel_img-kern_loader.o kernel_img-kern_rescue.o kernel_img-kern_term.o kernel_img-kern_i386_dl.o kernel_img-kern_i386_pc_init.o kernel_img-kern_parser.o kernel_img-kern_partition.o kernel_img-kern_env.o kernel_img-disk_i386_pc_biosdisk.o kernel_img-term_i386_pc_console.o kernel_img-symlist.o
	$(TARGET_CC) -o $@ $^ $(TARGET_LDFLAGS) $(kernel_img_LDFLAGS)

kernel_img-kern_i386_pc_startup.o: kern/i386/pc/startup.S
	$(TARGET_CC) -Ikern/i386/pc -I$(srcdir)/kern/i386/pc $(TARGET_CPPFLAGS) -DASM_FILE=1 $(TARGET_ASFLAGS) $(kernel_img_ASFLAGS) -MD -c -o $@ $<
-include kernel_img-kern_i386_pc_startup.d

kernel_img-kern_main.o: kern/main.c
	$(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(kernel_img_CFLAGS) -MD -c -o $@ $<
-include kernel_img-kern_main.d

kernel_img-kern_device.o: kern/device.c
	$(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(kernel_img_CFLAGS) -MD -c -o $@ $<
-include kernel_img-kern_device.d

kernel_img-kern_disk.o: kern/disk.c
	$(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(kernel_img_CFLAGS) -MD -c -o $@ $<
-include kernel_img-kern_disk.d

kernel_img-kern_dl.o: kern/dl.c
	$(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(kernel_img_CFLAGS) -MD -c -o $@ $<
-include kernel_img-kern_dl.d

kernel_img-kern_file.o: kern/file.c
	$(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(kernel_img_CFLAGS) -MD -c -o $@ $<
-include kernel_img-kern_file.d

kernel_img-kern_fs.o: kern/fs.c
	$(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(kernel_img_CFLAGS) -MD -c -o $@ $<
-include kernel_img-kern_fs.d

kernel_img-kern_err.o: kern/err.c
	$(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(kernel_img_CFLAGS) -MD -c -o $@ $<
-include kernel_img-kern_err.d

kernel_img-kern_misc.o: kern/misc.c
	$(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(kernel_img_CFLAGS) -MD -c -o $@ $<
-include kernel_img-kern_misc.d

kernel_img-kern_mm.o: kern/mm.c
	$(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(kernel_img_CFLAGS) -MD -c -o $@ $<
-include kernel_img-kern_mm.d

kernel_img-kern_loader.o: kern/loader.c
	$(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(kernel_img_CFLAGS) -MD -c -o $@ $<
-include kernel_img-kern_loader.d

kernel_img-kern_rescue.o: kern/rescue.c
	$(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(kernel_img_CFLAGS) -MD -c -o $@ $<
-include kernel_img-kern_rescue.d

kernel_img-kern_term.o: kern/term.c
	$(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(kernel_img_CFLAGS) -MD -c -o $@ $<
-include kernel_img-kern_term.d

kernel_img-kern_i386_dl.o: kern/i386/dl.c
	$(TARGET_CC) -Ikern/i386 -I$(srcdir)/kern/i386 $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(kernel_img_CFLAGS) -MD -c -o $@ $<
-include kernel_img-kern_i386_dl.d

kernel_img-kern_i386_pc_init.o: kern/i386/pc/init.c
	$(TARGET_CC) -Ikern/i386/pc -I$(srcdir)/kern/i386/pc $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(kernel_img_CFLAGS) -MD -c -o $@ $<
-include kernel_img-kern_i386_pc_init.d

kernel_img-kern_parser.o: kern/parser.c
	$(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(kernel_img_CFLAGS) -MD -c -o $@ $<
-include kernel_img-kern_parser.d

kernel_img-kern_partition.o: kern/partition.c
	$(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(kernel_img_CFLAGS) -MD -c -o $@ $<
-include kernel_img-kern_partition.d

kernel_img-kern_env.o: kern/env.c
	$(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(kernel_img_CFLAGS) -MD -c -o $@ $<
-include kernel_img-kern_env.d

kernel_img-disk_i386_pc_biosdisk.o: disk/i386/pc/biosdisk.c
	$(TARGET_CC) -Idisk/i386/pc -I$(srcdir)/disk/i386/pc $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(kernel_img_CFLAGS) -MD -c -o $@ $<
-include kernel_img-disk_i386_pc_biosdisk.d

kernel_img-term_i386_pc_console.o: term/i386/pc/console.c
	$(TARGET_CC) -Iterm/i386/pc -I$(srcdir)/term/i386/pc $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(kernel_img_CFLAGS) -MD -c -o $@ $<
-include kernel_img-term_i386_pc_console.d

kernel_img-symlist.o: symlist.c
	$(TARGET_CC) -I. -I$(srcdir)/. $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(kernel_img_CFLAGS) -MD -c -o $@ $<
-include kernel_img-symlist.d

kernel_img_HEADERS = arg.h boot.h cache.h device.h disk.h dl.h elf.h elfload.h \
	env.h err.h file.h fs.h kernel.h loader.h misc.h mm.h net.h parser.h \
	partition.h pc_partition.h rescue.h symbol.h term.h types.h \
	machine/biosdisk.h machine/boot.h machine/console.h machine/init.h \
	machine/memory.h machine/loader.h machine/time.h machine/vga.h \
	machine/vbe.h
kernel_img_CFLAGS = $(COMMON_CFLAGS)
kernel_img_ASFLAGS = $(COMMON_ASFLAGS)
kernel_img_LDFLAGS = -nostdlib -Wl,-N,-Ttext,8200 $(COMMON_CFLAGS)

MOSTLYCLEANFILES += symlist.c kernel_syms.lst
DEFSYMFILES += kernel_syms.lst

symlist.c: $(addprefix include/grub/,$(kernel_img_HEADERS)) config.h gensymlist.sh
	/bin/sh gensymlist.sh $(filter %.h,$^) > $@ || (rm -f $@; exit 1)

kernel_syms.lst: $(addprefix include/grub/,$(kernel_img_HEADERS)) config.h genkernsyms.sh
	/bin/sh genkernsyms.sh $(filter %.h,$^) > $@ || (rm -f $@; exit 1)

# Utilities.
bin_UTILITIES = grub-mkimage
sbin_UTILITIES = grub-setup grub-emu grub-mkdevicemap grub-probe

# For grub-mkimage.
grub_mkimage_SOURCES = util/i386/pc/grub-mkimage.c util/misc.c \
	util/resolve.c
CLEANFILES += grub-mkimage grub_mkimage-util_i386_pc_grub_mkimage.o grub_mkimage-util_misc.o grub_mkimage-util_resolve.o
MOSTLYCLEANFILES += grub_mkimage-util_i386_pc_grub_mkimage.d grub_mkimage-util_misc.d grub_mkimage-util_resolve.d

grub-mkimage: $(grub_mkimage_DEPENDENCIES) grub_mkimage-util_i386_pc_grub_mkimage.o grub_mkimage-util_misc.o grub_mkimage-util_resolve.o
	$(CC) -o $@ grub_mkimage-util_i386_pc_grub_mkimage.o grub_mkimage-util_misc.o grub_mkimage-util_resolve.o $(LDFLAGS) $(grub_mkimage_LDFLAGS)

grub_mkimage-util_i386_pc_grub_mkimage.o: util/i386/pc/grub-mkimage.c
	$(CC) -Iutil/i386/pc -I$(srcdir)/util/i386/pc $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_mkimage_CFLAGS) -MD -c -o $@ $<
-include grub_mkimage-util_i386_pc_grub_mkimage.d

grub_mkimage-util_misc.o: util/misc.c
	$(CC) -Iutil -I$(srcdir)/util $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_mkimage_CFLAGS) -MD -c -o $@ $<
-include grub_mkimage-util_misc.d

grub_mkimage-util_resolve.o: util/resolve.c
	$(CC) -Iutil -I$(srcdir)/util $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_mkimage_CFLAGS) -MD -c -o $@ $<
-include grub_mkimage-util_resolve.d

grub_mkimage_LDFLAGS = $(LIBLZO)

# For grub-setup.
grub_setup_SOURCES = util/i386/pc/grub-setup.c util/i386/pc/biosdisk.c	\
	util/misc.c util/i386/pc/getroot.c kern/device.c kern/disk.c	\
	kern/err.c kern/misc.c fs/fat.c fs/ext2.c fs/xfs.c fs/affs.c	\
	fs/sfs.c kern/parser.c kern/partition.c partmap/pc.c		\
	fs/ufs.c fs/minix.c fs/hfs.c fs/jfs.c fs/hfsplus.c kern/file.c	\
	kern/fs.c kern/env.c fs/fshelp.c util/raid.c util/lvm.c
CLEANFILES += grub-setup grub_setup-util_i386_pc_grub_setup.o grub_setup-util_i386_pc_biosdisk.o grub_setup-util_misc.o grub_setup-util_i386_pc_getroot.o grub_setup-kern_device.o grub_setup-kern_disk.o grub_setup-kern_err.o grub_setup-kern_misc.o grub_setup-fs_fat.o grub_setup-fs_ext2.o grub_setup-fs_xfs.o grub_setup-fs_affs.o grub_setup-fs_sfs.o grub_setup-kern_parser.o grub_setup-kern_partition.o grub_setup-partmap_pc.o grub_setup-fs_ufs.o grub_setup-fs_minix.o grub_setup-fs_hfs.o grub_setup-fs_jfs.o grub_setup-fs_hfsplus.o grub_setup-kern_file.o grub_setup-kern_fs.o grub_setup-kern_env.o grub_setup-fs_fshelp.o grub_setup-util_raid.o grub_setup-util_lvm.o
MOSTLYCLEANFILES += grub_setup-util_i386_pc_grub_setup.d grub_setup-util_i386_pc_biosdisk.d grub_setup-util_misc.d grub_setup-util_i386_pc_getroot.d grub_setup-kern_device.d grub_setup-kern_disk.d grub_setup-kern_err.d grub_setup-kern_misc.d grub_setup-fs_fat.d grub_setup-fs_ext2.d grub_setup-fs_xfs.d grub_setup-fs_affs.d grub_setup-fs_sfs.d grub_setup-kern_parser.d grub_setup-kern_partition.d grub_setup-partmap_pc.d grub_setup-fs_ufs.d grub_setup-fs_minix.d grub_setup-fs_hfs.d grub_setup-fs_jfs.d grub_setup-fs_hfsplus.d grub_setup-kern_file.d grub_setup-kern_fs.d grub_setup-kern_env.d grub_setup-fs_fshelp.d grub_setup-util_raid.d grub_setup-util_lvm.d

grub-setup: $(grub_setup_DEPENDENCIES) grub_setup-util_i386_pc_grub_setup.o grub_setup-util_i386_pc_biosdisk.o grub_setup-util_misc.o grub_setup-util_i386_pc_getroot.o grub_setup-kern_device.o grub_setup-kern_disk.o grub_setup-kern_err.o grub_setup-kern_misc.o grub_setup-fs_fat.o grub_setup-fs_ext2.o grub_setup-fs_xfs.o grub_setup-fs_affs.o grub_setup-fs_sfs.o grub_setup-kern_parser.o grub_setup-kern_partition.o grub_setup-partmap_pc.o grub_setup-fs_ufs.o grub_setup-fs_minix.o grub_setup-fs_hfs.o grub_setup-fs_jfs.o grub_setup-fs_hfsplus.o grub_setup-kern_file.o grub_setup-kern_fs.o grub_setup-kern_env.o grub_setup-fs_fshelp.o grub_setup-util_raid.o grub_setup-util_lvm.o
	$(CC) -o $@ grub_setup-util_i386_pc_grub_setup.o grub_setup-util_i386_pc_biosdisk.o grub_setup-util_misc.o grub_setup-util_i386_pc_getroot.o grub_setup-kern_device.o grub_setup-kern_disk.o grub_setup-kern_err.o grub_setup-kern_misc.o grub_setup-fs_fat.o grub_setup-fs_ext2.o grub_setup-fs_xfs.o grub_setup-fs_affs.o grub_setup-fs_sfs.o grub_setup-kern_parser.o grub_setup-kern_partition.o grub_setup-partmap_pc.o grub_setup-fs_ufs.o grub_setup-fs_minix.o grub_setup-fs_hfs.o grub_setup-fs_jfs.o grub_setup-fs_hfsplus.o grub_setup-kern_file.o grub_setup-kern_fs.o grub_setup-kern_env.o grub_setup-fs_fshelp.o grub_setup-util_raid.o grub_setup-util_lvm.o $(LDFLAGS) $(grub_setup_LDFLAGS)

grub_setup-util_i386_pc_grub_setup.o: util/i386/pc/grub-setup.c
	$(CC) -Iutil/i386/pc -I$(srcdir)/util/i386/pc $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -MD -c -o $@ $<
-include grub_setup-util_i386_pc_grub_setup.d

grub_setup-util_i386_pc_biosdisk.o: util/i386/pc/biosdisk.c
	$(CC) -Iutil/i386/pc -I$(srcdir)/util/i386/pc $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -MD -c -o $@ $<
-include grub_setup-util_i386_pc_biosdisk.d

grub_setup-util_misc.o: util/misc.c
	$(CC) -Iutil -I$(srcdir)/util $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -MD -c -o $@ $<
-include grub_setup-util_misc.d

grub_setup-util_i386_pc_getroot.o: util/i386/pc/getroot.c
	$(CC) -Iutil/i386/pc -I$(srcdir)/util/i386/pc $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -MD -c -o $@ $<
-include grub_setup-util_i386_pc_getroot.d

grub_setup-kern_device.o: kern/device.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -MD -c -o $@ $<
-include grub_setup-kern_device.d

grub_setup-kern_disk.o: kern/disk.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -MD -c -o $@ $<
-include grub_setup-kern_disk.d

grub_setup-kern_err.o: kern/err.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -MD -c -o $@ $<
-include grub_setup-kern_err.d

grub_setup-kern_misc.o: kern/misc.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -MD -c -o $@ $<
-include grub_setup-kern_misc.d

grub_setup-fs_fat.o: fs/fat.c
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -MD -c -o $@ $<
-include grub_setup-fs_fat.d

grub_setup-fs_ext2.o: fs/ext2.c
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -MD -c -o $@ $<
-include grub_setup-fs_ext2.d

grub_setup-fs_xfs.o: fs/xfs.c
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -MD -c -o $@ $<
-include grub_setup-fs_xfs.d

grub_setup-fs_affs.o: fs/affs.c
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -MD -c -o $@ $<
-include grub_setup-fs_affs.d

grub_setup-fs_sfs.o: fs/sfs.c
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -MD -c -o $@ $<
-include grub_setup-fs_sfs.d

grub_setup-kern_parser.o: kern/parser.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -MD -c -o $@ $<
-include grub_setup-kern_parser.d

grub_setup-kern_partition.o: kern/partition.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -MD -c -o $@ $<
-include grub_setup-kern_partition.d

grub_setup-partmap_pc.o: partmap/pc.c
	$(CC) -Ipartmap -I$(srcdir)/partmap $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -MD -c -o $@ $<
-include grub_setup-partmap_pc.d

grub_setup-fs_ufs.o: fs/ufs.c
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -MD -c -o $@ $<
-include grub_setup-fs_ufs.d

grub_setup-fs_minix.o: fs/minix.c
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -MD -c -o $@ $<
-include grub_setup-fs_minix.d

grub_setup-fs_hfs.o: fs/hfs.c
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -MD -c -o $@ $<
-include grub_setup-fs_hfs.d

grub_setup-fs_jfs.o: fs/jfs.c
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -MD -c -o $@ $<
-include grub_setup-fs_jfs.d

grub_setup-fs_hfsplus.o: fs/hfsplus.c
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -MD -c -o $@ $<
-include grub_setup-fs_hfsplus.d

grub_setup-kern_file.o: kern/file.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -MD -c -o $@ $<
-include grub_setup-kern_file.d

grub_setup-kern_fs.o: kern/fs.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -MD -c -o $@ $<
-include grub_setup-kern_fs.d

grub_setup-kern_env.o: kern/env.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -MD -c -o $@ $<
-include grub_setup-kern_env.d

grub_setup-fs_fshelp.o: fs/fshelp.c
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -MD -c -o $@ $<
-include grub_setup-fs_fshelp.d

grub_setup-util_raid.o: util/raid.c
	$(CC) -Iutil -I$(srcdir)/util $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -MD -c -o $@ $<
-include grub_setup-util_raid.d

grub_setup-util_lvm.o: util/lvm.c
	$(CC) -Iutil -I$(srcdir)/util $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -MD -c -o $@ $<
-include grub_setup-util_lvm.d


# For grub-mkdevicemap.
grub_mkdevicemap_SOURCES = util/i386/pc/grub-mkdevicemap.c util/misc.c
CLEANFILES += grub-mkdevicemap grub_mkdevicemap-util_i386_pc_grub_mkdevicemap.o grub_mkdevicemap-util_misc.o
MOSTLYCLEANFILES += grub_mkdevicemap-util_i386_pc_grub_mkdevicemap.d grub_mkdevicemap-util_misc.d

grub-mkdevicemap: $(grub_mkdevicemap_DEPENDENCIES) grub_mkdevicemap-util_i386_pc_grub_mkdevicemap.o grub_mkdevicemap-util_misc.o
	$(CC) -o $@ grub_mkdevicemap-util_i386_pc_grub_mkdevicemap.o grub_mkdevicemap-util_misc.o $(LDFLAGS) $(grub_mkdevicemap_LDFLAGS)

grub_mkdevicemap-util_i386_pc_grub_mkdevicemap.o: util/i386/pc/grub-mkdevicemap.c
	$(CC) -Iutil/i386/pc -I$(srcdir)/util/i386/pc $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_mkdevicemap_CFLAGS) -MD -c -o $@ $<
-include grub_mkdevicemap-util_i386_pc_grub_mkdevicemap.d

grub_mkdevicemap-util_misc.o: util/misc.c
	$(CC) -Iutil -I$(srcdir)/util $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_mkdevicemap_CFLAGS) -MD -c -o $@ $<
-include grub_mkdevicemap-util_misc.d


# For grub-probe.
grub_probe_SOURCES = util/i386/pc/grub-probe.c	\
	util/i386/pc/biosdisk.c	util/misc.c util/i386/pc/getroot.c	\
	kern/device.c kern/disk.c kern/err.c kern/misc.c fs/fat.c	\
	fs/ext2.c kern/parser.c kern/partition.c partmap/pc.c fs/ufs.c 	\
	fs/minix.c fs/hfs.c fs/jfs.c kern/fs.c kern/env.c fs/fshelp.c 	\
	fs/xfs.c fs/affs.c fs/sfs.c fs/hfsplus.c
CLEANFILES += grub-probe grub_probe-util_i386_pc_grub_probe.o grub_probe-util_i386_pc_biosdisk.o grub_probe-util_misc.o grub_probe-util_i386_pc_getroot.o grub_probe-kern_device.o grub_probe-kern_disk.o grub_probe-kern_err.o grub_probe-kern_misc.o grub_probe-fs_fat.o grub_probe-fs_ext2.o grub_probe-kern_parser.o grub_probe-kern_partition.o grub_probe-partmap_pc.o grub_probe-fs_ufs.o grub_probe-fs_minix.o grub_probe-fs_hfs.o grub_probe-fs_jfs.o grub_probe-kern_fs.o grub_probe-kern_env.o grub_probe-fs_fshelp.o grub_probe-fs_xfs.o grub_probe-fs_affs.o grub_probe-fs_sfs.o grub_probe-fs_hfsplus.o
MOSTLYCLEANFILES += grub_probe-util_i386_pc_grub_probe.d grub_probe-util_i386_pc_biosdisk.d grub_probe-util_misc.d grub_probe-util_i386_pc_getroot.d grub_probe-kern_device.d grub_probe-kern_disk.d grub_probe-kern_err.d grub_probe-kern_misc.d grub_probe-fs_fat.d grub_probe-fs_ext2.d grub_probe-kern_parser.d grub_probe-kern_partition.d grub_probe-partmap_pc.d grub_probe-fs_ufs.d grub_probe-fs_minix.d grub_probe-fs_hfs.d grub_probe-fs_jfs.d grub_probe-kern_fs.d grub_probe-kern_env.d grub_probe-fs_fshelp.d grub_probe-fs_xfs.d grub_probe-fs_affs.d grub_probe-fs_sfs.d grub_probe-fs_hfsplus.d

grub-probe: $(grub_probe_DEPENDENCIES) grub_probe-util_i386_pc_grub_probe.o grub_probe-util_i386_pc_biosdisk.o grub_probe-util_misc.o grub_probe-util_i386_pc_getroot.o grub_probe-kern_device.o grub_probe-kern_disk.o grub_probe-kern_err.o grub_probe-kern_misc.o grub_probe-fs_fat.o grub_probe-fs_ext2.o grub_probe-kern_parser.o grub_probe-kern_partition.o grub_probe-partmap_pc.o grub_probe-fs_ufs.o grub_probe-fs_minix.o grub_probe-fs_hfs.o grub_probe-fs_jfs.o grub_probe-kern_fs.o grub_probe-kern_env.o grub_probe-fs_fshelp.o grub_probe-fs_xfs.o grub_probe-fs_affs.o grub_probe-fs_sfs.o grub_probe-fs_hfsplus.o
	$(CC) -o $@ grub_probe-util_i386_pc_grub_probe.o grub_probe-util_i386_pc_biosdisk.o grub_probe-util_misc.o grub_probe-util_i386_pc_getroot.o grub_probe-kern_device.o grub_probe-kern_disk.o grub_probe-kern_err.o grub_probe-kern_misc.o grub_probe-fs_fat.o grub_probe-fs_ext2.o grub_probe-kern_parser.o grub_probe-kern_partition.o grub_probe-partmap_pc.o grub_probe-fs_ufs.o grub_probe-fs_minix.o grub_probe-fs_hfs.o grub_probe-fs_jfs.o grub_probe-kern_fs.o grub_probe-kern_env.o grub_probe-fs_fshelp.o grub_probe-fs_xfs.o grub_probe-fs_affs.o grub_probe-fs_sfs.o grub_probe-fs_hfsplus.o $(LDFLAGS) $(grub_probe_LDFLAGS)

grub_probe-util_i386_pc_grub_probe.o: util/i386/pc/grub-probe.c
	$(CC) -Iutil/i386/pc -I$(srcdir)/util/i386/pc $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_probe_CFLAGS) -MD -c -o $@ $<
-include grub_probe-util_i386_pc_grub_probe.d

grub_probe-util_i386_pc_biosdisk.o: util/i386/pc/biosdisk.c
	$(CC) -Iutil/i386/pc -I$(srcdir)/util/i386/pc $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_probe_CFLAGS) -MD -c -o $@ $<
-include grub_probe-util_i386_pc_biosdisk.d

grub_probe-util_misc.o: util/misc.c
	$(CC) -Iutil -I$(srcdir)/util $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_probe_CFLAGS) -MD -c -o $@ $<
-include grub_probe-util_misc.d

grub_probe-util_i386_pc_getroot.o: util/i386/pc/getroot.c
	$(CC) -Iutil/i386/pc -I$(srcdir)/util/i386/pc $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_probe_CFLAGS) -MD -c -o $@ $<
-include grub_probe-util_i386_pc_getroot.d

grub_probe-kern_device.o: kern/device.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_probe_CFLAGS) -MD -c -o $@ $<
-include grub_probe-kern_device.d

grub_probe-kern_disk.o: kern/disk.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_probe_CFLAGS) -MD -c -o $@ $<
-include grub_probe-kern_disk.d

grub_probe-kern_err.o: kern/err.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_probe_CFLAGS) -MD -c -o $@ $<
-include grub_probe-kern_err.d

grub_probe-kern_misc.o: kern/misc.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_probe_CFLAGS) -MD -c -o $@ $<
-include grub_probe-kern_misc.d

grub_probe-fs_fat.o: fs/fat.c
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_probe_CFLAGS) -MD -c -o $@ $<
-include grub_probe-fs_fat.d

grub_probe-fs_ext2.o: fs/ext2.c
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_probe_CFLAGS) -MD -c -o $@ $<
-include grub_probe-fs_ext2.d

grub_probe-kern_parser.o: kern/parser.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_probe_CFLAGS) -MD -c -o $@ $<
-include grub_probe-kern_parser.d

grub_probe-kern_partition.o: kern/partition.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_probe_CFLAGS) -MD -c -o $@ $<
-include grub_probe-kern_partition.d

grub_probe-partmap_pc.o: partmap/pc.c
	$(CC) -Ipartmap -I$(srcdir)/partmap $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_probe_CFLAGS) -MD -c -o $@ $<
-include grub_probe-partmap_pc.d

grub_probe-fs_ufs.o: fs/ufs.c
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_probe_CFLAGS) -MD -c -o $@ $<
-include grub_probe-fs_ufs.d

grub_probe-fs_minix.o: fs/minix.c
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_probe_CFLAGS) -MD -c -o $@ $<
-include grub_probe-fs_minix.d

grub_probe-fs_hfs.o: fs/hfs.c
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_probe_CFLAGS) -MD -c -o $@ $<
-include grub_probe-fs_hfs.d

grub_probe-fs_jfs.o: fs/jfs.c
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_probe_CFLAGS) -MD -c -o $@ $<
-include grub_probe-fs_jfs.d

grub_probe-kern_fs.o: kern/fs.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_probe_CFLAGS) -MD -c -o $@ $<
-include grub_probe-kern_fs.d

grub_probe-kern_env.o: kern/env.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_probe_CFLAGS) -MD -c -o $@ $<
-include grub_probe-kern_env.d

grub_probe-fs_fshelp.o: fs/fshelp.c
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_probe_CFLAGS) -MD -c -o $@ $<
-include grub_probe-fs_fshelp.d

grub_probe-fs_xfs.o: fs/xfs.c
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_probe_CFLAGS) -MD -c -o $@ $<
-include grub_probe-fs_xfs.d

grub_probe-fs_affs.o: fs/affs.c
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_probe_CFLAGS) -MD -c -o $@ $<
-include grub_probe-fs_affs.d

grub_probe-fs_sfs.o: fs/sfs.c
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_probe_CFLAGS) -MD -c -o $@ $<
-include grub_probe-fs_sfs.d

grub_probe-fs_hfsplus.o: fs/hfsplus.c
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_probe_CFLAGS) -MD -c -o $@ $<
-include grub_probe-fs_hfsplus.d


# For grub-emu.
grub_emu_DEPENDENCIES = grub_script.tab.c grub_script.tab.h		\
	grub_modules_init.h
grub_emu_SOURCES = commands/boot.c commands/cat.c commands/cmp.c	\
	commands/configfile.c commands/echo.c commands/help.c		\
	commands/terminal.c commands/ls.c commands/test.c 		\
	commands/search.c commands/blocklist.c				\
	commands/i386/pc/halt.c commands/i386/pc/reboot.c		\
	disk/loopback.c	disk/raid.c disk/lvm.c				\
	fs/affs.c fs/ext2.c fs/fat.c fs/fshelp.c fs/hfs.c fs/iso9660.c	\
	fs/jfs.c fs/minix.c fs/sfs.c fs/ufs.c fs/xfs.c fs/hfsplus.c	\
	io/gzio.c							\
	kern/device.c kern/disk.c kern/dl.c kern/elf.c kern/env.c	\
	kern/err.c							\
	normal/execute.c kern/file.c kern/fs.c normal/lexer.c 		\
	kern/loader.c kern/main.c kern/misc.c kern/parser.c		\
	grub_script.tab.c kern/partition.c kern/rescue.c kern/term.c	\
	normal/arg.c normal/cmdline.c normal/command.c normal/function.c\
	normal/completion.c normal/main.c				\
	normal/menu.c normal/menu_entry.c normal/misc.c normal/script.c	\
	partmap/amiga.c	partmap/apple.c partmap/pc.c partmap/sun.c	\
	partmap/acorn.c partmap/gpt.c					\
	util/console.c util/grub-emu.c util/misc.c			\
	util/i386/pc/biosdisk.c util/i386/pc/getroot.c			\
	util/i386/pc/misc.c grub_emu_init.c
CLEANFILES += grub-emu grub_emu-commands_boot.o grub_emu-commands_cat.o grub_emu-commands_cmp.o grub_emu-commands_configfile.o grub_emu-commands_echo.o grub_emu-commands_help.o grub_emu-commands_terminal.o grub_emu-commands_ls.o grub_emu-commands_test.o grub_emu-commands_search.o grub_emu-commands_blocklist.o grub_emu-commands_i386_pc_halt.o grub_emu-commands_i386_pc_reboot.o grub_emu-disk_loopback.o grub_emu-disk_raid.o grub_emu-disk_lvm.o grub_emu-fs_affs.o grub_emu-fs_ext2.o grub_emu-fs_fat.o grub_emu-fs_fshelp.o grub_emu-fs_hfs.o grub_emu-fs_iso9660.o grub_emu-fs_jfs.o grub_emu-fs_minix.o grub_emu-fs_sfs.o grub_emu-fs_ufs.o grub_emu-fs_xfs.o grub_emu-fs_hfsplus.o grub_emu-io_gzio.o grub_emu-kern_device.o grub_emu-kern_disk.o grub_emu-kern_dl.o grub_emu-kern_elf.o grub_emu-kern_env.o grub_emu-kern_err.o grub_emu-normal_execute.o grub_emu-kern_file.o grub_emu-kern_fs.o grub_emu-normal_lexer.o grub_emu-kern_loader.o grub_emu-kern_main.o grub_emu-kern_misc.o grub_emu-kern_parser.o grub_emu-grub_script_tab.o grub_emu-kern_partition.o grub_emu-kern_rescue.o grub_emu-kern_term.o grub_emu-normal_arg.o grub_emu-normal_cmdline.o grub_emu-normal_command.o grub_emu-normal_function.o grub_emu-normal_completion.o grub_emu-normal_main.o grub_emu-normal_menu.o grub_emu-normal_menu_entry.o grub_emu-normal_misc.o grub_emu-normal_script.o grub_emu-partmap_amiga.o grub_emu-partmap_apple.o grub_emu-partmap_pc.o grub_emu-partmap_sun.o grub_emu-partmap_acorn.o grub_emu-partmap_gpt.o grub_emu-util_console.o grub_emu-util_grub_emu.o grub_emu-util_misc.o grub_emu-util_i386_pc_biosdisk.o grub_emu-util_i386_pc_getroot.o grub_emu-util_i386_pc_misc.o grub_emu-grub_emu_init.o
MOSTLYCLEANFILES += grub_emu-commands_boot.d grub_emu-commands_cat.d grub_emu-commands_cmp.d grub_emu-commands_configfile.d grub_emu-commands_echo.d grub_emu-commands_help.d grub_emu-commands_terminal.d grub_emu-commands_ls.d grub_emu-commands_test.d grub_emu-commands_search.d grub_emu-commands_blocklist.d grub_emu-commands_i386_pc_halt.d grub_emu-commands_i386_pc_reboot.d grub_emu-disk_loopback.d grub_emu-disk_raid.d grub_emu-disk_lvm.d grub_emu-fs_affs.d grub_emu-fs_ext2.d grub_emu-fs_fat.d grub_emu-fs_fshelp.d grub_emu-fs_hfs.d grub_emu-fs_iso9660.d grub_emu-fs_jfs.d grub_emu-fs_minix.d grub_emu-fs_sfs.d grub_emu-fs_ufs.d grub_emu-fs_xfs.d grub_emu-fs_hfsplus.d grub_emu-io_gzio.d grub_emu-kern_device.d grub_emu-kern_disk.d grub_emu-kern_dl.d grub_emu-kern_elf.d grub_emu-kern_env.d grub_emu-kern_err.d grub_emu-normal_execute.d grub_emu-kern_file.d grub_emu-kern_fs.d grub_emu-normal_lexer.d grub_emu-kern_loader.d grub_emu-kern_main.d grub_emu-kern_misc.d grub_emu-kern_parser.d grub_emu-grub_script_tab.d grub_emu-kern_partition.d grub_emu-kern_rescue.d grub_emu-kern_term.d grub_emu-normal_arg.d grub_emu-normal_cmdline.d grub_emu-normal_command.d grub_emu-normal_function.d grub_emu-normal_completion.d grub_emu-normal_main.d grub_emu-normal_menu.d grub_emu-normal_menu_entry.d grub_emu-normal_misc.d grub_emu-normal_script.d grub_emu-partmap_amiga.d grub_emu-partmap_apple.d grub_emu-partmap_pc.d grub_emu-partmap_sun.d grub_emu-partmap_acorn.d grub_emu-partmap_gpt.d grub_emu-util_console.d grub_emu-util_grub_emu.d grub_emu-util_misc.d grub_emu-util_i386_pc_biosdisk.d grub_emu-util_i386_pc_getroot.d grub_emu-util_i386_pc_misc.d grub_emu-grub_emu_init.d

grub-emu: $(grub_emu_DEPENDENCIES) grub_emu-commands_boot.o grub_emu-commands_cat.o grub_emu-commands_cmp.o grub_emu-commands_configfile.o grub_emu-commands_echo.o grub_emu-commands_help.o grub_emu-commands_terminal.o grub_emu-commands_ls.o grub_emu-commands_test.o grub_emu-commands_search.o grub_emu-commands_blocklist.o grub_emu-commands_i386_pc_halt.o grub_emu-commands_i386_pc_reboot.o grub_emu-disk_loopback.o grub_emu-disk_raid.o grub_emu-disk_lvm.o grub_emu-fs_affs.o grub_emu-fs_ext2.o grub_emu-fs_fat.o grub_emu-fs_fshelp.o grub_emu-fs_hfs.o grub_emu-fs_iso9660.o grub_emu-fs_jfs.o grub_emu-fs_minix.o grub_emu-fs_sfs.o grub_emu-fs_ufs.o grub_emu-fs_xfs.o grub_emu-fs_hfsplus.o grub_emu-io_gzio.o grub_emu-kern_device.o grub_emu-kern_disk.o grub_emu-kern_dl.o grub_emu-kern_elf.o grub_emu-kern_env.o grub_emu-kern_err.o grub_emu-normal_execute.o grub_emu-kern_file.o grub_emu-kern_fs.o grub_emu-normal_lexer.o grub_emu-kern_loader.o grub_emu-kern_main.o grub_emu-kern_misc.o grub_emu-kern_parser.o grub_emu-grub_script_tab.o grub_emu-kern_partition.o grub_emu-kern_rescue.o grub_emu-kern_term.o grub_emu-normal_arg.o grub_emu-normal_cmdline.o grub_emu-normal_command.o grub_emu-normal_function.o grub_emu-normal_completion.o grub_emu-normal_main.o grub_emu-normal_menu.o grub_emu-normal_menu_entry.o grub_emu-normal_misc.o grub_emu-normal_script.o grub_emu-partmap_amiga.o grub_emu-partmap_apple.o grub_emu-partmap_pc.o grub_emu-partmap_sun.o grub_emu-partmap_acorn.o grub_emu-partmap_gpt.o grub_emu-util_console.o grub_emu-util_grub_emu.o grub_emu-util_misc.o grub_emu-util_i386_pc_biosdisk.o grub_emu-util_i386_pc_getroot.o grub_emu-util_i386_pc_misc.o grub_emu-grub_emu_init.o
	$(CC) -o $@ grub_emu-commands_boot.o grub_emu-commands_cat.o grub_emu-commands_cmp.o grub_emu-commands_configfile.o grub_emu-commands_echo.o grub_emu-commands_help.o grub_emu-commands_terminal.o grub_emu-commands_ls.o grub_emu-commands_test.o grub_emu-commands_search.o grub_emu-commands_blocklist.o grub_emu-commands_i386_pc_halt.o grub_emu-commands_i386_pc_reboot.o grub_emu-disk_loopback.o grub_emu-disk_raid.o grub_emu-disk_lvm.o grub_emu-fs_affs.o grub_emu-fs_ext2.o grub_emu-fs_fat.o grub_emu-fs_fshelp.o grub_emu-fs_hfs.o grub_emu-fs_iso9660.o grub_emu-fs_jfs.o grub_emu-fs_minix.o grub_emu-fs_sfs.o grub_emu-fs_ufs.o grub_emu-fs_xfs.o grub_emu-fs_hfsplus.o grub_emu-io_gzio.o grub_emu-kern_device.o grub_emu-kern_disk.o grub_emu-kern_dl.o grub_emu-kern_elf.o grub_emu-kern_env.o grub_emu-kern_err.o grub_emu-normal_execute.o grub_emu-kern_file.o grub_emu-kern_fs.o grub_emu-normal_lexer.o grub_emu-kern_loader.o grub_emu-kern_main.o grub_emu-kern_misc.o grub_emu-kern_parser.o grub_emu-grub_script_tab.o grub_emu-kern_partition.o grub_emu-kern_rescue.o grub_emu-kern_term.o grub_emu-normal_arg.o grub_emu-normal_cmdline.o grub_emu-normal_command.o grub_emu-normal_function.o grub_emu-normal_completion.o grub_emu-normal_main.o grub_emu-normal_menu.o grub_emu-normal_menu_entry.o grub_emu-normal_misc.o grub_emu-normal_script.o grub_emu-partmap_amiga.o grub_emu-partmap_apple.o grub_emu-partmap_pc.o grub_emu-partmap_sun.o grub_emu-partmap_acorn.o grub_emu-partmap_gpt.o grub_emu-util_console.o grub_emu-util_grub_emu.o grub_emu-util_misc.o grub_emu-util_i386_pc_biosdisk.o grub_emu-util_i386_pc_getroot.o grub_emu-util_i386_pc_misc.o grub_emu-grub_emu_init.o $(LDFLAGS) $(grub_emu_LDFLAGS)

grub_emu-commands_boot.o: commands/boot.c
	$(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-commands_boot.d

grub_emu-commands_cat.o: commands/cat.c
	$(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-commands_cat.d

grub_emu-commands_cmp.o: commands/cmp.c
	$(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-commands_cmp.d

grub_emu-commands_configfile.o: commands/configfile.c
	$(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-commands_configfile.d

grub_emu-commands_echo.o: commands/echo.c
	$(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-commands_echo.d

grub_emu-commands_help.o: commands/help.c
	$(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-commands_help.d

grub_emu-commands_terminal.o: commands/terminal.c
	$(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-commands_terminal.d

grub_emu-commands_ls.o: commands/ls.c
	$(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-commands_ls.d

grub_emu-commands_test.o: commands/test.c
	$(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-commands_test.d

grub_emu-commands_search.o: commands/search.c
	$(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-commands_search.d

grub_emu-commands_blocklist.o: commands/blocklist.c
	$(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-commands_blocklist.d

grub_emu-commands_i386_pc_halt.o: commands/i386/pc/halt.c
	$(CC) -Icommands/i386/pc -I$(srcdir)/commands/i386/pc $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-commands_i386_pc_halt.d

grub_emu-commands_i386_pc_reboot.o: commands/i386/pc/reboot.c
	$(CC) -Icommands/i386/pc -I$(srcdir)/commands/i386/pc $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-commands_i386_pc_reboot.d

grub_emu-disk_loopback.o: disk/loopback.c
	$(CC) -Idisk -I$(srcdir)/disk $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-disk_loopback.d

grub_emu-disk_raid.o: disk/raid.c
	$(CC) -Idisk -I$(srcdir)/disk $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-disk_raid.d

grub_emu-disk_lvm.o: disk/lvm.c
	$(CC) -Idisk -I$(srcdir)/disk $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-disk_lvm.d

grub_emu-fs_affs.o: fs/affs.c
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-fs_affs.d

grub_emu-fs_ext2.o: fs/ext2.c
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-fs_ext2.d

grub_emu-fs_fat.o: fs/fat.c
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-fs_fat.d

grub_emu-fs_fshelp.o: fs/fshelp.c
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-fs_fshelp.d

grub_emu-fs_hfs.o: fs/hfs.c
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-fs_hfs.d

grub_emu-fs_iso9660.o: fs/iso9660.c
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-fs_iso9660.d

grub_emu-fs_jfs.o: fs/jfs.c
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-fs_jfs.d

grub_emu-fs_minix.o: fs/minix.c
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-fs_minix.d

grub_emu-fs_sfs.o: fs/sfs.c
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-fs_sfs.d

grub_emu-fs_ufs.o: fs/ufs.c
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-fs_ufs.d

grub_emu-fs_xfs.o: fs/xfs.c
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-fs_xfs.d

grub_emu-fs_hfsplus.o: fs/hfsplus.c
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-fs_hfsplus.d

grub_emu-io_gzio.o: io/gzio.c
	$(CC) -Iio -I$(srcdir)/io $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-io_gzio.d

grub_emu-kern_device.o: kern/device.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-kern_device.d

grub_emu-kern_disk.o: kern/disk.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-kern_disk.d

grub_emu-kern_dl.o: kern/dl.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-kern_dl.d

grub_emu-kern_elf.o: kern/elf.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-kern_elf.d

grub_emu-kern_env.o: kern/env.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-kern_env.d

grub_emu-kern_err.o: kern/err.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-kern_err.d

grub_emu-normal_execute.o: normal/execute.c
	$(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-normal_execute.d

grub_emu-kern_file.o: kern/file.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-kern_file.d

grub_emu-kern_fs.o: kern/fs.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-kern_fs.d

grub_emu-normal_lexer.o: normal/lexer.c
	$(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-normal_lexer.d

grub_emu-kern_loader.o: kern/loader.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-kern_loader.d

grub_emu-kern_main.o: kern/main.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-kern_main.d

grub_emu-kern_misc.o: kern/misc.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-kern_misc.d

grub_emu-kern_parser.o: kern/parser.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-kern_parser.d

grub_emu-grub_script_tab.o: grub_script.tab.c
	$(CC) -I. -I$(srcdir)/. $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-grub_script_tab.d

grub_emu-kern_partition.o: kern/partition.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-kern_partition.d

grub_emu-kern_rescue.o: kern/rescue.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-kern_rescue.d

grub_emu-kern_term.o: kern/term.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-kern_term.d

grub_emu-normal_arg.o: normal/arg.c
	$(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-normal_arg.d

grub_emu-normal_cmdline.o: normal/cmdline.c
	$(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-normal_cmdline.d

grub_emu-normal_command.o: normal/command.c
	$(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-normal_command.d

grub_emu-normal_function.o: normal/function.c
	$(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-normal_function.d

grub_emu-normal_completion.o: normal/completion.c
	$(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-normal_completion.d

grub_emu-normal_main.o: normal/main.c
	$(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-normal_main.d

grub_emu-normal_menu.o: normal/menu.c
	$(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-normal_menu.d

grub_emu-normal_menu_entry.o: normal/menu_entry.c
	$(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-normal_menu_entry.d

grub_emu-normal_misc.o: normal/misc.c
	$(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-normal_misc.d

grub_emu-normal_script.o: normal/script.c
	$(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-normal_script.d

grub_emu-partmap_amiga.o: partmap/amiga.c
	$(CC) -Ipartmap -I$(srcdir)/partmap $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-partmap_amiga.d

grub_emu-partmap_apple.o: partmap/apple.c
	$(CC) -Ipartmap -I$(srcdir)/partmap $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-partmap_apple.d

grub_emu-partmap_pc.o: partmap/pc.c
	$(CC) -Ipartmap -I$(srcdir)/partmap $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-partmap_pc.d

grub_emu-partmap_sun.o: partmap/sun.c
	$(CC) -Ipartmap -I$(srcdir)/partmap $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-partmap_sun.d

grub_emu-partmap_acorn.o: partmap/acorn.c
	$(CC) -Ipartmap -I$(srcdir)/partmap $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-partmap_acorn.d

grub_emu-partmap_gpt.o: partmap/gpt.c
	$(CC) -Ipartmap -I$(srcdir)/partmap $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-partmap_gpt.d

grub_emu-util_console.o: util/console.c
	$(CC) -Iutil -I$(srcdir)/util $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-util_console.d

grub_emu-util_grub_emu.o: util/grub-emu.c
	$(CC) -Iutil -I$(srcdir)/util $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-util_grub_emu.d

grub_emu-util_misc.o: util/misc.c
	$(CC) -Iutil -I$(srcdir)/util $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-util_misc.d

grub_emu-util_i386_pc_biosdisk.o: util/i386/pc/biosdisk.c
	$(CC) -Iutil/i386/pc -I$(srcdir)/util/i386/pc $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-util_i386_pc_biosdisk.d

grub_emu-util_i386_pc_getroot.o: util/i386/pc/getroot.c
	$(CC) -Iutil/i386/pc -I$(srcdir)/util/i386/pc $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-util_i386_pc_getroot.d

grub_emu-util_i386_pc_misc.o: util/i386/pc/misc.c
	$(CC) -Iutil/i386/pc -I$(srcdir)/util/i386/pc $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-util_i386_pc_misc.d

grub_emu-grub_emu_init.o: grub_emu_init.c
	$(CC) -I. -I$(srcdir)/. $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -MD -c -o $@ $<
-include grub_emu-grub_emu_init.d


grub_emu_LDFLAGS = $(LIBCURSES)

# Scripts.
sbin_SCRIPTS = grub-install

# For grub-install.
grub_install_SOURCES = util/i386/pc/grub-install.in
CLEANFILES += grub-install

grub-install: util/i386/pc/grub-install.in config.status
	./config.status --file=grub-install:util/i386/pc/grub-install.in
	chmod +x $@


# Modules.
pkgdata_MODULES = _chain.mod _linux.mod linux.mod normal.mod \
	_multiboot.mod chain.mod multiboot.mod reboot.mod halt.mod	\
	vbe.mod vbetest.mod vbeinfo.mod video.mod gfxterm.mod \
	videotest.mod play.mod bitmap.mod tga.mod

# For _chain.mod.
_chain_mod_SOURCES = loader/i386/pc/chainloader.c
CLEANFILES += _chain.mod mod-_chain.o mod-_chain.c pre-_chain.o _chain_mod-loader_i386_pc_chainloader.o und-_chain.lst
ifneq ($(_chain_mod_EXPORTS),no)
CLEANFILES += def-_chain.lst
DEFSYMFILES += def-_chain.lst
endif
MOSTLYCLEANFILES += _chain_mod-loader_i386_pc_chainloader.d
UNDSYMFILES += und-_chain.lst

_chain.mod: pre-_chain.o mod-_chain.o
	-rm -f $@
	$(TARGET_CC) $(_chain_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-_chain.o: $(_chain_mod_DEPENDENCIES) _chain_mod-loader_i386_pc_chainloader.o
	-rm -f $@
	$(TARGET_CC) $(_chain_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ _chain_mod-loader_i386_pc_chainloader.o

mod-_chain.o: mod-_chain.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(_chain_mod_CFLAGS) -c -o $@ $<

mod-_chain.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh '_chain' $< > $@ || (rm -f $@; exit 1)

ifneq ($(_chain_mod_EXPORTS),no)
def-_chain.lst: pre-_chain.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 _chain/' > $@
endif

und-_chain.lst: pre-_chain.o
	echo '_chain' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

_chain_mod-loader_i386_pc_chainloader.o: loader/i386/pc/chainloader.c
	$(TARGET_CC) -Iloader/i386/pc -I$(srcdir)/loader/i386/pc $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(_chain_mod_CFLAGS) -MD -c -o $@ $<
-include _chain_mod-loader_i386_pc_chainloader.d

CLEANFILES += cmd-_chain_mod-loader_i386_pc_chainloader.lst fs-_chain_mod-loader_i386_pc_chainloader.lst
COMMANDFILES += cmd-_chain_mod-loader_i386_pc_chainloader.lst
FSFILES += fs-_chain_mod-loader_i386_pc_chainloader.lst

cmd-_chain_mod-loader_i386_pc_chainloader.lst: loader/i386/pc/chainloader.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Iloader/i386/pc -I$(srcdir)/loader/i386/pc $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(_chain_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh _chain > $@ || (rm -f $@; exit 1)

fs-_chain_mod-loader_i386_pc_chainloader.lst: loader/i386/pc/chainloader.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Iloader/i386/pc -I$(srcdir)/loader/i386/pc $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(_chain_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh _chain > $@ || (rm -f $@; exit 1)


_chain_mod_CFLAGS = $(COMMON_CFLAGS)
_chain_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For chain.mod.
chain_mod_SOURCES = loader/i386/pc/chainloader_normal.c
CLEANFILES += chain.mod mod-chain.o mod-chain.c pre-chain.o chain_mod-loader_i386_pc_chainloader_normal.o und-chain.lst
ifneq ($(chain_mod_EXPORTS),no)
CLEANFILES += def-chain.lst
DEFSYMFILES += def-chain.lst
endif
MOSTLYCLEANFILES += chain_mod-loader_i386_pc_chainloader_normal.d
UNDSYMFILES += und-chain.lst

chain.mod: pre-chain.o mod-chain.o
	-rm -f $@
	$(TARGET_CC) $(chain_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-chain.o: $(chain_mod_DEPENDENCIES) chain_mod-loader_i386_pc_chainloader_normal.o
	-rm -f $@
	$(TARGET_CC) $(chain_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ chain_mod-loader_i386_pc_chainloader_normal.o

mod-chain.o: mod-chain.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(chain_mod_CFLAGS) -c -o $@ $<

mod-chain.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'chain' $< > $@ || (rm -f $@; exit 1)

ifneq ($(chain_mod_EXPORTS),no)
def-chain.lst: pre-chain.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 chain/' > $@
endif

und-chain.lst: pre-chain.o
	echo 'chain' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

chain_mod-loader_i386_pc_chainloader_normal.o: loader/i386/pc/chainloader_normal.c
	$(TARGET_CC) -Iloader/i386/pc -I$(srcdir)/loader/i386/pc $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(chain_mod_CFLAGS) -MD -c -o $@ $<
-include chain_mod-loader_i386_pc_chainloader_normal.d

CLEANFILES += cmd-chain_mod-loader_i386_pc_chainloader_normal.lst fs-chain_mod-loader_i386_pc_chainloader_normal.lst
COMMANDFILES += cmd-chain_mod-loader_i386_pc_chainloader_normal.lst
FSFILES += fs-chain_mod-loader_i386_pc_chainloader_normal.lst

cmd-chain_mod-loader_i386_pc_chainloader_normal.lst: loader/i386/pc/chainloader_normal.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Iloader/i386/pc -I$(srcdir)/loader/i386/pc $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(chain_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh chain > $@ || (rm -f $@; exit 1)

fs-chain_mod-loader_i386_pc_chainloader_normal.lst: loader/i386/pc/chainloader_normal.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Iloader/i386/pc -I$(srcdir)/loader/i386/pc $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(chain_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh chain > $@ || (rm -f $@; exit 1)


chain_mod_CFLAGS = $(COMMON_CFLAGS)
chain_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For _linux.mod.
_linux_mod_SOURCES = loader/i386/pc/linux.c
CLEANFILES += _linux.mod mod-_linux.o mod-_linux.c pre-_linux.o _linux_mod-loader_i386_pc_linux.o und-_linux.lst
ifneq ($(_linux_mod_EXPORTS),no)
CLEANFILES += def-_linux.lst
DEFSYMFILES += def-_linux.lst
endif
MOSTLYCLEANFILES += _linux_mod-loader_i386_pc_linux.d
UNDSYMFILES += und-_linux.lst

_linux.mod: pre-_linux.o mod-_linux.o
	-rm -f $@
	$(TARGET_CC) $(_linux_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-_linux.o: $(_linux_mod_DEPENDENCIES) _linux_mod-loader_i386_pc_linux.o
	-rm -f $@
	$(TARGET_CC) $(_linux_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ _linux_mod-loader_i386_pc_linux.o

mod-_linux.o: mod-_linux.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(_linux_mod_CFLAGS) -c -o $@ $<

mod-_linux.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh '_linux' $< > $@ || (rm -f $@; exit 1)

ifneq ($(_linux_mod_EXPORTS),no)
def-_linux.lst: pre-_linux.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 _linux/' > $@
endif

und-_linux.lst: pre-_linux.o
	echo '_linux' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

_linux_mod-loader_i386_pc_linux.o: loader/i386/pc/linux.c
	$(TARGET_CC) -Iloader/i386/pc -I$(srcdir)/loader/i386/pc $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(_linux_mod_CFLAGS) -MD -c -o $@ $<
-include _linux_mod-loader_i386_pc_linux.d

CLEANFILES += cmd-_linux_mod-loader_i386_pc_linux.lst fs-_linux_mod-loader_i386_pc_linux.lst
COMMANDFILES += cmd-_linux_mod-loader_i386_pc_linux.lst
FSFILES += fs-_linux_mod-loader_i386_pc_linux.lst

cmd-_linux_mod-loader_i386_pc_linux.lst: loader/i386/pc/linux.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Iloader/i386/pc -I$(srcdir)/loader/i386/pc $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(_linux_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh _linux > $@ || (rm -f $@; exit 1)

fs-_linux_mod-loader_i386_pc_linux.lst: loader/i386/pc/linux.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Iloader/i386/pc -I$(srcdir)/loader/i386/pc $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(_linux_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh _linux > $@ || (rm -f $@; exit 1)


_linux_mod_CFLAGS = $(COMMON_CFLAGS)
_linux_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For linux.mod.
linux_mod_SOURCES = loader/i386/pc/linux_normal.c
CLEANFILES += linux.mod mod-linux.o mod-linux.c pre-linux.o linux_mod-loader_i386_pc_linux_normal.o und-linux.lst
ifneq ($(linux_mod_EXPORTS),no)
CLEANFILES += def-linux.lst
DEFSYMFILES += def-linux.lst
endif
MOSTLYCLEANFILES += linux_mod-loader_i386_pc_linux_normal.d
UNDSYMFILES += und-linux.lst

linux.mod: pre-linux.o mod-linux.o
	-rm -f $@
	$(TARGET_CC) $(linux_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-linux.o: $(linux_mod_DEPENDENCIES) linux_mod-loader_i386_pc_linux_normal.o
	-rm -f $@
	$(TARGET_CC) $(linux_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ linux_mod-loader_i386_pc_linux_normal.o

mod-linux.o: mod-linux.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(linux_mod_CFLAGS) -c -o $@ $<

mod-linux.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'linux' $< > $@ || (rm -f $@; exit 1)

ifneq ($(linux_mod_EXPORTS),no)
def-linux.lst: pre-linux.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 linux/' > $@
endif

und-linux.lst: pre-linux.o
	echo 'linux' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

linux_mod-loader_i386_pc_linux_normal.o: loader/i386/pc/linux_normal.c
	$(TARGET_CC) -Iloader/i386/pc -I$(srcdir)/loader/i386/pc $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(linux_mod_CFLAGS) -MD -c -o $@ $<
-include linux_mod-loader_i386_pc_linux_normal.d

CLEANFILES += cmd-linux_mod-loader_i386_pc_linux_normal.lst fs-linux_mod-loader_i386_pc_linux_normal.lst
COMMANDFILES += cmd-linux_mod-loader_i386_pc_linux_normal.lst
FSFILES += fs-linux_mod-loader_i386_pc_linux_normal.lst

cmd-linux_mod-loader_i386_pc_linux_normal.lst: loader/i386/pc/linux_normal.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Iloader/i386/pc -I$(srcdir)/loader/i386/pc $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(linux_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh linux > $@ || (rm -f $@; exit 1)

fs-linux_mod-loader_i386_pc_linux_normal.lst: loader/i386/pc/linux_normal.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Iloader/i386/pc -I$(srcdir)/loader/i386/pc $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(linux_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh linux > $@ || (rm -f $@; exit 1)


linux_mod_CFLAGS = $(COMMON_CFLAGS)
linux_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For normal.mod.
normal_mod_DEPENDENCIES = grub_script.tab.c grub_script.tab.h
normal_mod_SOURCES = normal/arg.c normal/cmdline.c normal/command.c	\
	normal/completion.c normal/execute.c		 		\
	normal/function.c normal/lexer.c normal/main.c normal/menu.c	\
	normal/menu_entry.c normal/misc.c grub_script.tab.c 		\
	normal/script.c normal/i386/setjmp.S
CLEANFILES += normal.mod mod-normal.o mod-normal.c pre-normal.o normal_mod-normal_arg.o normal_mod-normal_cmdline.o normal_mod-normal_command.o normal_mod-normal_completion.o normal_mod-normal_execute.o normal_mod-normal_function.o normal_mod-normal_lexer.o normal_mod-normal_main.o normal_mod-normal_menu.o normal_mod-normal_menu_entry.o normal_mod-normal_misc.o normal_mod-grub_script_tab.o normal_mod-normal_script.o normal_mod-normal_i386_setjmp.o und-normal.lst
ifneq ($(normal_mod_EXPORTS),no)
CLEANFILES += def-normal.lst
DEFSYMFILES += def-normal.lst
endif
MOSTLYCLEANFILES += normal_mod-normal_arg.d normal_mod-normal_cmdline.d normal_mod-normal_command.d normal_mod-normal_completion.d normal_mod-normal_execute.d normal_mod-normal_function.d normal_mod-normal_lexer.d normal_mod-normal_main.d normal_mod-normal_menu.d normal_mod-normal_menu_entry.d normal_mod-normal_misc.d normal_mod-grub_script_tab.d normal_mod-normal_script.d normal_mod-normal_i386_setjmp.d
UNDSYMFILES += und-normal.lst

normal.mod: pre-normal.o mod-normal.o
	-rm -f $@
	$(TARGET_CC) $(normal_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-normal.o: $(normal_mod_DEPENDENCIES) normal_mod-normal_arg.o normal_mod-normal_cmdline.o normal_mod-normal_command.o normal_mod-normal_completion.o normal_mod-normal_execute.o normal_mod-normal_function.o normal_mod-normal_lexer.o normal_mod-normal_main.o normal_mod-normal_menu.o normal_mod-normal_menu_entry.o normal_mod-normal_misc.o normal_mod-grub_script_tab.o normal_mod-normal_script.o normal_mod-normal_i386_setjmp.o
	-rm -f $@
	$(TARGET_CC) $(normal_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ normal_mod-normal_arg.o normal_mod-normal_cmdline.o normal_mod-normal_command.o normal_mod-normal_completion.o normal_mod-normal_execute.o normal_mod-normal_function.o normal_mod-normal_lexer.o normal_mod-normal_main.o normal_mod-normal_menu.o normal_mod-normal_menu_entry.o normal_mod-normal_misc.o normal_mod-grub_script_tab.o normal_mod-normal_script.o normal_mod-normal_i386_setjmp.o

mod-normal.o: mod-normal.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -c -o $@ $<

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
	$(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -MD -c -o $@ $<
-include normal_mod-normal_arg.d

CLEANFILES += cmd-normal_mod-normal_arg.lst fs-normal_mod-normal_arg.lst
COMMANDFILES += cmd-normal_mod-normal_arg.lst
FSFILES += fs-normal_mod-normal_arg.lst

cmd-normal_mod-normal_arg.lst: normal/arg.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-normal_arg.lst: normal/arg.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_cmdline.o: normal/cmdline.c
	$(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -MD -c -o $@ $<
-include normal_mod-normal_cmdline.d

CLEANFILES += cmd-normal_mod-normal_cmdline.lst fs-normal_mod-normal_cmdline.lst
COMMANDFILES += cmd-normal_mod-normal_cmdline.lst
FSFILES += fs-normal_mod-normal_cmdline.lst

cmd-normal_mod-normal_cmdline.lst: normal/cmdline.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-normal_cmdline.lst: normal/cmdline.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_command.o: normal/command.c
	$(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -MD -c -o $@ $<
-include normal_mod-normal_command.d

CLEANFILES += cmd-normal_mod-normal_command.lst fs-normal_mod-normal_command.lst
COMMANDFILES += cmd-normal_mod-normal_command.lst
FSFILES += fs-normal_mod-normal_command.lst

cmd-normal_mod-normal_command.lst: normal/command.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-normal_command.lst: normal/command.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_completion.o: normal/completion.c
	$(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -MD -c -o $@ $<
-include normal_mod-normal_completion.d

CLEANFILES += cmd-normal_mod-normal_completion.lst fs-normal_mod-normal_completion.lst
COMMANDFILES += cmd-normal_mod-normal_completion.lst
FSFILES += fs-normal_mod-normal_completion.lst

cmd-normal_mod-normal_completion.lst: normal/completion.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-normal_completion.lst: normal/completion.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_execute.o: normal/execute.c
	$(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -MD -c -o $@ $<
-include normal_mod-normal_execute.d

CLEANFILES += cmd-normal_mod-normal_execute.lst fs-normal_mod-normal_execute.lst
COMMANDFILES += cmd-normal_mod-normal_execute.lst
FSFILES += fs-normal_mod-normal_execute.lst

cmd-normal_mod-normal_execute.lst: normal/execute.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-normal_execute.lst: normal/execute.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_function.o: normal/function.c
	$(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -MD -c -o $@ $<
-include normal_mod-normal_function.d

CLEANFILES += cmd-normal_mod-normal_function.lst fs-normal_mod-normal_function.lst
COMMANDFILES += cmd-normal_mod-normal_function.lst
FSFILES += fs-normal_mod-normal_function.lst

cmd-normal_mod-normal_function.lst: normal/function.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-normal_function.lst: normal/function.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_lexer.o: normal/lexer.c
	$(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -MD -c -o $@ $<
-include normal_mod-normal_lexer.d

CLEANFILES += cmd-normal_mod-normal_lexer.lst fs-normal_mod-normal_lexer.lst
COMMANDFILES += cmd-normal_mod-normal_lexer.lst
FSFILES += fs-normal_mod-normal_lexer.lst

cmd-normal_mod-normal_lexer.lst: normal/lexer.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-normal_lexer.lst: normal/lexer.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_main.o: normal/main.c
	$(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -MD -c -o $@ $<
-include normal_mod-normal_main.d

CLEANFILES += cmd-normal_mod-normal_main.lst fs-normal_mod-normal_main.lst
COMMANDFILES += cmd-normal_mod-normal_main.lst
FSFILES += fs-normal_mod-normal_main.lst

cmd-normal_mod-normal_main.lst: normal/main.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-normal_main.lst: normal/main.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_menu.o: normal/menu.c
	$(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -MD -c -o $@ $<
-include normal_mod-normal_menu.d

CLEANFILES += cmd-normal_mod-normal_menu.lst fs-normal_mod-normal_menu.lst
COMMANDFILES += cmd-normal_mod-normal_menu.lst
FSFILES += fs-normal_mod-normal_menu.lst

cmd-normal_mod-normal_menu.lst: normal/menu.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-normal_menu.lst: normal/menu.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_menu_entry.o: normal/menu_entry.c
	$(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -MD -c -o $@ $<
-include normal_mod-normal_menu_entry.d

CLEANFILES += cmd-normal_mod-normal_menu_entry.lst fs-normal_mod-normal_menu_entry.lst
COMMANDFILES += cmd-normal_mod-normal_menu_entry.lst
FSFILES += fs-normal_mod-normal_menu_entry.lst

cmd-normal_mod-normal_menu_entry.lst: normal/menu_entry.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-normal_menu_entry.lst: normal/menu_entry.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_misc.o: normal/misc.c
	$(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -MD -c -o $@ $<
-include normal_mod-normal_misc.d

CLEANFILES += cmd-normal_mod-normal_misc.lst fs-normal_mod-normal_misc.lst
COMMANDFILES += cmd-normal_mod-normal_misc.lst
FSFILES += fs-normal_mod-normal_misc.lst

cmd-normal_mod-normal_misc.lst: normal/misc.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-normal_misc.lst: normal/misc.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-grub_script_tab.o: grub_script.tab.c
	$(TARGET_CC) -I. -I$(srcdir)/. $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -MD -c -o $@ $<
-include normal_mod-grub_script_tab.d

CLEANFILES += cmd-normal_mod-grub_script_tab.lst fs-normal_mod-grub_script_tab.lst
COMMANDFILES += cmd-normal_mod-grub_script_tab.lst
FSFILES += fs-normal_mod-grub_script_tab.lst

cmd-normal_mod-grub_script_tab.lst: grub_script.tab.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -I. -I$(srcdir)/. $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-grub_script_tab.lst: grub_script.tab.c genfslist.sh
	set -e; 	  $(TARGET_CC) -I. -I$(srcdir)/. $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_script.o: normal/script.c
	$(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -MD -c -o $@ $<
-include normal_mod-normal_script.d

CLEANFILES += cmd-normal_mod-normal_script.lst fs-normal_mod-normal_script.lst
COMMANDFILES += cmd-normal_mod-normal_script.lst
FSFILES += fs-normal_mod-normal_script.lst

cmd-normal_mod-normal_script.lst: normal/script.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-normal_script.lst: normal/script.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_i386_setjmp.o: normal/i386/setjmp.S
	$(TARGET_CC) -Inormal/i386 -I$(srcdir)/normal/i386 $(TARGET_CPPFLAGS) $(TARGET_ASFLAGS) $(normal_mod_ASFLAGS) -MD -c -o $@ $<
-include normal_mod-normal_i386_setjmp.d

CLEANFILES += cmd-normal_mod-normal_i386_setjmp.lst fs-normal_mod-normal_i386_setjmp.lst
COMMANDFILES += cmd-normal_mod-normal_i386_setjmp.lst
FSFILES += fs-normal_mod-normal_i386_setjmp.lst

cmd-normal_mod-normal_i386_setjmp.lst: normal/i386/setjmp.S gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Inormal/i386 -I$(srcdir)/normal/i386 $(TARGET_CPPFLAGS) $(TARGET_ASFLAGS) $(normal_mod_ASFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-normal_i386_setjmp.lst: normal/i386/setjmp.S genfslist.sh
	set -e; 	  $(TARGET_CC) -Inormal/i386 -I$(srcdir)/normal/i386 $(TARGET_CPPFLAGS) $(TARGET_ASFLAGS) $(normal_mod_ASFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod_CFLAGS = $(COMMON_CFLAGS)
normal_mod_ASFLAGS = $(COMMON_ASFLAGS)
normal_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For reboot.mod.
reboot_mod_SOURCES = commands/i386/pc/reboot.c
CLEANFILES += reboot.mod mod-reboot.o mod-reboot.c pre-reboot.o reboot_mod-commands_i386_pc_reboot.o und-reboot.lst
ifneq ($(reboot_mod_EXPORTS),no)
CLEANFILES += def-reboot.lst
DEFSYMFILES += def-reboot.lst
endif
MOSTLYCLEANFILES += reboot_mod-commands_i386_pc_reboot.d
UNDSYMFILES += und-reboot.lst

reboot.mod: pre-reboot.o mod-reboot.o
	-rm -f $@
	$(TARGET_CC) $(reboot_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-reboot.o: $(reboot_mod_DEPENDENCIES) reboot_mod-commands_i386_pc_reboot.o
	-rm -f $@
	$(TARGET_CC) $(reboot_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ reboot_mod-commands_i386_pc_reboot.o

mod-reboot.o: mod-reboot.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(reboot_mod_CFLAGS) -c -o $@ $<

mod-reboot.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'reboot' $< > $@ || (rm -f $@; exit 1)

ifneq ($(reboot_mod_EXPORTS),no)
def-reboot.lst: pre-reboot.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 reboot/' > $@
endif

und-reboot.lst: pre-reboot.o
	echo 'reboot' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

reboot_mod-commands_i386_pc_reboot.o: commands/i386/pc/reboot.c
	$(TARGET_CC) -Icommands/i386/pc -I$(srcdir)/commands/i386/pc $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(reboot_mod_CFLAGS) -MD -c -o $@ $<
-include reboot_mod-commands_i386_pc_reboot.d

CLEANFILES += cmd-reboot_mod-commands_i386_pc_reboot.lst fs-reboot_mod-commands_i386_pc_reboot.lst
COMMANDFILES += cmd-reboot_mod-commands_i386_pc_reboot.lst
FSFILES += fs-reboot_mod-commands_i386_pc_reboot.lst

cmd-reboot_mod-commands_i386_pc_reboot.lst: commands/i386/pc/reboot.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Icommands/i386/pc -I$(srcdir)/commands/i386/pc $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(reboot_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh reboot > $@ || (rm -f $@; exit 1)

fs-reboot_mod-commands_i386_pc_reboot.lst: commands/i386/pc/reboot.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Icommands/i386/pc -I$(srcdir)/commands/i386/pc $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(reboot_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh reboot > $@ || (rm -f $@; exit 1)


reboot_mod_CFLAGS = $(COMMON_CFLAGS)
reboot_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For halt.mod.
halt_mod_SOURCES = commands/i386/pc/halt.c
CLEANFILES += halt.mod mod-halt.o mod-halt.c pre-halt.o halt_mod-commands_i386_pc_halt.o und-halt.lst
ifneq ($(halt_mod_EXPORTS),no)
CLEANFILES += def-halt.lst
DEFSYMFILES += def-halt.lst
endif
MOSTLYCLEANFILES += halt_mod-commands_i386_pc_halt.d
UNDSYMFILES += und-halt.lst

halt.mod: pre-halt.o mod-halt.o
	-rm -f $@
	$(TARGET_CC) $(halt_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-halt.o: $(halt_mod_DEPENDENCIES) halt_mod-commands_i386_pc_halt.o
	-rm -f $@
	$(TARGET_CC) $(halt_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ halt_mod-commands_i386_pc_halt.o

mod-halt.o: mod-halt.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(halt_mod_CFLAGS) -c -o $@ $<

mod-halt.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'halt' $< > $@ || (rm -f $@; exit 1)

ifneq ($(halt_mod_EXPORTS),no)
def-halt.lst: pre-halt.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 halt/' > $@
endif

und-halt.lst: pre-halt.o
	echo 'halt' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

halt_mod-commands_i386_pc_halt.o: commands/i386/pc/halt.c
	$(TARGET_CC) -Icommands/i386/pc -I$(srcdir)/commands/i386/pc $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(halt_mod_CFLAGS) -MD -c -o $@ $<
-include halt_mod-commands_i386_pc_halt.d

CLEANFILES += cmd-halt_mod-commands_i386_pc_halt.lst fs-halt_mod-commands_i386_pc_halt.lst
COMMANDFILES += cmd-halt_mod-commands_i386_pc_halt.lst
FSFILES += fs-halt_mod-commands_i386_pc_halt.lst

cmd-halt_mod-commands_i386_pc_halt.lst: commands/i386/pc/halt.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Icommands/i386/pc -I$(srcdir)/commands/i386/pc $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(halt_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh halt > $@ || (rm -f $@; exit 1)

fs-halt_mod-commands_i386_pc_halt.lst: commands/i386/pc/halt.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Icommands/i386/pc -I$(srcdir)/commands/i386/pc $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(halt_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh halt > $@ || (rm -f $@; exit 1)


halt_mod_CFLAGS = $(COMMON_CFLAGS)
halt_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For serial.mod.
serial_mod_SOURCES = term/i386/pc/serial.c
serial_mod_CFLAGS = $(COMMON_CFLAGS)
serial_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For _multiboot.mod.
_multiboot_mod_SOURCES = loader/i386/pc/multiboot.c
CLEANFILES += _multiboot.mod mod-_multiboot.o mod-_multiboot.c pre-_multiboot.o _multiboot_mod-loader_i386_pc_multiboot.o und-_multiboot.lst
ifneq ($(_multiboot_mod_EXPORTS),no)
CLEANFILES += def-_multiboot.lst
DEFSYMFILES += def-_multiboot.lst
endif
MOSTLYCLEANFILES += _multiboot_mod-loader_i386_pc_multiboot.d
UNDSYMFILES += und-_multiboot.lst

_multiboot.mod: pre-_multiboot.o mod-_multiboot.o
	-rm -f $@
	$(TARGET_CC) $(_multiboot_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-_multiboot.o: $(_multiboot_mod_DEPENDENCIES) _multiboot_mod-loader_i386_pc_multiboot.o
	-rm -f $@
	$(TARGET_CC) $(_multiboot_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ _multiboot_mod-loader_i386_pc_multiboot.o

mod-_multiboot.o: mod-_multiboot.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(_multiboot_mod_CFLAGS) -c -o $@ $<

mod-_multiboot.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh '_multiboot' $< > $@ || (rm -f $@; exit 1)

ifneq ($(_multiboot_mod_EXPORTS),no)
def-_multiboot.lst: pre-_multiboot.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 _multiboot/' > $@
endif

und-_multiboot.lst: pre-_multiboot.o
	echo '_multiboot' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

_multiboot_mod-loader_i386_pc_multiboot.o: loader/i386/pc/multiboot.c
	$(TARGET_CC) -Iloader/i386/pc -I$(srcdir)/loader/i386/pc $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(_multiboot_mod_CFLAGS) -MD -c -o $@ $<
-include _multiboot_mod-loader_i386_pc_multiboot.d

CLEANFILES += cmd-_multiboot_mod-loader_i386_pc_multiboot.lst fs-_multiboot_mod-loader_i386_pc_multiboot.lst
COMMANDFILES += cmd-_multiboot_mod-loader_i386_pc_multiboot.lst
FSFILES += fs-_multiboot_mod-loader_i386_pc_multiboot.lst

cmd-_multiboot_mod-loader_i386_pc_multiboot.lst: loader/i386/pc/multiboot.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Iloader/i386/pc -I$(srcdir)/loader/i386/pc $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(_multiboot_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh _multiboot > $@ || (rm -f $@; exit 1)

fs-_multiboot_mod-loader_i386_pc_multiboot.lst: loader/i386/pc/multiboot.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Iloader/i386/pc -I$(srcdir)/loader/i386/pc $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(_multiboot_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh _multiboot > $@ || (rm -f $@; exit 1)


_multiboot_mod_CFLAGS = $(COMMON_CFLAGS)
_multiboot_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For multiboot.mod.
multiboot_mod_SOURCES = loader/i386/pc/multiboot_normal.c
CLEANFILES += multiboot.mod mod-multiboot.o mod-multiboot.c pre-multiboot.o multiboot_mod-loader_i386_pc_multiboot_normal.o und-multiboot.lst
ifneq ($(multiboot_mod_EXPORTS),no)
CLEANFILES += def-multiboot.lst
DEFSYMFILES += def-multiboot.lst
endif
MOSTLYCLEANFILES += multiboot_mod-loader_i386_pc_multiboot_normal.d
UNDSYMFILES += und-multiboot.lst

multiboot.mod: pre-multiboot.o mod-multiboot.o
	-rm -f $@
	$(TARGET_CC) $(multiboot_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-multiboot.o: $(multiboot_mod_DEPENDENCIES) multiboot_mod-loader_i386_pc_multiboot_normal.o
	-rm -f $@
	$(TARGET_CC) $(multiboot_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ multiboot_mod-loader_i386_pc_multiboot_normal.o

mod-multiboot.o: mod-multiboot.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(multiboot_mod_CFLAGS) -c -o $@ $<

mod-multiboot.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'multiboot' $< > $@ || (rm -f $@; exit 1)

ifneq ($(multiboot_mod_EXPORTS),no)
def-multiboot.lst: pre-multiboot.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 multiboot/' > $@
endif

und-multiboot.lst: pre-multiboot.o
	echo 'multiboot' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

multiboot_mod-loader_i386_pc_multiboot_normal.o: loader/i386/pc/multiboot_normal.c
	$(TARGET_CC) -Iloader/i386/pc -I$(srcdir)/loader/i386/pc $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(multiboot_mod_CFLAGS) -MD -c -o $@ $<
-include multiboot_mod-loader_i386_pc_multiboot_normal.d

CLEANFILES += cmd-multiboot_mod-loader_i386_pc_multiboot_normal.lst fs-multiboot_mod-loader_i386_pc_multiboot_normal.lst
COMMANDFILES += cmd-multiboot_mod-loader_i386_pc_multiboot_normal.lst
FSFILES += fs-multiboot_mod-loader_i386_pc_multiboot_normal.lst

cmd-multiboot_mod-loader_i386_pc_multiboot_normal.lst: loader/i386/pc/multiboot_normal.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Iloader/i386/pc -I$(srcdir)/loader/i386/pc $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(multiboot_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh multiboot > $@ || (rm -f $@; exit 1)

fs-multiboot_mod-loader_i386_pc_multiboot_normal.lst: loader/i386/pc/multiboot_normal.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Iloader/i386/pc -I$(srcdir)/loader/i386/pc $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(multiboot_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh multiboot > $@ || (rm -f $@; exit 1)


multiboot_mod_CFLAGS = $(COMMON_CFLAGS)
multiboot_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For vbe.mod.
vbe_mod_SOURCES = video/i386/pc/vbe.c video/i386/pc/vbeblit.c \
		  video/i386/pc/vbefill.c video/i386/pc/vbeutil.c
CLEANFILES += vbe.mod mod-vbe.o mod-vbe.c pre-vbe.o vbe_mod-video_i386_pc_vbe.o vbe_mod-video_i386_pc_vbeblit.o vbe_mod-video_i386_pc_vbefill.o vbe_mod-video_i386_pc_vbeutil.o und-vbe.lst
ifneq ($(vbe_mod_EXPORTS),no)
CLEANFILES += def-vbe.lst
DEFSYMFILES += def-vbe.lst
endif
MOSTLYCLEANFILES += vbe_mod-video_i386_pc_vbe.d vbe_mod-video_i386_pc_vbeblit.d vbe_mod-video_i386_pc_vbefill.d vbe_mod-video_i386_pc_vbeutil.d
UNDSYMFILES += und-vbe.lst

vbe.mod: pre-vbe.o mod-vbe.o
	-rm -f $@
	$(TARGET_CC) $(vbe_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-vbe.o: $(vbe_mod_DEPENDENCIES) vbe_mod-video_i386_pc_vbe.o vbe_mod-video_i386_pc_vbeblit.o vbe_mod-video_i386_pc_vbefill.o vbe_mod-video_i386_pc_vbeutil.o
	-rm -f $@
	$(TARGET_CC) $(vbe_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ vbe_mod-video_i386_pc_vbe.o vbe_mod-video_i386_pc_vbeblit.o vbe_mod-video_i386_pc_vbefill.o vbe_mod-video_i386_pc_vbeutil.o

mod-vbe.o: mod-vbe.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(vbe_mod_CFLAGS) -c -o $@ $<

mod-vbe.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'vbe' $< > $@ || (rm -f $@; exit 1)

ifneq ($(vbe_mod_EXPORTS),no)
def-vbe.lst: pre-vbe.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 vbe/' > $@
endif

und-vbe.lst: pre-vbe.o
	echo 'vbe' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

vbe_mod-video_i386_pc_vbe.o: video/i386/pc/vbe.c
	$(TARGET_CC) -Ivideo/i386/pc -I$(srcdir)/video/i386/pc $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(vbe_mod_CFLAGS) -MD -c -o $@ $<
-include vbe_mod-video_i386_pc_vbe.d

CLEANFILES += cmd-vbe_mod-video_i386_pc_vbe.lst fs-vbe_mod-video_i386_pc_vbe.lst
COMMANDFILES += cmd-vbe_mod-video_i386_pc_vbe.lst
FSFILES += fs-vbe_mod-video_i386_pc_vbe.lst

cmd-vbe_mod-video_i386_pc_vbe.lst: video/i386/pc/vbe.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ivideo/i386/pc -I$(srcdir)/video/i386/pc $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(vbe_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh vbe > $@ || (rm -f $@; exit 1)

fs-vbe_mod-video_i386_pc_vbe.lst: video/i386/pc/vbe.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Ivideo/i386/pc -I$(srcdir)/video/i386/pc $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(vbe_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh vbe > $@ || (rm -f $@; exit 1)


vbe_mod-video_i386_pc_vbeblit.o: video/i386/pc/vbeblit.c
	$(TARGET_CC) -Ivideo/i386/pc -I$(srcdir)/video/i386/pc $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(vbe_mod_CFLAGS) -MD -c -o $@ $<
-include vbe_mod-video_i386_pc_vbeblit.d

CLEANFILES += cmd-vbe_mod-video_i386_pc_vbeblit.lst fs-vbe_mod-video_i386_pc_vbeblit.lst
COMMANDFILES += cmd-vbe_mod-video_i386_pc_vbeblit.lst
FSFILES += fs-vbe_mod-video_i386_pc_vbeblit.lst

cmd-vbe_mod-video_i386_pc_vbeblit.lst: video/i386/pc/vbeblit.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ivideo/i386/pc -I$(srcdir)/video/i386/pc $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(vbe_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh vbe > $@ || (rm -f $@; exit 1)

fs-vbe_mod-video_i386_pc_vbeblit.lst: video/i386/pc/vbeblit.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Ivideo/i386/pc -I$(srcdir)/video/i386/pc $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(vbe_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh vbe > $@ || (rm -f $@; exit 1)


vbe_mod-video_i386_pc_vbefill.o: video/i386/pc/vbefill.c
	$(TARGET_CC) -Ivideo/i386/pc -I$(srcdir)/video/i386/pc $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(vbe_mod_CFLAGS) -MD -c -o $@ $<
-include vbe_mod-video_i386_pc_vbefill.d

CLEANFILES += cmd-vbe_mod-video_i386_pc_vbefill.lst fs-vbe_mod-video_i386_pc_vbefill.lst
COMMANDFILES += cmd-vbe_mod-video_i386_pc_vbefill.lst
FSFILES += fs-vbe_mod-video_i386_pc_vbefill.lst

cmd-vbe_mod-video_i386_pc_vbefill.lst: video/i386/pc/vbefill.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ivideo/i386/pc -I$(srcdir)/video/i386/pc $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(vbe_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh vbe > $@ || (rm -f $@; exit 1)

fs-vbe_mod-video_i386_pc_vbefill.lst: video/i386/pc/vbefill.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Ivideo/i386/pc -I$(srcdir)/video/i386/pc $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(vbe_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh vbe > $@ || (rm -f $@; exit 1)


vbe_mod-video_i386_pc_vbeutil.o: video/i386/pc/vbeutil.c
	$(TARGET_CC) -Ivideo/i386/pc -I$(srcdir)/video/i386/pc $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(vbe_mod_CFLAGS) -MD -c -o $@ $<
-include vbe_mod-video_i386_pc_vbeutil.d

CLEANFILES += cmd-vbe_mod-video_i386_pc_vbeutil.lst fs-vbe_mod-video_i386_pc_vbeutil.lst
COMMANDFILES += cmd-vbe_mod-video_i386_pc_vbeutil.lst
FSFILES += fs-vbe_mod-video_i386_pc_vbeutil.lst

cmd-vbe_mod-video_i386_pc_vbeutil.lst: video/i386/pc/vbeutil.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ivideo/i386/pc -I$(srcdir)/video/i386/pc $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(vbe_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh vbe > $@ || (rm -f $@; exit 1)

fs-vbe_mod-video_i386_pc_vbeutil.lst: video/i386/pc/vbeutil.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Ivideo/i386/pc -I$(srcdir)/video/i386/pc $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(vbe_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh vbe > $@ || (rm -f $@; exit 1)


vbe_mod_CFLAGS = $(COMMON_CFLAGS)
vbe_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For vbeinfo.mod.
vbeinfo_mod_SOURCES = commands/i386/pc/vbeinfo.c
CLEANFILES += vbeinfo.mod mod-vbeinfo.o mod-vbeinfo.c pre-vbeinfo.o vbeinfo_mod-commands_i386_pc_vbeinfo.o und-vbeinfo.lst
ifneq ($(vbeinfo_mod_EXPORTS),no)
CLEANFILES += def-vbeinfo.lst
DEFSYMFILES += def-vbeinfo.lst
endif
MOSTLYCLEANFILES += vbeinfo_mod-commands_i386_pc_vbeinfo.d
UNDSYMFILES += und-vbeinfo.lst

vbeinfo.mod: pre-vbeinfo.o mod-vbeinfo.o
	-rm -f $@
	$(TARGET_CC) $(vbeinfo_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-vbeinfo.o: $(vbeinfo_mod_DEPENDENCIES) vbeinfo_mod-commands_i386_pc_vbeinfo.o
	-rm -f $@
	$(TARGET_CC) $(vbeinfo_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ vbeinfo_mod-commands_i386_pc_vbeinfo.o

mod-vbeinfo.o: mod-vbeinfo.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(vbeinfo_mod_CFLAGS) -c -o $@ $<

mod-vbeinfo.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'vbeinfo' $< > $@ || (rm -f $@; exit 1)

ifneq ($(vbeinfo_mod_EXPORTS),no)
def-vbeinfo.lst: pre-vbeinfo.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 vbeinfo/' > $@
endif

und-vbeinfo.lst: pre-vbeinfo.o
	echo 'vbeinfo' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

vbeinfo_mod-commands_i386_pc_vbeinfo.o: commands/i386/pc/vbeinfo.c
	$(TARGET_CC) -Icommands/i386/pc -I$(srcdir)/commands/i386/pc $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(vbeinfo_mod_CFLAGS) -MD -c -o $@ $<
-include vbeinfo_mod-commands_i386_pc_vbeinfo.d

CLEANFILES += cmd-vbeinfo_mod-commands_i386_pc_vbeinfo.lst fs-vbeinfo_mod-commands_i386_pc_vbeinfo.lst
COMMANDFILES += cmd-vbeinfo_mod-commands_i386_pc_vbeinfo.lst
FSFILES += fs-vbeinfo_mod-commands_i386_pc_vbeinfo.lst

cmd-vbeinfo_mod-commands_i386_pc_vbeinfo.lst: commands/i386/pc/vbeinfo.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Icommands/i386/pc -I$(srcdir)/commands/i386/pc $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(vbeinfo_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh vbeinfo > $@ || (rm -f $@; exit 1)

fs-vbeinfo_mod-commands_i386_pc_vbeinfo.lst: commands/i386/pc/vbeinfo.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Icommands/i386/pc -I$(srcdir)/commands/i386/pc $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(vbeinfo_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh vbeinfo > $@ || (rm -f $@; exit 1)


vbeinfo_mod_CFLAGS = $(COMMON_CFLAGS)
vbeinfo_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For vbetest.mod.
vbetest_mod_SOURCES = commands/i386/pc/vbetest.c
CLEANFILES += vbetest.mod mod-vbetest.o mod-vbetest.c pre-vbetest.o vbetest_mod-commands_i386_pc_vbetest.o und-vbetest.lst
ifneq ($(vbetest_mod_EXPORTS),no)
CLEANFILES += def-vbetest.lst
DEFSYMFILES += def-vbetest.lst
endif
MOSTLYCLEANFILES += vbetest_mod-commands_i386_pc_vbetest.d
UNDSYMFILES += und-vbetest.lst

vbetest.mod: pre-vbetest.o mod-vbetest.o
	-rm -f $@
	$(TARGET_CC) $(vbetest_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-vbetest.o: $(vbetest_mod_DEPENDENCIES) vbetest_mod-commands_i386_pc_vbetest.o
	-rm -f $@
	$(TARGET_CC) $(vbetest_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ vbetest_mod-commands_i386_pc_vbetest.o

mod-vbetest.o: mod-vbetest.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(vbetest_mod_CFLAGS) -c -o $@ $<

mod-vbetest.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'vbetest' $< > $@ || (rm -f $@; exit 1)

ifneq ($(vbetest_mod_EXPORTS),no)
def-vbetest.lst: pre-vbetest.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 vbetest/' > $@
endif

und-vbetest.lst: pre-vbetest.o
	echo 'vbetest' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

vbetest_mod-commands_i386_pc_vbetest.o: commands/i386/pc/vbetest.c
	$(TARGET_CC) -Icommands/i386/pc -I$(srcdir)/commands/i386/pc $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(vbetest_mod_CFLAGS) -MD -c -o $@ $<
-include vbetest_mod-commands_i386_pc_vbetest.d

CLEANFILES += cmd-vbetest_mod-commands_i386_pc_vbetest.lst fs-vbetest_mod-commands_i386_pc_vbetest.lst
COMMANDFILES += cmd-vbetest_mod-commands_i386_pc_vbetest.lst
FSFILES += fs-vbetest_mod-commands_i386_pc_vbetest.lst

cmd-vbetest_mod-commands_i386_pc_vbetest.lst: commands/i386/pc/vbetest.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Icommands/i386/pc -I$(srcdir)/commands/i386/pc $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(vbetest_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh vbetest > $@ || (rm -f $@; exit 1)

fs-vbetest_mod-commands_i386_pc_vbetest.lst: commands/i386/pc/vbetest.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Icommands/i386/pc -I$(srcdir)/commands/i386/pc $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(vbetest_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh vbetest > $@ || (rm -f $@; exit 1)


vbetest_mod_CFLAGS = $(COMMON_CFLAGS)
vbetest_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For play.mod.
play_mod_SOURCES = commands/i386/pc/play.c
CLEANFILES += play.mod mod-play.o mod-play.c pre-play.o play_mod-commands_i386_pc_play.o und-play.lst
ifneq ($(play_mod_EXPORTS),no)
CLEANFILES += def-play.lst
DEFSYMFILES += def-play.lst
endif
MOSTLYCLEANFILES += play_mod-commands_i386_pc_play.d
UNDSYMFILES += und-play.lst

play.mod: pre-play.o mod-play.o
	-rm -f $@
	$(TARGET_CC) $(play_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-play.o: $(play_mod_DEPENDENCIES) play_mod-commands_i386_pc_play.o
	-rm -f $@
	$(TARGET_CC) $(play_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ play_mod-commands_i386_pc_play.o

mod-play.o: mod-play.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(play_mod_CFLAGS) -c -o $@ $<

mod-play.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'play' $< > $@ || (rm -f $@; exit 1)

ifneq ($(play_mod_EXPORTS),no)
def-play.lst: pre-play.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 play/' > $@
endif

und-play.lst: pre-play.o
	echo 'play' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

play_mod-commands_i386_pc_play.o: commands/i386/pc/play.c
	$(TARGET_CC) -Icommands/i386/pc -I$(srcdir)/commands/i386/pc $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(play_mod_CFLAGS) -MD -c -o $@ $<
-include play_mod-commands_i386_pc_play.d

CLEANFILES += cmd-play_mod-commands_i386_pc_play.lst fs-play_mod-commands_i386_pc_play.lst
COMMANDFILES += cmd-play_mod-commands_i386_pc_play.lst
FSFILES += fs-play_mod-commands_i386_pc_play.lst

cmd-play_mod-commands_i386_pc_play.lst: commands/i386/pc/play.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Icommands/i386/pc -I$(srcdir)/commands/i386/pc $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(play_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh play > $@ || (rm -f $@; exit 1)

fs-play_mod-commands_i386_pc_play.lst: commands/i386/pc/play.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Icommands/i386/pc -I$(srcdir)/commands/i386/pc $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(play_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh play > $@ || (rm -f $@; exit 1)


play_mod_CFLAGS = $(COMMON_CFLAGS)
play_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For video.mod.
video_mod_SOURCES = video/video.c
CLEANFILES += video.mod mod-video.o mod-video.c pre-video.o video_mod-video_video.o und-video.lst
ifneq ($(video_mod_EXPORTS),no)
CLEANFILES += def-video.lst
DEFSYMFILES += def-video.lst
endif
MOSTLYCLEANFILES += video_mod-video_video.d
UNDSYMFILES += und-video.lst

video.mod: pre-video.o mod-video.o
	-rm -f $@
	$(TARGET_CC) $(video_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-video.o: $(video_mod_DEPENDENCIES) video_mod-video_video.o
	-rm -f $@
	$(TARGET_CC) $(video_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ video_mod-video_video.o

mod-video.o: mod-video.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(video_mod_CFLAGS) -c -o $@ $<

mod-video.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'video' $< > $@ || (rm -f $@; exit 1)

ifneq ($(video_mod_EXPORTS),no)
def-video.lst: pre-video.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 video/' > $@
endif

und-video.lst: pre-video.o
	echo 'video' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

video_mod-video_video.o: video/video.c
	$(TARGET_CC) -Ivideo -I$(srcdir)/video $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(video_mod_CFLAGS) -MD -c -o $@ $<
-include video_mod-video_video.d

CLEANFILES += cmd-video_mod-video_video.lst fs-video_mod-video_video.lst
COMMANDFILES += cmd-video_mod-video_video.lst
FSFILES += fs-video_mod-video_video.lst

cmd-video_mod-video_video.lst: video/video.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ivideo -I$(srcdir)/video $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(video_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh video > $@ || (rm -f $@; exit 1)

fs-video_mod-video_video.lst: video/video.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Ivideo -I$(srcdir)/video $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(video_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh video > $@ || (rm -f $@; exit 1)


video_mod_CFLAGS = $(COMMON_CFLAGS)
video_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For gfxterm.mod.
gfxterm_mod_SOURCES = term/gfxterm.c
CLEANFILES += gfxterm.mod mod-gfxterm.o mod-gfxterm.c pre-gfxterm.o gfxterm_mod-term_gfxterm.o und-gfxterm.lst
ifneq ($(gfxterm_mod_EXPORTS),no)
CLEANFILES += def-gfxterm.lst
DEFSYMFILES += def-gfxterm.lst
endif
MOSTLYCLEANFILES += gfxterm_mod-term_gfxterm.d
UNDSYMFILES += und-gfxterm.lst

gfxterm.mod: pre-gfxterm.o mod-gfxterm.o
	-rm -f $@
	$(TARGET_CC) $(gfxterm_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-gfxterm.o: $(gfxterm_mod_DEPENDENCIES) gfxterm_mod-term_gfxterm.o
	-rm -f $@
	$(TARGET_CC) $(gfxterm_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ gfxterm_mod-term_gfxterm.o

mod-gfxterm.o: mod-gfxterm.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(gfxterm_mod_CFLAGS) -c -o $@ $<

mod-gfxterm.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'gfxterm' $< > $@ || (rm -f $@; exit 1)

ifneq ($(gfxterm_mod_EXPORTS),no)
def-gfxterm.lst: pre-gfxterm.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 gfxterm/' > $@
endif

und-gfxterm.lst: pre-gfxterm.o
	echo 'gfxterm' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

gfxterm_mod-term_gfxterm.o: term/gfxterm.c
	$(TARGET_CC) -Iterm -I$(srcdir)/term $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(gfxterm_mod_CFLAGS) -MD -c -o $@ $<
-include gfxterm_mod-term_gfxterm.d

CLEANFILES += cmd-gfxterm_mod-term_gfxterm.lst fs-gfxterm_mod-term_gfxterm.lst
COMMANDFILES += cmd-gfxterm_mod-term_gfxterm.lst
FSFILES += fs-gfxterm_mod-term_gfxterm.lst

cmd-gfxterm_mod-term_gfxterm.lst: term/gfxterm.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Iterm -I$(srcdir)/term $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(gfxterm_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh gfxterm > $@ || (rm -f $@; exit 1)

fs-gfxterm_mod-term_gfxterm.lst: term/gfxterm.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Iterm -I$(srcdir)/term $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(gfxterm_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh gfxterm > $@ || (rm -f $@; exit 1)


gfxterm_mod_CFLAGS = $(COMMON_CFLAGS)
gfxterm_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For videotest.mod.
videotest_mod_SOURCES = commands/videotest.c
CLEANFILES += videotest.mod mod-videotest.o mod-videotest.c pre-videotest.o videotest_mod-commands_videotest.o und-videotest.lst
ifneq ($(videotest_mod_EXPORTS),no)
CLEANFILES += def-videotest.lst
DEFSYMFILES += def-videotest.lst
endif
MOSTLYCLEANFILES += videotest_mod-commands_videotest.d
UNDSYMFILES += und-videotest.lst

videotest.mod: pre-videotest.o mod-videotest.o
	-rm -f $@
	$(TARGET_CC) $(videotest_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-videotest.o: $(videotest_mod_DEPENDENCIES) videotest_mod-commands_videotest.o
	-rm -f $@
	$(TARGET_CC) $(videotest_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ videotest_mod-commands_videotest.o

mod-videotest.o: mod-videotest.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(videotest_mod_CFLAGS) -c -o $@ $<

mod-videotest.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'videotest' $< > $@ || (rm -f $@; exit 1)

ifneq ($(videotest_mod_EXPORTS),no)
def-videotest.lst: pre-videotest.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 videotest/' > $@
endif

und-videotest.lst: pre-videotest.o
	echo 'videotest' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

videotest_mod-commands_videotest.o: commands/videotest.c
	$(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(videotest_mod_CFLAGS) -MD -c -o $@ $<
-include videotest_mod-commands_videotest.d

CLEANFILES += cmd-videotest_mod-commands_videotest.lst fs-videotest_mod-commands_videotest.lst
COMMANDFILES += cmd-videotest_mod-commands_videotest.lst
FSFILES += fs-videotest_mod-commands_videotest.lst

cmd-videotest_mod-commands_videotest.lst: commands/videotest.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(videotest_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh videotest > $@ || (rm -f $@; exit 1)

fs-videotest_mod-commands_videotest.lst: commands/videotest.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(videotest_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh videotest > $@ || (rm -f $@; exit 1)


videotest_mod_CFLAGS = $(COMMON_CFLAGS)
videotest_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For bitmap.mod
bitmap_mod_SOURCES = video/bitmap.c
CLEANFILES += bitmap.mod mod-bitmap.o mod-bitmap.c pre-bitmap.o bitmap_mod-video_bitmap.o und-bitmap.lst
ifneq ($(bitmap_mod_EXPORTS),no)
CLEANFILES += def-bitmap.lst
DEFSYMFILES += def-bitmap.lst
endif
MOSTLYCLEANFILES += bitmap_mod-video_bitmap.d
UNDSYMFILES += und-bitmap.lst

bitmap.mod: pre-bitmap.o mod-bitmap.o
	-rm -f $@
	$(TARGET_CC) $(bitmap_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-bitmap.o: $(bitmap_mod_DEPENDENCIES) bitmap_mod-video_bitmap.o
	-rm -f $@
	$(TARGET_CC) $(bitmap_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ bitmap_mod-video_bitmap.o

mod-bitmap.o: mod-bitmap.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(bitmap_mod_CFLAGS) -c -o $@ $<

mod-bitmap.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'bitmap' $< > $@ || (rm -f $@; exit 1)

ifneq ($(bitmap_mod_EXPORTS),no)
def-bitmap.lst: pre-bitmap.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 bitmap/' > $@
endif

und-bitmap.lst: pre-bitmap.o
	echo 'bitmap' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

bitmap_mod-video_bitmap.o: video/bitmap.c
	$(TARGET_CC) -Ivideo -I$(srcdir)/video $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(bitmap_mod_CFLAGS) -MD -c -o $@ $<
-include bitmap_mod-video_bitmap.d

CLEANFILES += cmd-bitmap_mod-video_bitmap.lst fs-bitmap_mod-video_bitmap.lst
COMMANDFILES += cmd-bitmap_mod-video_bitmap.lst
FSFILES += fs-bitmap_mod-video_bitmap.lst

cmd-bitmap_mod-video_bitmap.lst: video/bitmap.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ivideo -I$(srcdir)/video $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(bitmap_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh bitmap > $@ || (rm -f $@; exit 1)

fs-bitmap_mod-video_bitmap.lst: video/bitmap.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Ivideo -I$(srcdir)/video $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(bitmap_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh bitmap > $@ || (rm -f $@; exit 1)


bitmap_mod_CFLAGS = $(COMMON_CFLAGS)
bitmap_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For tga.mod
tga_mod_SOURCES = video/readers/tga.c
CLEANFILES += tga.mod mod-tga.o mod-tga.c pre-tga.o tga_mod-video_readers_tga.o und-tga.lst
ifneq ($(tga_mod_EXPORTS),no)
CLEANFILES += def-tga.lst
DEFSYMFILES += def-tga.lst
endif
MOSTLYCLEANFILES += tga_mod-video_readers_tga.d
UNDSYMFILES += und-tga.lst

tga.mod: pre-tga.o mod-tga.o
	-rm -f $@
	$(TARGET_CC) $(tga_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-tga.o: $(tga_mod_DEPENDENCIES) tga_mod-video_readers_tga.o
	-rm -f $@
	$(TARGET_CC) $(tga_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ tga_mod-video_readers_tga.o

mod-tga.o: mod-tga.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(tga_mod_CFLAGS) -c -o $@ $<

mod-tga.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'tga' $< > $@ || (rm -f $@; exit 1)

ifneq ($(tga_mod_EXPORTS),no)
def-tga.lst: pre-tga.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 tga/' > $@
endif

und-tga.lst: pre-tga.o
	echo 'tga' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

tga_mod-video_readers_tga.o: video/readers/tga.c
	$(TARGET_CC) -Ivideo/readers -I$(srcdir)/video/readers $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(tga_mod_CFLAGS) -MD -c -o $@ $<
-include tga_mod-video_readers_tga.d

CLEANFILES += cmd-tga_mod-video_readers_tga.lst fs-tga_mod-video_readers_tga.lst
COMMANDFILES += cmd-tga_mod-video_readers_tga.lst
FSFILES += fs-tga_mod-video_readers_tga.lst

cmd-tga_mod-video_readers_tga.lst: video/readers/tga.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ivideo/readers -I$(srcdir)/video/readers $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(tga_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh tga > $@ || (rm -f $@; exit 1)

fs-tga_mod-video_readers_tga.lst: video/readers/tga.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Ivideo/readers -I$(srcdir)/video/readers $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(tga_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh tga > $@ || (rm -f $@; exit 1)


tga_mod_CFLAGS = $(COMMON_CFLAGS)
tga_mod_LDFLAGS = $(COMMON_LDFLAGS)

include $(srcdir)/conf/common.mk
