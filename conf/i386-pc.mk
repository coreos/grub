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
	$(CC) $(LDFLAGS) $(boot_img_LDFLAGS) -o $@ $^

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
	$(CC) $(LDFLAGS) $(diskboot_img_LDFLAGS) -o $@ $^

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
	$(CC) $(LDFLAGS) $(kernel_img_LDFLAGS) -o $@ $^

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
	machine/loader.h machine/partition.h
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
sbin_UTILITIES = pupa-setup
noinst_UTILITIES = genmoddep

# For pupa-mkimage.
pupa_mkimage_SOURCES = util/i386/pc/pupa-mkimage.c util/misc.c \
	util/resolve.c
CLEANFILES += pupa-mkimage pupa_mkimage-util_i386_pc_pupa_mkimage.o pupa_mkimage-util_misc.o pupa_mkimage-util_resolve.o
MOSTLYCLEANFILES += pupa_mkimage-util_i386_pc_pupa_mkimage.d pupa_mkimage-util_misc.d pupa_mkimage-util_resolve.d

pupa-mkimage: pupa_mkimage-util_i386_pc_pupa_mkimage.o pupa_mkimage-util_misc.o pupa_mkimage-util_resolve.o
	$(BUILD_CC) $(BUILD_LDFLAGS) $(pupa_mkimage_LDFLAGS) -o $@ $^

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


# For pupa-setup.
pupa_setup_SOURCES = util/i386/pc/pupa-setup.c util/i386/pc/biosdisk.c \
	util/misc.c kern/device.c kern/disk.c kern/file.c kern/fs.c \
	kern/err.c kern/misc.c disk/i386/pc/partition.c fs/fat.c
CLEANFILES += pupa-setup pupa_setup-util_i386_pc_pupa_setup.o pupa_setup-util_i386_pc_biosdisk.o pupa_setup-util_misc.o pupa_setup-kern_device.o pupa_setup-kern_disk.o pupa_setup-kern_file.o pupa_setup-kern_fs.o pupa_setup-kern_err.o pupa_setup-kern_misc.o pupa_setup-disk_i386_pc_partition.o pupa_setup-fs_fat.o
MOSTLYCLEANFILES += pupa_setup-util_i386_pc_pupa_setup.d pupa_setup-util_i386_pc_biosdisk.d pupa_setup-util_misc.d pupa_setup-kern_device.d pupa_setup-kern_disk.d pupa_setup-kern_file.d pupa_setup-kern_fs.d pupa_setup-kern_err.d pupa_setup-kern_misc.d pupa_setup-disk_i386_pc_partition.d pupa_setup-fs_fat.d

pupa-setup: pupa_setup-util_i386_pc_pupa_setup.o pupa_setup-util_i386_pc_biosdisk.o pupa_setup-util_misc.o pupa_setup-kern_device.o pupa_setup-kern_disk.o pupa_setup-kern_file.o pupa_setup-kern_fs.o pupa_setup-kern_err.o pupa_setup-kern_misc.o pupa_setup-disk_i386_pc_partition.o pupa_setup-fs_fat.o
	$(BUILD_CC) $(BUILD_LDFLAGS) $(pupa_setup_LDFLAGS) -o $@ $^

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


# For genmoddep.
genmoddep_SOURCES = util/genmoddep.c
CLEANFILES += genmoddep genmoddep-util_genmoddep.o
MOSTLYCLEANFILES += genmoddep-util_genmoddep.d

genmoddep: genmoddep-util_genmoddep.o
	$(BUILD_CC) $(BUILD_LDFLAGS) $(genmoddep_LDFLAGS) -o $@ $^

genmoddep-util_genmoddep.o: util/genmoddep.c
	$(BUILD_CC) -Iutil -I$(srcdir)/util $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(genmoddep_CFLAGS) -c -o $@ $<

genmoddep-util_genmoddep.d: util/genmoddep.c
	set -e; 	  $(BUILD_CC) -Iutil -I$(srcdir)/util $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DPUPA_UTIL=1 $(genmoddep_CFLAGS) -M $< 	  | sed 's,genmoddep\.o[ :]*,genmoddep-util_genmoddep.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include genmoddep-util_genmoddep.d


# Modules.
pkgdata_MODULES = chain.mod fat.mod

# For chain.mod.
chain_mod_SOURCES = loader/i386/pc/chainloader.c
CLEANFILES += chain.mod mod-chain.o mod-chain.c pre-chain.o chain_mod-loader_i386_pc_chainloader.o def-chain.lst und-chain.lst
MOSTLYCLEANFILES += chain_mod-loader_i386_pc_chainloader.d
DEFSYMFILES += def-chain.lst
UNDSYMFILES += und-chain.lst

chain.mod: pre-chain.o mod-chain.o
	-rm -f $@
	$(LD) -r -o $@ $^
	$(STRIP) --strip-unneeded -K pupa_mod_init -K pupa_mod_fini -R .note -R .comment $@

pre-chain.o: chain_mod-loader_i386_pc_chainloader.o
	-rm -f $@
	$(LD) -r -o $@ $^

mod-chain.o: mod-chain.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(chain_mod_CFLAGS) -c -o $@ $<

mod-chain.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'chain' $< > $@ || (rm -f $@; exit 1)

def-chain.lst: pre-chain.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 chain/' > $@

und-chain.lst: pre-chain.o
	echo 'chain' > $@
	$(NM) -u -P -p $< >> $@

chain_mod-loader_i386_pc_chainloader.o: loader/i386/pc/chainloader.c
	$(CC) -Iloader/i386/pc -I$(srcdir)/loader/i386/pc $(CPPFLAGS) $(CFLAGS) $(chain_mod_CFLAGS) -c -o $@ $<

chain_mod-loader_i386_pc_chainloader.d: loader/i386/pc/chainloader.c
	set -e; 	  $(CC) -Iloader/i386/pc -I$(srcdir)/loader/i386/pc $(CPPFLAGS) $(CFLAGS) $(chain_mod_CFLAGS) -M $< 	  | sed 's,chainloader\.o[ :]*,chain_mod-loader_i386_pc_chainloader.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include chain_mod-loader_i386_pc_chainloader.d

chain_mod_CFLAGS = $(COMMON_CFLAGS)

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
	$(NM) -u -P -p $< >> $@

fat_mod-fs_fat.o: fs/fat.c
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(fat_mod_CFLAGS) -c -o $@ $<

fat_mod-fs_fat.d: fs/fat.c
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(fat_mod_CFLAGS) -M $< 	  | sed 's,fat\.o[ :]*,fat_mod-fs_fat.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include fat_mod-fs_fat.d

fat_mod_CFLAGS = $(COMMON_CFLAGS)
CLEANFILES += moddep.lst
pkgdata_DATA += moddep.lst
moddep.lst: $(DEFSYMFILES) $(UNDSYMFILES) genmoddep
	cat $(DEFSYMFILES) /dev/null | ./genmoddep $(UNDSYMFILES) > $@ \
	  || (rm -f $@; exit 1)
