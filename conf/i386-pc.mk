# -*- makefile -*-

COMMON_ASFLAGS = -nostdinc -fno-builtin
COMMON_CFLAGS = -fno-builtin -mrtd -mregparm=3

# Images.
pkgdata_IMAGES = boot.img diskboot.img kernel.img

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
	kern/i386/dl.c kern/i386/pc/init.c disk/i386/pc/partition.c \
	disk/i386/pc/biosdisk.c \
	term/i386/pc/console.c \
	symlist.c
CLEANFILES += kernel.img kernel.exec kernel_img-kern_i386_pc_startup.o kernel_img-kern_main.o kernel_img-kern_device.o kernel_img-kern_disk.o kernel_img-kern_dl.o kernel_img-kern_file.o kernel_img-kern_fs.o kernel_img-kern_err.o kernel_img-kern_misc.o kernel_img-kern_mm.o kernel_img-kern_loader.o kernel_img-kern_rescue.o kernel_img-kern_term.o kernel_img-kern_i386_dl.o kernel_img-kern_i386_pc_init.o kernel_img-disk_i386_pc_partition.o kernel_img-disk_i386_pc_biosdisk.o kernel_img-term_i386_pc_console.o kernel_img-symlist.o
MOSTLYCLEANFILES += kernel_img-kern_i386_pc_startup.d kernel_img-kern_main.d kernel_img-kern_device.d kernel_img-kern_disk.d kernel_img-kern_dl.d kernel_img-kern_file.d kernel_img-kern_fs.d kernel_img-kern_err.d kernel_img-kern_misc.d kernel_img-kern_mm.d kernel_img-kern_loader.d kernel_img-kern_rescue.d kernel_img-kern_term.d kernel_img-kern_i386_dl.d kernel_img-kern_i386_pc_init.d kernel_img-disk_i386_pc_partition.d kernel_img-disk_i386_pc_biosdisk.d kernel_img-term_i386_pc_console.d kernel_img-symlist.d

kernel.img: kernel.exec
	$(OBJCOPY) -O binary -R .note -R .comment $< $@

kernel.exec: kernel_img-kern_i386_pc_startup.o kernel_img-kern_main.o kernel_img-kern_device.o kernel_img-kern_disk.o kernel_img-kern_dl.o kernel_img-kern_file.o kernel_img-kern_fs.o kernel_img-kern_err.o kernel_img-kern_misc.o kernel_img-kern_mm.o kernel_img-kern_loader.o kernel_img-kern_rescue.o kernel_img-kern_term.o kernel_img-kern_i386_dl.o kernel_img-kern_i386_pc_init.o kernel_img-disk_i386_pc_partition.o kernel_img-disk_i386_pc_biosdisk.o kernel_img-term_i386_pc_console.o kernel_img-symlist.o
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

kernel_img-disk_i386_pc_partition.o: disk/i386/pc/partition.c
	$(CC) -Idisk/i386/pc -I$(srcdir)/disk/i386/pc $(CPPFLAGS)  $(CFLAGS) $(kernel_img_CFLAGS) -c -o $@ $<

kernel_img-disk_i386_pc_partition.d: disk/i386/pc/partition.c
	set -e; 	  $(CC) -Idisk/i386/pc -I$(srcdir)/disk/i386/pc $(CPPFLAGS)  $(CFLAGS) $(kernel_img_CFLAGS) -M $< 	  | sed 's,partition\.o[ :]*,kernel_img-disk_i386_pc_partition.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include kernel_img-disk_i386_pc_partition.d

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

kernel_img_HEADERS = boot.h device.h disk.h dl.h elf.h err.h \
	file.h fs.h kernel.h loader.h misc.h mm.h net.h rescue.h symbol.h \
	term.h types.h machine/biosdisk.h machine/boot.h \
	machine/console.h machine/init.h machine/memory.h \
	machine/loader.h machine/partition.h machine/vga.h
kernel_img_CFLAGS = $(COMMON_CFLAGS)
kernel_img_ASFLAGS = $(COMMON_ASFLAGS)
kernel_img_LDFLAGS = -nostdlib -Wl,-N,-Ttext,8200

MOSTLYCLEANFILES += symlist.c kernel_syms.lst
DEFSYMFILES += kernel_syms.lst

symlist.c: $(addprefix include/pupa/,$(kernel_img_HEADERS)) gensymlist.sh
	sh $(srcdir)/gensymlist.sh $(filter %.h,$^) > $@

kernel_syms.lst: $(addprefix include/pupa/,$(kernel_img_HEADERS)) genkernsyms.sh
	sh $(srcdir)/genkernsyms.sh $(filter %h,$^) > $@

# Utilities.
bin_UTILITIES = pupa-mkimage
sbin_UTILITIES = pupa-setup pupa-emu
noinst_UTILITIES = genmoddep

# For pupa-mkimage.
pupa_mkimage_SOURCES = util/i386/pc/pupa-mkimage.c util/misc.c \
	util/resolve.c
CLEANFILES += pupa-mkimage pupa_mkimage-util_i386_pc_pupa_mkimage.o pupa_mkimage-util_misc.o pupa_mkimage-util_resolve.o
MOSTLYCLEANFILES += pupa_mkimage-util_i386_pc_pupa_mkimage.d pupa_mkimage-util_misc.d pupa_mkimage-util_resolve.d

pupa-mkimage: pupa_mkimage-util_i386_pc_pupa_mkimage.o pupa_mkimage-util_misc.o pupa_mkimage-util_resolve.o
	$(BUILD_CC) -o $@ $^ $(BUILD_LDFLAGS) $(pupa_mkimage_LDFLAGS)

pupa_mkimage-util_i386_pc_pupa_mkimage.o: util/i386/pc/pupa-mkimage.c
	$(BUILD_CC) -Iutil/i386/pc -I$(srcdir)/util/i386/pc $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_mkimage_CFLAGS) -c -o $@ $<

pupa_mkimage-util_i386_pc_pupa_mkimage.d: util/i386/pc/pupa-mkimage.c
	set -e; 	  $(BUILD_CC) -Iutil/i386/pc -I$(srcdir)/util/i386/pc $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_mkimage_CFLAGS) -M $< 	  | sed 's,pupa\-mkimage\.o[ :]*,pupa_mkimage-util_i386_pc_pupa_mkimage.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupa_mkimage-util_i386_pc_pupa_mkimage.d

pupa_mkimage-util_misc.o: util/misc.c
	$(BUILD_CC) -Iutil -I$(srcdir)/util $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_mkimage_CFLAGS) -c -o $@ $<

pupa_mkimage-util_misc.d: util/misc.c
	set -e; 	  $(BUILD_CC) -Iutil -I$(srcdir)/util $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_mkimage_CFLAGS) -M $< 	  | sed 's,misc\.o[ :]*,pupa_mkimage-util_misc.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupa_mkimage-util_misc.d

pupa_mkimage-util_resolve.o: util/resolve.c
	$(BUILD_CC) -Iutil -I$(srcdir)/util $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_mkimage_CFLAGS) -c -o $@ $<

pupa_mkimage-util_resolve.d: util/resolve.c
	set -e; 	  $(BUILD_CC) -Iutil -I$(srcdir)/util $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_mkimage_CFLAGS) -M $< 	  | sed 's,resolve\.o[ :]*,pupa_mkimage-util_resolve.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupa_mkimage-util_resolve.d

pupa_mkimage_LDFLAGS = -llzo

# For pupa-setup.
pupa_setup_SOURCES = util/i386/pc/pupa-setup.c util/i386/pc/biosdisk.c \
	util/misc.c util/i386/pc/getroot.c kern/device.c kern/disk.c \
	kern/err.c kern/misc.c disk/i386/pc/partition.c fs/fat.c fs/ext2.c \
	kern/file.c kern/fs.c
CLEANFILES += pupa-setup pupa_setup-util_i386_pc_pupa_setup.o pupa_setup-util_i386_pc_biosdisk.o pupa_setup-util_misc.o pupa_setup-util_i386_pc_getroot.o pupa_setup-kern_device.o pupa_setup-kern_disk.o pupa_setup-kern_err.o pupa_setup-kern_misc.o pupa_setup-disk_i386_pc_partition.o pupa_setup-fs_fat.o pupa_setup-fs_ext2.o pupa_setup-kern_file.o pupa_setup-kern_fs.o
MOSTLYCLEANFILES += pupa_setup-util_i386_pc_pupa_setup.d pupa_setup-util_i386_pc_biosdisk.d pupa_setup-util_misc.d pupa_setup-util_i386_pc_getroot.d pupa_setup-kern_device.d pupa_setup-kern_disk.d pupa_setup-kern_err.d pupa_setup-kern_misc.d pupa_setup-disk_i386_pc_partition.d pupa_setup-fs_fat.d pupa_setup-fs_ext2.d pupa_setup-kern_file.d pupa_setup-kern_fs.d

pupa-setup: pupa_setup-util_i386_pc_pupa_setup.o pupa_setup-util_i386_pc_biosdisk.o pupa_setup-util_misc.o pupa_setup-util_i386_pc_getroot.o pupa_setup-kern_device.o pupa_setup-kern_disk.o pupa_setup-kern_err.o pupa_setup-kern_misc.o pupa_setup-disk_i386_pc_partition.o pupa_setup-fs_fat.o pupa_setup-fs_ext2.o pupa_setup-kern_file.o pupa_setup-kern_fs.o
	$(BUILD_CC) -o $@ $^ $(BUILD_LDFLAGS) $(pupa_setup_LDFLAGS)

pupa_setup-util_i386_pc_pupa_setup.o: util/i386/pc/pupa-setup.c
	$(BUILD_CC) -Iutil/i386/pc -I$(srcdir)/util/i386/pc $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_setup_CFLAGS) -c -o $@ $<

pupa_setup-util_i386_pc_pupa_setup.d: util/i386/pc/pupa-setup.c
	set -e; 	  $(BUILD_CC) -Iutil/i386/pc -I$(srcdir)/util/i386/pc $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_setup_CFLAGS) -M $< 	  | sed 's,pupa\-setup\.o[ :]*,pupa_setup-util_i386_pc_pupa_setup.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupa_setup-util_i386_pc_pupa_setup.d

pupa_setup-util_i386_pc_biosdisk.o: util/i386/pc/biosdisk.c
	$(BUILD_CC) -Iutil/i386/pc -I$(srcdir)/util/i386/pc $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_setup_CFLAGS) -c -o $@ $<

pupa_setup-util_i386_pc_biosdisk.d: util/i386/pc/biosdisk.c
	set -e; 	  $(BUILD_CC) -Iutil/i386/pc -I$(srcdir)/util/i386/pc $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_setup_CFLAGS) -M $< 	  | sed 's,biosdisk\.o[ :]*,pupa_setup-util_i386_pc_biosdisk.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupa_setup-util_i386_pc_biosdisk.d

pupa_setup-util_misc.o: util/misc.c
	$(BUILD_CC) -Iutil -I$(srcdir)/util $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_setup_CFLAGS) -c -o $@ $<

pupa_setup-util_misc.d: util/misc.c
	set -e; 	  $(BUILD_CC) -Iutil -I$(srcdir)/util $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_setup_CFLAGS) -M $< 	  | sed 's,misc\.o[ :]*,pupa_setup-util_misc.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupa_setup-util_misc.d

pupa_setup-util_i386_pc_getroot.o: util/i386/pc/getroot.c
	$(BUILD_CC) -Iutil/i386/pc -I$(srcdir)/util/i386/pc $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_setup_CFLAGS) -c -o $@ $<

pupa_setup-util_i386_pc_getroot.d: util/i386/pc/getroot.c
	set -e; 	  $(BUILD_CC) -Iutil/i386/pc -I$(srcdir)/util/i386/pc $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_setup_CFLAGS) -M $< 	  | sed 's,getroot\.o[ :]*,pupa_setup-util_i386_pc_getroot.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupa_setup-util_i386_pc_getroot.d

pupa_setup-kern_device.o: kern/device.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_setup_CFLAGS) -c -o $@ $<

pupa_setup-kern_device.d: kern/device.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_setup_CFLAGS) -M $< 	  | sed 's,device\.o[ :]*,pupa_setup-kern_device.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupa_setup-kern_device.d

pupa_setup-kern_disk.o: kern/disk.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_setup_CFLAGS) -c -o $@ $<

pupa_setup-kern_disk.d: kern/disk.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_setup_CFLAGS) -M $< 	  | sed 's,disk\.o[ :]*,pupa_setup-kern_disk.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupa_setup-kern_disk.d

pupa_setup-kern_err.o: kern/err.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_setup_CFLAGS) -c -o $@ $<

pupa_setup-kern_err.d: kern/err.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_setup_CFLAGS) -M $< 	  | sed 's,err\.o[ :]*,pupa_setup-kern_err.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupa_setup-kern_err.d

pupa_setup-kern_misc.o: kern/misc.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_setup_CFLAGS) -c -o $@ $<

pupa_setup-kern_misc.d: kern/misc.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_setup_CFLAGS) -M $< 	  | sed 's,misc\.o[ :]*,pupa_setup-kern_misc.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupa_setup-kern_misc.d

pupa_setup-disk_i386_pc_partition.o: disk/i386/pc/partition.c
	$(BUILD_CC) -Idisk/i386/pc -I$(srcdir)/disk/i386/pc $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_setup_CFLAGS) -c -o $@ $<

pupa_setup-disk_i386_pc_partition.d: disk/i386/pc/partition.c
	set -e; 	  $(BUILD_CC) -Idisk/i386/pc -I$(srcdir)/disk/i386/pc $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_setup_CFLAGS) -M $< 	  | sed 's,partition\.o[ :]*,pupa_setup-disk_i386_pc_partition.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupa_setup-disk_i386_pc_partition.d

pupa_setup-fs_fat.o: fs/fat.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_setup_CFLAGS) -c -o $@ $<

pupa_setup-fs_fat.d: fs/fat.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_setup_CFLAGS) -M $< 	  | sed 's,fat\.o[ :]*,pupa_setup-fs_fat.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupa_setup-fs_fat.d

pupa_setup-fs_ext2.o: fs/ext2.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_setup_CFLAGS) -c -o $@ $<

pupa_setup-fs_ext2.d: fs/ext2.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_setup_CFLAGS) -M $< 	  | sed 's,ext2\.o[ :]*,pupa_setup-fs_ext2.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupa_setup-fs_ext2.d

pupa_setup-kern_file.o: kern/file.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_setup_CFLAGS) -c -o $@ $<

pupa_setup-kern_file.d: kern/file.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_setup_CFLAGS) -M $< 	  | sed 's,file\.o[ :]*,pupa_setup-kern_file.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupa_setup-kern_file.d

pupa_setup-kern_fs.o: kern/fs.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_setup_CFLAGS) -c -o $@ $<

pupa_setup-kern_fs.d: kern/fs.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_setup_CFLAGS) -M $< 	  | sed 's,fs\.o[ :]*,pupa_setup-kern_fs.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupa_setup-kern_fs.d


# For pupa
pupa_emu_SOURCES = kern/main.c kern/device.c				\
	kern/disk.c kern/dl.c kern/file.c kern/fs.c kern/err.c		\
        kern/misc.c kern/loader.c kern/rescue.c kern/term.c		\
	disk/i386/pc/partition.c 					\
	util/i386/pc/biosdisk.c fs/fat.c fs/ext2.c			\
	normal/cmdline.c normal/command.c normal/main.c normal/menu.c	\
	util/console.c util/pupa-emu.c util/misc.c util/i386/pc/getroot.c
CLEANFILES += pupa-emu pupa_emu-kern_main.o pupa_emu-kern_device.o pupa_emu-kern_disk.o pupa_emu-kern_dl.o pupa_emu-kern_file.o pupa_emu-kern_fs.o pupa_emu-kern_err.o pupa_emu-kern_misc.o pupa_emu-kern_loader.o pupa_emu-kern_rescue.o pupa_emu-kern_term.o pupa_emu-disk_i386_pc_partition.o pupa_emu-util_i386_pc_biosdisk.o pupa_emu-fs_fat.o pupa_emu-fs_ext2.o pupa_emu-normal_cmdline.o pupa_emu-normal_command.o pupa_emu-normal_main.o pupa_emu-normal_menu.o pupa_emu-util_console.o pupa_emu-util_pupa_emu.o pupa_emu-util_misc.o pupa_emu-util_i386_pc_getroot.o
MOSTLYCLEANFILES += pupa_emu-kern_main.d pupa_emu-kern_device.d pupa_emu-kern_disk.d pupa_emu-kern_dl.d pupa_emu-kern_file.d pupa_emu-kern_fs.d pupa_emu-kern_err.d pupa_emu-kern_misc.d pupa_emu-kern_loader.d pupa_emu-kern_rescue.d pupa_emu-kern_term.d pupa_emu-disk_i386_pc_partition.d pupa_emu-util_i386_pc_biosdisk.d pupa_emu-fs_fat.d pupa_emu-fs_ext2.d pupa_emu-normal_cmdline.d pupa_emu-normal_command.d pupa_emu-normal_main.d pupa_emu-normal_menu.d pupa_emu-util_console.d pupa_emu-util_pupa_emu.d pupa_emu-util_misc.d pupa_emu-util_i386_pc_getroot.d

pupa-emu: pupa_emu-kern_main.o pupa_emu-kern_device.o pupa_emu-kern_disk.o pupa_emu-kern_dl.o pupa_emu-kern_file.o pupa_emu-kern_fs.o pupa_emu-kern_err.o pupa_emu-kern_misc.o pupa_emu-kern_loader.o pupa_emu-kern_rescue.o pupa_emu-kern_term.o pupa_emu-disk_i386_pc_partition.o pupa_emu-util_i386_pc_biosdisk.o pupa_emu-fs_fat.o pupa_emu-fs_ext2.o pupa_emu-normal_cmdline.o pupa_emu-normal_command.o pupa_emu-normal_main.o pupa_emu-normal_menu.o pupa_emu-util_console.o pupa_emu-util_pupa_emu.o pupa_emu-util_misc.o pupa_emu-util_i386_pc_getroot.o
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

pupa_emu-disk_i386_pc_partition.o: disk/i386/pc/partition.c
	$(BUILD_CC) -Idisk/i386/pc -I$(srcdir)/disk/i386/pc $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_emu_CFLAGS) -c -o $@ $<

pupa_emu-disk_i386_pc_partition.d: disk/i386/pc/partition.c
	set -e; 	  $(BUILD_CC) -Idisk/i386/pc -I$(srcdir)/disk/i386/pc $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(pupa_emu_CFLAGS) -M $< 	  | sed 's,partition\.o[ :]*,pupa_emu-disk_i386_pc_partition.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pupa_emu-disk_i386_pc_partition.d

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

pupa_emu_LDFLAGS = -lncurses

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
pkgdata_MODULES = _chain.mod _linux.mod fat.mod ext2.mod normal.mod hello.mod \
	vga.mod font.mod _multiboot.mod

# For _chain.mod.
_chain_mod_SOURCES = loader/i386/pc/chainloader.c
CLEANFILES += _chain.mod mod-_chain.o mod-_chain.c pre-_chain.o _chain_mod-loader_i386_pc_chainloader.o def-_chain.lst und-_chain.lst
MOSTLYCLEANFILES += _chain_mod-loader_i386_pc_chainloader.d
DEFSYMFILES += def-_chain.lst
UNDSYMFILES += und-_chain.lst

_chain.mod: pre-_chain.o mod-_chain.o
	-rm -f $@
	$(LD) -r -o $@ $^
	$(STRIP) --strip-unneeded -K pupa_mod_init -K pupa_mod_fini -R .note -R .comment $@

pre-_chain.o: _chain_mod-loader_i386_pc_chainloader.o
	-rm -f $@
	$(LD) -r -o $@ $^

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

# For fat.mod.
fat_mod_SOURCES = fs/fat.c
CLEANFILES += fat.mod mod-fat.o mod-fat.c pre-fat.o fat_mod-fs_fat.o def-fat.lst und-fat.lst
MOSTLYCLEANFILES += fat_mod-fs_fat.d
DEFSYMFILES += def-fat.lst
UNDSYMFILES += und-fat.lst

fat.mod: pre-fat.o mod-fat.o
	-rm -f $@
	$(LD) -r -o $@ $^
	$(STRIP) --strip-unneeded -K pupa_mod_init -K pupa_mod_fini -R .note -R .comment $@

pre-fat.o: fat_mod-fs_fat.o
	-rm -f $@
	$(LD) -r -o $@ $^

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
	$(LD) -r -o $@ $^
	$(STRIP) --strip-unneeded -K pupa_mod_init -K pupa_mod_fini -R .note -R .comment $@

pre-ext2.o: ext2_mod-fs_ext2.o
	-rm -f $@
	$(LD) -r -o $@ $^

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

# For _linux.mod.
_linux_mod_SOURCES = loader/i386/pc/linux.c
CLEANFILES += _linux.mod mod-_linux.o mod-_linux.c pre-_linux.o _linux_mod-loader_i386_pc_linux.o def-_linux.lst und-_linux.lst
MOSTLYCLEANFILES += _linux_mod-loader_i386_pc_linux.d
DEFSYMFILES += def-_linux.lst
UNDSYMFILES += und-_linux.lst

_linux.mod: pre-_linux.o mod-_linux.o
	-rm -f $@
	$(LD) -r -o $@ $^
	$(STRIP) --strip-unneeded -K pupa_mod_init -K pupa_mod_fini -R .note -R .comment $@

pre-_linux.o: _linux_mod-loader_i386_pc_linux.o
	-rm -f $@
	$(LD) -r -o $@ $^

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

# For normal.mod.
normal_mod_SOURCES = normal/cmdline.c normal/command.c normal/main.c \
	normal/menu.c normal/i386/setjmp.S
CLEANFILES += normal.mod mod-normal.o mod-normal.c pre-normal.o normal_mod-normal_cmdline.o normal_mod-normal_command.o normal_mod-normal_main.o normal_mod-normal_menu.o normal_mod-normal_i386_setjmp.o def-normal.lst und-normal.lst
MOSTLYCLEANFILES += normal_mod-normal_cmdline.d normal_mod-normal_command.d normal_mod-normal_main.d normal_mod-normal_menu.d normal_mod-normal_i386_setjmp.d
DEFSYMFILES += def-normal.lst
UNDSYMFILES += und-normal.lst

normal.mod: pre-normal.o mod-normal.o
	-rm -f $@
	$(LD) -r -o $@ $^
	$(STRIP) --strip-unneeded -K pupa_mod_init -K pupa_mod_fini -R .note -R .comment $@

pre-normal.o: normal_mod-normal_cmdline.o normal_mod-normal_command.o normal_mod-normal_main.o normal_mod-normal_menu.o normal_mod-normal_i386_setjmp.o
	-rm -f $@
	$(LD) -r -o $@ $^

mod-normal.o: mod-normal.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -c -o $@ $<

mod-normal.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'normal' $< > $@ || (rm -f $@; exit 1)

def-normal.lst: pre-normal.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 normal/' > $@

und-normal.lst: pre-normal.o
	echo 'normal' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

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
	$(LD) -r -o $@ $^
	$(STRIP) --strip-unneeded -K pupa_mod_init -K pupa_mod_fini -R .note -R .comment $@

pre-hello.o: hello_mod-hello_hello.o
	-rm -f $@
	$(LD) -r -o $@ $^

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

# For vga.mod.
vga_mod_SOURCES = term/i386/pc/vga.c
CLEANFILES += vga.mod mod-vga.o mod-vga.c pre-vga.o vga_mod-term_i386_pc_vga.o def-vga.lst und-vga.lst
MOSTLYCLEANFILES += vga_mod-term_i386_pc_vga.d
DEFSYMFILES += def-vga.lst
UNDSYMFILES += und-vga.lst

vga.mod: pre-vga.o mod-vga.o
	-rm -f $@
	$(LD) -r -o $@ $^
	$(STRIP) --strip-unneeded -K pupa_mod_init -K pupa_mod_fini -R .note -R .comment $@

pre-vga.o: vga_mod-term_i386_pc_vga.o
	-rm -f $@
	$(LD) -r -o $@ $^

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
	$(LD) -r -o $@ $^
	$(STRIP) --strip-unneeded -K pupa_mod_init -K pupa_mod_fini -R .note -R .comment $@

pre-font.o: font_mod-font_manager.o
	-rm -f $@
	$(LD) -r -o $@ $^

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
	$(LD) -r -o $@ $^
	$(STRIP) --strip-unneeded -K pupa_mod_init -K pupa_mod_fini -R .note -R .comment $@

pre-_multiboot.o: _multiboot_mod-loader_i386_pc_multiboot.o
	-rm -f $@
	$(LD) -r -o $@ $^

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
CLEANFILES += moddep.lst
pkgdata_DATA += moddep.lst
moddep.lst: $(DEFSYMFILES) $(UNDSYMFILES) genmoddep
	cat $(DEFSYMFILES) /dev/null | ./genmoddep $(UNDSYMFILES) > $@ \
	  || (rm -f $@; exit 1)
