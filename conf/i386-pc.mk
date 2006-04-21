# -*- makefile -*-

COMMON_ASFLAGS = -nostdinc -fno-builtin
COMMON_CFLAGS = -fno-builtin -mrtd -mregparm=3 -m32
COMMON_LDFLAGS = -melf_i386 -nostdlib

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
pxeboot_img_LDFLAGS = -nostdlib -Wl,-N,-Ttext,7C00

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
	kern/i386/dl.c kern/i386/pc/init.c kern/parser.c kern/partition.c \
	kern/env.c disk/i386/pc/biosdisk.c \
	term/i386/pc/console.c \
	symlist.c
CLEANFILES += kernel.img kernel.exec kernel_img-kern_i386_pc_startup.o kernel_img-kern_main.o kernel_img-kern_device.o kernel_img-kern_disk.o kernel_img-kern_dl.o kernel_img-kern_file.o kernel_img-kern_fs.o kernel_img-kern_err.o kernel_img-kern_misc.o kernel_img-kern_mm.o kernel_img-kern_loader.o kernel_img-kern_rescue.o kernel_img-kern_term.o kernel_img-kern_i386_dl.o kernel_img-kern_i386_pc_init.o kernel_img-kern_parser.o kernel_img-kern_partition.o kernel_img-kern_env.o kernel_img-disk_i386_pc_biosdisk.o kernel_img-term_i386_pc_console.o kernel_img-symlist.o
MOSTLYCLEANFILES += kernel_img-kern_i386_pc_startup.d kernel_img-kern_main.d kernel_img-kern_device.d kernel_img-kern_disk.d kernel_img-kern_dl.d kernel_img-kern_file.d kernel_img-kern_fs.d kernel_img-kern_err.d kernel_img-kern_misc.d kernel_img-kern_mm.d kernel_img-kern_loader.d kernel_img-kern_rescue.d kernel_img-kern_term.d kernel_img-kern_i386_dl.d kernel_img-kern_i386_pc_init.d kernel_img-kern_parser.d kernel_img-kern_partition.d kernel_img-kern_env.d kernel_img-disk_i386_pc_biosdisk.d kernel_img-term_i386_pc_console.d kernel_img-symlist.d

kernel.img: kernel.exec
	$(OBJCOPY) -O binary -R .note -R .comment $< $@

kernel.exec: kernel_img-kern_i386_pc_startup.o kernel_img-kern_main.o kernel_img-kern_device.o kernel_img-kern_disk.o kernel_img-kern_dl.o kernel_img-kern_file.o kernel_img-kern_fs.o kernel_img-kern_err.o kernel_img-kern_misc.o kernel_img-kern_mm.o kernel_img-kern_loader.o kernel_img-kern_rescue.o kernel_img-kern_term.o kernel_img-kern_i386_dl.o kernel_img-kern_i386_pc_init.o kernel_img-kern_parser.o kernel_img-kern_partition.o kernel_img-kern_env.o kernel_img-disk_i386_pc_biosdisk.o kernel_img-term_i386_pc_console.o kernel_img-symlist.o
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

kernel_img-kern_parser.o: kern/parser.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS)  $(CFLAGS) $(kernel_img_CFLAGS) -c -o $@ $<

kernel_img-kern_parser.d: kern/parser.c
	set -e; 	  $(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS)  $(CFLAGS) $(kernel_img_CFLAGS) -M $< 	  | sed 's,parser\.o[ :]*,kernel_img-kern_parser.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include kernel_img-kern_parser.d

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
	file.h fs.h kernel.h loader.h misc.h mm.h net.h parser.h partition.h \
	pc_partition.h rescue.h symbol.h term.h types.h \
	machine/biosdisk.h machine/boot.h machine/console.h machine/init.h \
	machine/memory.h machine/loader.h machine/time.h machine/vga.h \
	machine/vbe.h
kernel_img_CFLAGS = $(COMMON_CFLAGS)
kernel_img_ASFLAGS = $(COMMON_ASFLAGS)
kernel_img_LDFLAGS = -nostdlib -Wl,-N,-Ttext,8200 $(COMMON_CFLAGS)

MOSTLYCLEANFILES += symlist.c kernel_syms.lst
DEFSYMFILES += kernel_syms.lst

symlist.c: $(addprefix include/grub/,$(kernel_img_HEADERS)) gensymlist.sh
	sh $(srcdir)/gensymlist.sh $(filter %.h,$^) > $@

kernel_syms.lst: $(addprefix include/grub/,$(kernel_img_HEADERS)) genkernsyms.sh
	sh $(srcdir)/genkernsyms.sh $(filter %h,$^) > $@

# Utilities.
bin_UTILITIES = grub-mkimage
sbin_UTILITIES = grub-setup grub-emu grub-mkdevicemap grub-probefs
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

grub_mkimage_LDFLAGS = $(LIBLZO)

# For grub-setup.
grub_setup_SOURCES = util/i386/pc/grub-setup.c util/i386/pc/biosdisk.c	\
	util/misc.c util/i386/pc/getroot.c kern/device.c kern/disk.c	\
	kern/err.c kern/misc.c fs/fat.c fs/ext2.c fs/xfs.c fs/affs.c	\
	fs/sfs.c kern/parser.c kern/partition.c partmap/pc.c		\
	fs/ufs.c fs/minix.c fs/hfs.c fs/jfs.c fs/hfsplus.c kern/file.c	\
	kern/fs.c kern/env.c fs/fshelp.c
CLEANFILES += grub-setup grub_setup-util_i386_pc_grub_setup.o grub_setup-util_i386_pc_biosdisk.o grub_setup-util_misc.o grub_setup-util_i386_pc_getroot.o grub_setup-kern_device.o grub_setup-kern_disk.o grub_setup-kern_err.o grub_setup-kern_misc.o grub_setup-fs_fat.o grub_setup-fs_ext2.o grub_setup-fs_xfs.o grub_setup-fs_affs.o grub_setup-fs_sfs.o grub_setup-kern_parser.o grub_setup-kern_partition.o grub_setup-partmap_pc.o grub_setup-fs_ufs.o grub_setup-fs_minix.o grub_setup-fs_hfs.o grub_setup-fs_jfs.o grub_setup-fs_hfsplus.o grub_setup-kern_file.o grub_setup-kern_fs.o grub_setup-kern_env.o grub_setup-fs_fshelp.o
MOSTLYCLEANFILES += grub_setup-util_i386_pc_grub_setup.d grub_setup-util_i386_pc_biosdisk.d grub_setup-util_misc.d grub_setup-util_i386_pc_getroot.d grub_setup-kern_device.d grub_setup-kern_disk.d grub_setup-kern_err.d grub_setup-kern_misc.d grub_setup-fs_fat.d grub_setup-fs_ext2.d grub_setup-fs_xfs.d grub_setup-fs_affs.d grub_setup-fs_sfs.d grub_setup-kern_parser.d grub_setup-kern_partition.d grub_setup-partmap_pc.d grub_setup-fs_ufs.d grub_setup-fs_minix.d grub_setup-fs_hfs.d grub_setup-fs_jfs.d grub_setup-fs_hfsplus.d grub_setup-kern_file.d grub_setup-kern_fs.d grub_setup-kern_env.d grub_setup-fs_fshelp.d

grub-setup: grub_setup-util_i386_pc_grub_setup.o grub_setup-util_i386_pc_biosdisk.o grub_setup-util_misc.o grub_setup-util_i386_pc_getroot.o grub_setup-kern_device.o grub_setup-kern_disk.o grub_setup-kern_err.o grub_setup-kern_misc.o grub_setup-fs_fat.o grub_setup-fs_ext2.o grub_setup-fs_xfs.o grub_setup-fs_affs.o grub_setup-fs_sfs.o grub_setup-kern_parser.o grub_setup-kern_partition.o grub_setup-partmap_pc.o grub_setup-fs_ufs.o grub_setup-fs_minix.o grub_setup-fs_hfs.o grub_setup-fs_jfs.o grub_setup-fs_hfsplus.o grub_setup-kern_file.o grub_setup-kern_fs.o grub_setup-kern_env.o grub_setup-fs_fshelp.o
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

grub_setup-fs_xfs.o: fs/xfs.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -c -o $@ $<

grub_setup-fs_xfs.d: fs/xfs.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -M $< 	  | sed 's,xfs\.o[ :]*,grub_setup-fs_xfs.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_setup-fs_xfs.d

grub_setup-fs_affs.o: fs/affs.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -c -o $@ $<

grub_setup-fs_affs.d: fs/affs.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -M $< 	  | sed 's,affs\.o[ :]*,grub_setup-fs_affs.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_setup-fs_affs.d

grub_setup-fs_sfs.o: fs/sfs.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -c -o $@ $<

grub_setup-fs_sfs.d: fs/sfs.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -M $< 	  | sed 's,sfs\.o[ :]*,grub_setup-fs_sfs.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_setup-fs_sfs.d

grub_setup-kern_parser.o: kern/parser.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -c -o $@ $<

grub_setup-kern_parser.d: kern/parser.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -M $< 	  | sed 's,parser\.o[ :]*,grub_setup-kern_parser.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_setup-kern_parser.d

grub_setup-kern_partition.o: kern/partition.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -c -o $@ $<

grub_setup-kern_partition.d: kern/partition.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -M $< 	  | sed 's,partition\.o[ :]*,grub_setup-kern_partition.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_setup-kern_partition.d

grub_setup-partmap_pc.o: partmap/pc.c
	$(BUILD_CC) -Ipartmap -I$(srcdir)/partmap $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -c -o $@ $<

grub_setup-partmap_pc.d: partmap/pc.c
	set -e; 	  $(BUILD_CC) -Ipartmap -I$(srcdir)/partmap $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -M $< 	  | sed 's,pc\.o[ :]*,grub_setup-partmap_pc.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_setup-partmap_pc.d

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

grub_setup-fs_hfsplus.o: fs/hfsplus.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -c -o $@ $<

grub_setup-fs_hfsplus.d: fs/hfsplus.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_setup_CFLAGS) -M $< 	  | sed 's,hfsplus\.o[ :]*,grub_setup-fs_hfsplus.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_setup-fs_hfsplus.d

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


# For grub-mkdevicemap.
grub_mkdevicemap_SOURCES = util/i386/pc/grub-mkdevicemap.c util/misc.c
CLEANFILES += grub-mkdevicemap grub_mkdevicemap-util_i386_pc_grub_mkdevicemap.o grub_mkdevicemap-util_misc.o
MOSTLYCLEANFILES += grub_mkdevicemap-util_i386_pc_grub_mkdevicemap.d grub_mkdevicemap-util_misc.d

grub-mkdevicemap: grub_mkdevicemap-util_i386_pc_grub_mkdevicemap.o grub_mkdevicemap-util_misc.o
	$(BUILD_CC) -o $@ $^ $(BUILD_LDFLAGS) $(grub_mkdevicemap_LDFLAGS)

grub_mkdevicemap-util_i386_pc_grub_mkdevicemap.o: util/i386/pc/grub-mkdevicemap.c
	$(BUILD_CC) -Iutil/i386/pc -I$(srcdir)/util/i386/pc $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_mkdevicemap_CFLAGS) -c -o $@ $<

grub_mkdevicemap-util_i386_pc_grub_mkdevicemap.d: util/i386/pc/grub-mkdevicemap.c
	set -e; 	  $(BUILD_CC) -Iutil/i386/pc -I$(srcdir)/util/i386/pc $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_mkdevicemap_CFLAGS) -M $< 	  | sed 's,grub\-mkdevicemap\.o[ :]*,grub_mkdevicemap-util_i386_pc_grub_mkdevicemap.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_mkdevicemap-util_i386_pc_grub_mkdevicemap.d

grub_mkdevicemap-util_misc.o: util/misc.c
	$(BUILD_CC) -Iutil -I$(srcdir)/util $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_mkdevicemap_CFLAGS) -c -o $@ $<

grub_mkdevicemap-util_misc.d: util/misc.c
	set -e; 	  $(BUILD_CC) -Iutil -I$(srcdir)/util $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_mkdevicemap_CFLAGS) -M $< 	  | sed 's,misc\.o[ :]*,grub_mkdevicemap-util_misc.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_mkdevicemap-util_misc.d


# For grub-probefs.
grub_probefs_SOURCES = util/i386/pc/grub-probefs.c	\
	util/i386/pc/biosdisk.c	util/misc.c util/i386/pc/getroot.c	\
	kern/device.c kern/disk.c kern/err.c kern/misc.c fs/fat.c	\
	fs/ext2.c kern/parser.c kern/partition.c partmap/pc.c fs/ufs.c 	\
	fs/minix.c fs/hfs.c fs/jfs.c kern/fs.c kern/env.c fs/fshelp.c 	\
	fs/xfs.c fs/affs.c fs/sfs.c fs/hfsplus.c
CLEANFILES += grub-probefs grub_probefs-util_i386_pc_grub_probefs.o grub_probefs-util_i386_pc_biosdisk.o grub_probefs-util_misc.o grub_probefs-util_i386_pc_getroot.o grub_probefs-kern_device.o grub_probefs-kern_disk.o grub_probefs-kern_err.o grub_probefs-kern_misc.o grub_probefs-fs_fat.o grub_probefs-fs_ext2.o grub_probefs-kern_parser.o grub_probefs-kern_partition.o grub_probefs-partmap_pc.o grub_probefs-fs_ufs.o grub_probefs-fs_minix.o grub_probefs-fs_hfs.o grub_probefs-fs_jfs.o grub_probefs-kern_fs.o grub_probefs-kern_env.o grub_probefs-fs_fshelp.o grub_probefs-fs_xfs.o grub_probefs-fs_affs.o grub_probefs-fs_sfs.o grub_probefs-fs_hfsplus.o
MOSTLYCLEANFILES += grub_probefs-util_i386_pc_grub_probefs.d grub_probefs-util_i386_pc_biosdisk.d grub_probefs-util_misc.d grub_probefs-util_i386_pc_getroot.d grub_probefs-kern_device.d grub_probefs-kern_disk.d grub_probefs-kern_err.d grub_probefs-kern_misc.d grub_probefs-fs_fat.d grub_probefs-fs_ext2.d grub_probefs-kern_parser.d grub_probefs-kern_partition.d grub_probefs-partmap_pc.d grub_probefs-fs_ufs.d grub_probefs-fs_minix.d grub_probefs-fs_hfs.d grub_probefs-fs_jfs.d grub_probefs-kern_fs.d grub_probefs-kern_env.d grub_probefs-fs_fshelp.d grub_probefs-fs_xfs.d grub_probefs-fs_affs.d grub_probefs-fs_sfs.d grub_probefs-fs_hfsplus.d

grub-probefs: grub_probefs-util_i386_pc_grub_probefs.o grub_probefs-util_i386_pc_biosdisk.o grub_probefs-util_misc.o grub_probefs-util_i386_pc_getroot.o grub_probefs-kern_device.o grub_probefs-kern_disk.o grub_probefs-kern_err.o grub_probefs-kern_misc.o grub_probefs-fs_fat.o grub_probefs-fs_ext2.o grub_probefs-kern_parser.o grub_probefs-kern_partition.o grub_probefs-partmap_pc.o grub_probefs-fs_ufs.o grub_probefs-fs_minix.o grub_probefs-fs_hfs.o grub_probefs-fs_jfs.o grub_probefs-kern_fs.o grub_probefs-kern_env.o grub_probefs-fs_fshelp.o grub_probefs-fs_xfs.o grub_probefs-fs_affs.o grub_probefs-fs_sfs.o grub_probefs-fs_hfsplus.o
	$(BUILD_CC) -o $@ $^ $(BUILD_LDFLAGS) $(grub_probefs_LDFLAGS)

grub_probefs-util_i386_pc_grub_probefs.o: util/i386/pc/grub-probefs.c
	$(BUILD_CC) -Iutil/i386/pc -I$(srcdir)/util/i386/pc $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_probefs_CFLAGS) -c -o $@ $<

grub_probefs-util_i386_pc_grub_probefs.d: util/i386/pc/grub-probefs.c
	set -e; 	  $(BUILD_CC) -Iutil/i386/pc -I$(srcdir)/util/i386/pc $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_probefs_CFLAGS) -M $< 	  | sed 's,grub\-probefs\.o[ :]*,grub_probefs-util_i386_pc_grub_probefs.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_probefs-util_i386_pc_grub_probefs.d

grub_probefs-util_i386_pc_biosdisk.o: util/i386/pc/biosdisk.c
	$(BUILD_CC) -Iutil/i386/pc -I$(srcdir)/util/i386/pc $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_probefs_CFLAGS) -c -o $@ $<

grub_probefs-util_i386_pc_biosdisk.d: util/i386/pc/biosdisk.c
	set -e; 	  $(BUILD_CC) -Iutil/i386/pc -I$(srcdir)/util/i386/pc $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_probefs_CFLAGS) -M $< 	  | sed 's,biosdisk\.o[ :]*,grub_probefs-util_i386_pc_biosdisk.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_probefs-util_i386_pc_biosdisk.d

grub_probefs-util_misc.o: util/misc.c
	$(BUILD_CC) -Iutil -I$(srcdir)/util $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_probefs_CFLAGS) -c -o $@ $<

grub_probefs-util_misc.d: util/misc.c
	set -e; 	  $(BUILD_CC) -Iutil -I$(srcdir)/util $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_probefs_CFLAGS) -M $< 	  | sed 's,misc\.o[ :]*,grub_probefs-util_misc.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_probefs-util_misc.d

grub_probefs-util_i386_pc_getroot.o: util/i386/pc/getroot.c
	$(BUILD_CC) -Iutil/i386/pc -I$(srcdir)/util/i386/pc $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_probefs_CFLAGS) -c -o $@ $<

grub_probefs-util_i386_pc_getroot.d: util/i386/pc/getroot.c
	set -e; 	  $(BUILD_CC) -Iutil/i386/pc -I$(srcdir)/util/i386/pc $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_probefs_CFLAGS) -M $< 	  | sed 's,getroot\.o[ :]*,grub_probefs-util_i386_pc_getroot.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_probefs-util_i386_pc_getroot.d

grub_probefs-kern_device.o: kern/device.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_probefs_CFLAGS) -c -o $@ $<

grub_probefs-kern_device.d: kern/device.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_probefs_CFLAGS) -M $< 	  | sed 's,device\.o[ :]*,grub_probefs-kern_device.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_probefs-kern_device.d

grub_probefs-kern_disk.o: kern/disk.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_probefs_CFLAGS) -c -o $@ $<

grub_probefs-kern_disk.d: kern/disk.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_probefs_CFLAGS) -M $< 	  | sed 's,disk\.o[ :]*,grub_probefs-kern_disk.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_probefs-kern_disk.d

grub_probefs-kern_err.o: kern/err.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_probefs_CFLAGS) -c -o $@ $<

grub_probefs-kern_err.d: kern/err.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_probefs_CFLAGS) -M $< 	  | sed 's,err\.o[ :]*,grub_probefs-kern_err.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_probefs-kern_err.d

grub_probefs-kern_misc.o: kern/misc.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_probefs_CFLAGS) -c -o $@ $<

grub_probefs-kern_misc.d: kern/misc.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_probefs_CFLAGS) -M $< 	  | sed 's,misc\.o[ :]*,grub_probefs-kern_misc.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_probefs-kern_misc.d

grub_probefs-fs_fat.o: fs/fat.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_probefs_CFLAGS) -c -o $@ $<

grub_probefs-fs_fat.d: fs/fat.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_probefs_CFLAGS) -M $< 	  | sed 's,fat\.o[ :]*,grub_probefs-fs_fat.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_probefs-fs_fat.d

grub_probefs-fs_ext2.o: fs/ext2.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_probefs_CFLAGS) -c -o $@ $<

grub_probefs-fs_ext2.d: fs/ext2.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_probefs_CFLAGS) -M $< 	  | sed 's,ext2\.o[ :]*,grub_probefs-fs_ext2.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_probefs-fs_ext2.d

grub_probefs-kern_parser.o: kern/parser.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_probefs_CFLAGS) -c -o $@ $<

grub_probefs-kern_parser.d: kern/parser.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_probefs_CFLAGS) -M $< 	  | sed 's,parser\.o[ :]*,grub_probefs-kern_parser.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_probefs-kern_parser.d

grub_probefs-kern_partition.o: kern/partition.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_probefs_CFLAGS) -c -o $@ $<

grub_probefs-kern_partition.d: kern/partition.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_probefs_CFLAGS) -M $< 	  | sed 's,partition\.o[ :]*,grub_probefs-kern_partition.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_probefs-kern_partition.d

grub_probefs-partmap_pc.o: partmap/pc.c
	$(BUILD_CC) -Ipartmap -I$(srcdir)/partmap $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_probefs_CFLAGS) -c -o $@ $<

grub_probefs-partmap_pc.d: partmap/pc.c
	set -e; 	  $(BUILD_CC) -Ipartmap -I$(srcdir)/partmap $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_probefs_CFLAGS) -M $< 	  | sed 's,pc\.o[ :]*,grub_probefs-partmap_pc.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_probefs-partmap_pc.d

grub_probefs-fs_ufs.o: fs/ufs.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_probefs_CFLAGS) -c -o $@ $<

grub_probefs-fs_ufs.d: fs/ufs.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_probefs_CFLAGS) -M $< 	  | sed 's,ufs\.o[ :]*,grub_probefs-fs_ufs.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_probefs-fs_ufs.d

grub_probefs-fs_minix.o: fs/minix.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_probefs_CFLAGS) -c -o $@ $<

grub_probefs-fs_minix.d: fs/minix.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_probefs_CFLAGS) -M $< 	  | sed 's,minix\.o[ :]*,grub_probefs-fs_minix.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_probefs-fs_minix.d

grub_probefs-fs_hfs.o: fs/hfs.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_probefs_CFLAGS) -c -o $@ $<

grub_probefs-fs_hfs.d: fs/hfs.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_probefs_CFLAGS) -M $< 	  | sed 's,hfs\.o[ :]*,grub_probefs-fs_hfs.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_probefs-fs_hfs.d

grub_probefs-fs_jfs.o: fs/jfs.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_probefs_CFLAGS) -c -o $@ $<

grub_probefs-fs_jfs.d: fs/jfs.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_probefs_CFLAGS) -M $< 	  | sed 's,jfs\.o[ :]*,grub_probefs-fs_jfs.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_probefs-fs_jfs.d

grub_probefs-kern_fs.o: kern/fs.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_probefs_CFLAGS) -c -o $@ $<

grub_probefs-kern_fs.d: kern/fs.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_probefs_CFLAGS) -M $< 	  | sed 's,fs\.o[ :]*,grub_probefs-kern_fs.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_probefs-kern_fs.d

grub_probefs-kern_env.o: kern/env.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_probefs_CFLAGS) -c -o $@ $<

grub_probefs-kern_env.d: kern/env.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_probefs_CFLAGS) -M $< 	  | sed 's,env\.o[ :]*,grub_probefs-kern_env.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_probefs-kern_env.d

grub_probefs-fs_fshelp.o: fs/fshelp.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_probefs_CFLAGS) -c -o $@ $<

grub_probefs-fs_fshelp.d: fs/fshelp.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_probefs_CFLAGS) -M $< 	  | sed 's,fshelp\.o[ :]*,grub_probefs-fs_fshelp.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_probefs-fs_fshelp.d

grub_probefs-fs_xfs.o: fs/xfs.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_probefs_CFLAGS) -c -o $@ $<

grub_probefs-fs_xfs.d: fs/xfs.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_probefs_CFLAGS) -M $< 	  | sed 's,xfs\.o[ :]*,grub_probefs-fs_xfs.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_probefs-fs_xfs.d

grub_probefs-fs_affs.o: fs/affs.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_probefs_CFLAGS) -c -o $@ $<

grub_probefs-fs_affs.d: fs/affs.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_probefs_CFLAGS) -M $< 	  | sed 's,affs\.o[ :]*,grub_probefs-fs_affs.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_probefs-fs_affs.d

grub_probefs-fs_sfs.o: fs/sfs.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_probefs_CFLAGS) -c -o $@ $<

grub_probefs-fs_sfs.d: fs/sfs.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_probefs_CFLAGS) -M $< 	  | sed 's,sfs\.o[ :]*,grub_probefs-fs_sfs.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_probefs-fs_sfs.d

grub_probefs-fs_hfsplus.o: fs/hfsplus.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_probefs_CFLAGS) -c -o $@ $<

grub_probefs-fs_hfsplus.d: fs/hfsplus.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_probefs_CFLAGS) -M $< 	  | sed 's,hfsplus\.o[ :]*,grub_probefs-fs_hfsplus.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_probefs-fs_hfsplus.d


# For grub-emu.
grub_emu_SOURCES = commands/boot.c commands/cat.c commands/cmp.c 		\
	commands/configfile.c commands/default.c commands/help.c	\
	commands/terminal.c commands/ls.c commands/test.c 		\
	commands/search.c commands/timeout.c				\
	commands/i386/pc/halt.c commands/i386/pc/reboot.c		\
	disk/loopback.c							\
	fs/affs.c fs/ext2.c fs/fat.c fs/fshelp.c fs/hfs.c fs/iso9660.c	\
	fs/jfs.c fs/minix.c fs/sfs.c fs/ufs.c fs/xfs.c fs/hfsplus.c	\
	io/gzio.c							\
	kern/device.c kern/disk.c kern/dl.c kern/env.c kern/err.c 	\
	normal/execute.c kern/file.c kern/fs.c normal/lexer.c 		\
	kern/loader.c kern/main.c kern/misc.c kern/parser.c		\
	grub_script.tab.c kern/partition.c kern/rescue.c kern/term.c	\
	normal/arg.c normal/cmdline.c normal/command.c normal/function.c\
	normal/completion.c normal/context.c normal/main.c		\
	normal/menu.c normal/menu_entry.c normal/misc.c normal/script.c	\
	partmap/amiga.c	partmap/apple.c partmap/pc.c partmap/sun.c	\
	partmap/acorn.c partmap/gpt.c					\
	util/console.c util/grub-emu.c util/misc.c			\
	util/i386/pc/biosdisk.c util/i386/pc/getroot.c			\
	util/i386/pc/misc.c grub_emu_init.c
CLEANFILES += grub-emu grub_emu-commands_boot.o grub_emu-commands_cat.o grub_emu-commands_cmp.o grub_emu-commands_configfile.o grub_emu-commands_default.o grub_emu-commands_help.o grub_emu-commands_terminal.o grub_emu-commands_ls.o grub_emu-commands_test.o grub_emu-commands_search.o grub_emu-commands_timeout.o grub_emu-commands_i386_pc_halt.o grub_emu-commands_i386_pc_reboot.o grub_emu-disk_loopback.o grub_emu-fs_affs.o grub_emu-fs_ext2.o grub_emu-fs_fat.o grub_emu-fs_fshelp.o grub_emu-fs_hfs.o grub_emu-fs_iso9660.o grub_emu-fs_jfs.o grub_emu-fs_minix.o grub_emu-fs_sfs.o grub_emu-fs_ufs.o grub_emu-fs_xfs.o grub_emu-fs_hfsplus.o grub_emu-io_gzio.o grub_emu-kern_device.o grub_emu-kern_disk.o grub_emu-kern_dl.o grub_emu-kern_env.o grub_emu-kern_err.o grub_emu-normal_execute.o grub_emu-kern_file.o grub_emu-kern_fs.o grub_emu-normal_lexer.o grub_emu-kern_loader.o grub_emu-kern_main.o grub_emu-kern_misc.o grub_emu-kern_parser.o grub_emu-grub_script_tab.o grub_emu-kern_partition.o grub_emu-kern_rescue.o grub_emu-kern_term.o grub_emu-normal_arg.o grub_emu-normal_cmdline.o grub_emu-normal_command.o grub_emu-normal_function.o grub_emu-normal_completion.o grub_emu-normal_context.o grub_emu-normal_main.o grub_emu-normal_menu.o grub_emu-normal_menu_entry.o grub_emu-normal_misc.o grub_emu-normal_script.o grub_emu-partmap_amiga.o grub_emu-partmap_apple.o grub_emu-partmap_pc.o grub_emu-partmap_sun.o grub_emu-partmap_acorn.o grub_emu-partmap_gpt.o grub_emu-util_console.o grub_emu-util_grub_emu.o grub_emu-util_misc.o grub_emu-util_i386_pc_biosdisk.o grub_emu-util_i386_pc_getroot.o grub_emu-util_i386_pc_misc.o grub_emu-grub_emu_init.o
MOSTLYCLEANFILES += grub_emu-commands_boot.d grub_emu-commands_cat.d grub_emu-commands_cmp.d grub_emu-commands_configfile.d grub_emu-commands_default.d grub_emu-commands_help.d grub_emu-commands_terminal.d grub_emu-commands_ls.d grub_emu-commands_test.d grub_emu-commands_search.d grub_emu-commands_timeout.d grub_emu-commands_i386_pc_halt.d grub_emu-commands_i386_pc_reboot.d grub_emu-disk_loopback.d grub_emu-fs_affs.d grub_emu-fs_ext2.d grub_emu-fs_fat.d grub_emu-fs_fshelp.d grub_emu-fs_hfs.d grub_emu-fs_iso9660.d grub_emu-fs_jfs.d grub_emu-fs_minix.d grub_emu-fs_sfs.d grub_emu-fs_ufs.d grub_emu-fs_xfs.d grub_emu-fs_hfsplus.d grub_emu-io_gzio.d grub_emu-kern_device.d grub_emu-kern_disk.d grub_emu-kern_dl.d grub_emu-kern_env.d grub_emu-kern_err.d grub_emu-normal_execute.d grub_emu-kern_file.d grub_emu-kern_fs.d grub_emu-normal_lexer.d grub_emu-kern_loader.d grub_emu-kern_main.d grub_emu-kern_misc.d grub_emu-kern_parser.d grub_emu-grub_script_tab.d grub_emu-kern_partition.d grub_emu-kern_rescue.d grub_emu-kern_term.d grub_emu-normal_arg.d grub_emu-normal_cmdline.d grub_emu-normal_command.d grub_emu-normal_function.d grub_emu-normal_completion.d grub_emu-normal_context.d grub_emu-normal_main.d grub_emu-normal_menu.d grub_emu-normal_menu_entry.d grub_emu-normal_misc.d grub_emu-normal_script.d grub_emu-partmap_amiga.d grub_emu-partmap_apple.d grub_emu-partmap_pc.d grub_emu-partmap_sun.d grub_emu-partmap_acorn.d grub_emu-partmap_gpt.d grub_emu-util_console.d grub_emu-util_grub_emu.d grub_emu-util_misc.d grub_emu-util_i386_pc_biosdisk.d grub_emu-util_i386_pc_getroot.d grub_emu-util_i386_pc_misc.d grub_emu-grub_emu_init.d

grub-emu: grub_emu-commands_boot.o grub_emu-commands_cat.o grub_emu-commands_cmp.o grub_emu-commands_configfile.o grub_emu-commands_default.o grub_emu-commands_help.o grub_emu-commands_terminal.o grub_emu-commands_ls.o grub_emu-commands_test.o grub_emu-commands_search.o grub_emu-commands_timeout.o grub_emu-commands_i386_pc_halt.o grub_emu-commands_i386_pc_reboot.o grub_emu-disk_loopback.o grub_emu-fs_affs.o grub_emu-fs_ext2.o grub_emu-fs_fat.o grub_emu-fs_fshelp.o grub_emu-fs_hfs.o grub_emu-fs_iso9660.o grub_emu-fs_jfs.o grub_emu-fs_minix.o grub_emu-fs_sfs.o grub_emu-fs_ufs.o grub_emu-fs_xfs.o grub_emu-fs_hfsplus.o grub_emu-io_gzio.o grub_emu-kern_device.o grub_emu-kern_disk.o grub_emu-kern_dl.o grub_emu-kern_env.o grub_emu-kern_err.o grub_emu-normal_execute.o grub_emu-kern_file.o grub_emu-kern_fs.o grub_emu-normal_lexer.o grub_emu-kern_loader.o grub_emu-kern_main.o grub_emu-kern_misc.o grub_emu-kern_parser.o grub_emu-grub_script_tab.o grub_emu-kern_partition.o grub_emu-kern_rescue.o grub_emu-kern_term.o grub_emu-normal_arg.o grub_emu-normal_cmdline.o grub_emu-normal_command.o grub_emu-normal_function.o grub_emu-normal_completion.o grub_emu-normal_context.o grub_emu-normal_main.o grub_emu-normal_menu.o grub_emu-normal_menu_entry.o grub_emu-normal_misc.o grub_emu-normal_script.o grub_emu-partmap_amiga.o grub_emu-partmap_apple.o grub_emu-partmap_pc.o grub_emu-partmap_sun.o grub_emu-partmap_acorn.o grub_emu-partmap_gpt.o grub_emu-util_console.o grub_emu-util_grub_emu.o grub_emu-util_misc.o grub_emu-util_i386_pc_biosdisk.o grub_emu-util_i386_pc_getroot.o grub_emu-util_i386_pc_misc.o grub_emu-grub_emu_init.o
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

grub_emu-commands_test.o: commands/test.c
	$(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-commands_test.d: commands/test.c
	set -e; 	  $(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,test\.o[ :]*,grub_emu-commands_test.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-commands_test.d

grub_emu-commands_search.o: commands/search.c
	$(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-commands_search.d: commands/search.c
	set -e; 	  $(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,search\.o[ :]*,grub_emu-commands_search.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-commands_search.d

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

grub_emu-normal_execute.o: normal/execute.c
	$(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-normal_execute.d: normal/execute.c
	set -e; 	  $(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,execute\.o[ :]*,grub_emu-normal_execute.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-normal_execute.d

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

grub_emu-normal_lexer.o: normal/lexer.c
	$(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-normal_lexer.d: normal/lexer.c
	set -e; 	  $(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,lexer\.o[ :]*,grub_emu-normal_lexer.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-normal_lexer.d

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

grub_emu-grub_script_tab.o: grub_script.tab.c
	$(BUILD_CC) -I. -I$(srcdir)/. $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-grub_script_tab.d: grub_script.tab.c
	set -e; 	  $(BUILD_CC) -I. -I$(srcdir)/. $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,grub_script\.tab\.o[ :]*,grub_emu-grub_script_tab.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-grub_script_tab.d

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

grub_emu-normal_function.o: normal/function.c
	$(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-normal_function.d: normal/function.c
	set -e; 	  $(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,function\.o[ :]*,grub_emu-normal_function.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-normal_function.d

grub_emu-normal_completion.o: normal/completion.c
	$(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-normal_completion.d: normal/completion.c
	set -e; 	  $(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,completion\.o[ :]*,grub_emu-normal_completion.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-normal_completion.d

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

grub_emu-partmap_gpt.o: partmap/gpt.c
	$(BUILD_CC) -Ipartmap -I$(srcdir)/partmap $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-partmap_gpt.d: partmap/gpt.c
	set -e; 	  $(BUILD_CC) -Ipartmap -I$(srcdir)/partmap $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,gpt\.o[ :]*,grub_emu-partmap_gpt.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-partmap_gpt.d

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

grub_emu-grub_emu_init.o: grub_emu_init.c
	$(BUILD_CC) -I. -I$(srcdir)/. $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-grub_emu_init.d: grub_emu_init.c
	set -e; 	  $(BUILD_CC) -I. -I$(srcdir)/. $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,grub_emu_init\.o[ :]*,grub_emu-grub_emu_init.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-grub_emu_init.d


grub_emu_LDFLAGS = $(LIBCURSES)

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
grub_install_SOURCES = util/i386/pc/grub-install.in
CLEANFILES += grub-install

grub-install: util/i386/pc/grub-install.in config.status
	./config.status --file=grub-install:util/i386/pc/grub-install.in
	chmod +x $@


# Modules.
pkgdata_MODULES = _chain.mod _linux.mod linux.mod normal.mod \
	_multiboot.mod chain.mod multiboot.mod reboot.mod halt.mod	\
	vbe.mod vbetest.mod vbeinfo.mod video.mod gfxterm.mod \
	videotest.mod play.mod

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
	$(CC) $(_chain_mod_LDFLAGS) $(LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-_chain.o: _chain_mod-loader_i386_pc_chainloader.o
	-rm -f $@
	$(CC) $(_chain_mod_LDFLAGS) $(LDFLAGS) -Wl,-r,-d -o $@ $^

mod-_chain.o: mod-_chain.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(_chain_mod_CFLAGS) -c -o $@ $<

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
	$(CC) -Iloader/i386/pc -I$(srcdir)/loader/i386/pc $(CPPFLAGS) $(CFLAGS) $(_chain_mod_CFLAGS) -c -o $@ $<

_chain_mod-loader_i386_pc_chainloader.d: loader/i386/pc/chainloader.c
	set -e; 	  $(CC) -Iloader/i386/pc -I$(srcdir)/loader/i386/pc $(CPPFLAGS) $(CFLAGS) $(_chain_mod_CFLAGS) -M $< 	  | sed 's,chainloader\.o[ :]*,_chain_mod-loader_i386_pc_chainloader.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include _chain_mod-loader_i386_pc_chainloader.d

CLEANFILES += cmd-_chain_mod-loader_i386_pc_chainloader.lst fs-_chain_mod-loader_i386_pc_chainloader.lst
COMMANDFILES += cmd-_chain_mod-loader_i386_pc_chainloader.lst
FSFILES += fs-_chain_mod-loader_i386_pc_chainloader.lst

cmd-_chain_mod-loader_i386_pc_chainloader.lst: loader/i386/pc/chainloader.c gencmdlist.sh
	set -e; 	  $(CC) -Iloader/i386/pc -I$(srcdir)/loader/i386/pc $(CPPFLAGS) $(CFLAGS) $(_chain_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh _chain > $@ || (rm -f $@; exit 1)

fs-_chain_mod-loader_i386_pc_chainloader.lst: loader/i386/pc/chainloader.c genfslist.sh
	set -e; 	  $(CC) -Iloader/i386/pc -I$(srcdir)/loader/i386/pc $(CPPFLAGS) $(CFLAGS) $(_chain_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh _chain > $@ || (rm -f $@; exit 1)


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
	$(CC) $(chain_mod_LDFLAGS) $(LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-chain.o: chain_mod-loader_i386_pc_chainloader_normal.o
	-rm -f $@
	$(CC) $(chain_mod_LDFLAGS) $(LDFLAGS) -Wl,-r,-d -o $@ $^

mod-chain.o: mod-chain.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(chain_mod_CFLAGS) -c -o $@ $<

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
	$(CC) -Iloader/i386/pc -I$(srcdir)/loader/i386/pc $(CPPFLAGS) $(CFLAGS) $(chain_mod_CFLAGS) -c -o $@ $<

chain_mod-loader_i386_pc_chainloader_normal.d: loader/i386/pc/chainloader_normal.c
	set -e; 	  $(CC) -Iloader/i386/pc -I$(srcdir)/loader/i386/pc $(CPPFLAGS) $(CFLAGS) $(chain_mod_CFLAGS) -M $< 	  | sed 's,chainloader_normal\.o[ :]*,chain_mod-loader_i386_pc_chainloader_normal.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include chain_mod-loader_i386_pc_chainloader_normal.d

CLEANFILES += cmd-chain_mod-loader_i386_pc_chainloader_normal.lst fs-chain_mod-loader_i386_pc_chainloader_normal.lst
COMMANDFILES += cmd-chain_mod-loader_i386_pc_chainloader_normal.lst
FSFILES += fs-chain_mod-loader_i386_pc_chainloader_normal.lst

cmd-chain_mod-loader_i386_pc_chainloader_normal.lst: loader/i386/pc/chainloader_normal.c gencmdlist.sh
	set -e; 	  $(CC) -Iloader/i386/pc -I$(srcdir)/loader/i386/pc $(CPPFLAGS) $(CFLAGS) $(chain_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh chain > $@ || (rm -f $@; exit 1)

fs-chain_mod-loader_i386_pc_chainloader_normal.lst: loader/i386/pc/chainloader_normal.c genfslist.sh
	set -e; 	  $(CC) -Iloader/i386/pc -I$(srcdir)/loader/i386/pc $(CPPFLAGS) $(CFLAGS) $(chain_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh chain > $@ || (rm -f $@; exit 1)


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
	$(CC) $(_linux_mod_LDFLAGS) $(LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-_linux.o: _linux_mod-loader_i386_pc_linux.o
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

_linux_mod-loader_i386_pc_linux.o: loader/i386/pc/linux.c
	$(CC) -Iloader/i386/pc -I$(srcdir)/loader/i386/pc $(CPPFLAGS) $(CFLAGS) $(_linux_mod_CFLAGS) -c -o $@ $<

_linux_mod-loader_i386_pc_linux.d: loader/i386/pc/linux.c
	set -e; 	  $(CC) -Iloader/i386/pc -I$(srcdir)/loader/i386/pc $(CPPFLAGS) $(CFLAGS) $(_linux_mod_CFLAGS) -M $< 	  | sed 's,linux\.o[ :]*,_linux_mod-loader_i386_pc_linux.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include _linux_mod-loader_i386_pc_linux.d

CLEANFILES += cmd-_linux_mod-loader_i386_pc_linux.lst fs-_linux_mod-loader_i386_pc_linux.lst
COMMANDFILES += cmd-_linux_mod-loader_i386_pc_linux.lst
FSFILES += fs-_linux_mod-loader_i386_pc_linux.lst

cmd-_linux_mod-loader_i386_pc_linux.lst: loader/i386/pc/linux.c gencmdlist.sh
	set -e; 	  $(CC) -Iloader/i386/pc -I$(srcdir)/loader/i386/pc $(CPPFLAGS) $(CFLAGS) $(_linux_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh _linux > $@ || (rm -f $@; exit 1)

fs-_linux_mod-loader_i386_pc_linux.lst: loader/i386/pc/linux.c genfslist.sh
	set -e; 	  $(CC) -Iloader/i386/pc -I$(srcdir)/loader/i386/pc $(CPPFLAGS) $(CFLAGS) $(_linux_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh _linux > $@ || (rm -f $@; exit 1)


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
	$(CC) $(linux_mod_LDFLAGS) $(LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-linux.o: linux_mod-loader_i386_pc_linux_normal.o
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

linux_mod-loader_i386_pc_linux_normal.o: loader/i386/pc/linux_normal.c
	$(CC) -Iloader/i386/pc -I$(srcdir)/loader/i386/pc $(CPPFLAGS) $(CFLAGS) $(linux_mod_CFLAGS) -c -o $@ $<

linux_mod-loader_i386_pc_linux_normal.d: loader/i386/pc/linux_normal.c
	set -e; 	  $(CC) -Iloader/i386/pc -I$(srcdir)/loader/i386/pc $(CPPFLAGS) $(CFLAGS) $(linux_mod_CFLAGS) -M $< 	  | sed 's,linux_normal\.o[ :]*,linux_mod-loader_i386_pc_linux_normal.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include linux_mod-loader_i386_pc_linux_normal.d

CLEANFILES += cmd-linux_mod-loader_i386_pc_linux_normal.lst fs-linux_mod-loader_i386_pc_linux_normal.lst
COMMANDFILES += cmd-linux_mod-loader_i386_pc_linux_normal.lst
FSFILES += fs-linux_mod-loader_i386_pc_linux_normal.lst

cmd-linux_mod-loader_i386_pc_linux_normal.lst: loader/i386/pc/linux_normal.c gencmdlist.sh
	set -e; 	  $(CC) -Iloader/i386/pc -I$(srcdir)/loader/i386/pc $(CPPFLAGS) $(CFLAGS) $(linux_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh linux > $@ || (rm -f $@; exit 1)

fs-linux_mod-loader_i386_pc_linux_normal.lst: loader/i386/pc/linux_normal.c genfslist.sh
	set -e; 	  $(CC) -Iloader/i386/pc -I$(srcdir)/loader/i386/pc $(CPPFLAGS) $(CFLAGS) $(linux_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh linux > $@ || (rm -f $@; exit 1)


linux_mod_CFLAGS = $(COMMON_CFLAGS)
linux_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For normal.mod.
normal_mod_SOURCES = normal/arg.c normal/cmdline.c normal/command.c	\
	normal/completion.c normal/context.c normal/execute.c 		\
	normal/function.c normal/lexer.c normal/main.c normal/menu.c	\
	normal/menu_entry.c normal/misc.c grub_script.tab.c 		\
	normal/script.c normal/i386/setjmp.S
CLEANFILES += normal.mod mod-normal.o mod-normal.c pre-normal.o normal_mod-normal_arg.o normal_mod-normal_cmdline.o normal_mod-normal_command.o normal_mod-normal_completion.o normal_mod-normal_context.o normal_mod-normal_execute.o normal_mod-normal_function.o normal_mod-normal_lexer.o normal_mod-normal_main.o normal_mod-normal_menu.o normal_mod-normal_menu_entry.o normal_mod-normal_misc.o normal_mod-grub_script_tab.o normal_mod-normal_script.o normal_mod-normal_i386_setjmp.o und-normal.lst
ifneq ($(normal_mod_EXPORTS),no)
CLEANFILES += def-normal.lst
DEFSYMFILES += def-normal.lst
endif
MOSTLYCLEANFILES += normal_mod-normal_arg.d normal_mod-normal_cmdline.d normal_mod-normal_command.d normal_mod-normal_completion.d normal_mod-normal_context.d normal_mod-normal_execute.d normal_mod-normal_function.d normal_mod-normal_lexer.d normal_mod-normal_main.d normal_mod-normal_menu.d normal_mod-normal_menu_entry.d normal_mod-normal_misc.d normal_mod-grub_script_tab.d normal_mod-normal_script.d normal_mod-normal_i386_setjmp.d
UNDSYMFILES += und-normal.lst

normal.mod: pre-normal.o mod-normal.o
	-rm -f $@
	$(CC) $(normal_mod_LDFLAGS) $(LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-normal.o: normal_mod-normal_arg.o normal_mod-normal_cmdline.o normal_mod-normal_command.o normal_mod-normal_completion.o normal_mod-normal_context.o normal_mod-normal_execute.o normal_mod-normal_function.o normal_mod-normal_lexer.o normal_mod-normal_main.o normal_mod-normal_menu.o normal_mod-normal_menu_entry.o normal_mod-normal_misc.o normal_mod-grub_script_tab.o normal_mod-normal_script.o normal_mod-normal_i386_setjmp.o
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


normal_mod-normal_context.o: normal/context.c
	$(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -c -o $@ $<

normal_mod-normal_context.d: normal/context.c
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -M $< 	  | sed 's,context\.o[ :]*,normal_mod-normal_context.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include normal_mod-normal_context.d

CLEANFILES += cmd-normal_mod-normal_context.lst fs-normal_mod-normal_context.lst
COMMANDFILES += cmd-normal_mod-normal_context.lst
FSFILES += fs-normal_mod-normal_context.lst

cmd-normal_mod-normal_context.lst: normal/context.c gencmdlist.sh
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-normal_context.lst: normal/context.c genfslist.sh
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


normal_mod-normal_i386_setjmp.o: normal/i386/setjmp.S
	$(CC) -Inormal/i386 -I$(srcdir)/normal/i386 $(CPPFLAGS) $(ASFLAGS) $(normal_mod_ASFLAGS) -c -o $@ $<

normal_mod-normal_i386_setjmp.d: normal/i386/setjmp.S
	set -e; 	  $(CC) -Inormal/i386 -I$(srcdir)/normal/i386 $(CPPFLAGS) $(ASFLAGS) $(normal_mod_ASFLAGS) -M $< 	  | sed 's,setjmp\.o[ :]*,normal_mod-normal_i386_setjmp.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include normal_mod-normal_i386_setjmp.d

CLEANFILES += cmd-normal_mod-normal_i386_setjmp.lst fs-normal_mod-normal_i386_setjmp.lst
COMMANDFILES += cmd-normal_mod-normal_i386_setjmp.lst
FSFILES += fs-normal_mod-normal_i386_setjmp.lst

cmd-normal_mod-normal_i386_setjmp.lst: normal/i386/setjmp.S gencmdlist.sh
	set -e; 	  $(CC) -Inormal/i386 -I$(srcdir)/normal/i386 $(CPPFLAGS) $(ASFLAGS) $(normal_mod_ASFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-normal_i386_setjmp.lst: normal/i386/setjmp.S genfslist.sh
	set -e; 	  $(CC) -Inormal/i386 -I$(srcdir)/normal/i386 $(CPPFLAGS) $(ASFLAGS) $(normal_mod_ASFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod_CFLAGS = $(COMMON_CFLAGS)
normal_mod_ASFLAGS = $(COMMON_ASFLAGS) -m32
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
	$(CC) $(reboot_mod_LDFLAGS) $(LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-reboot.o: reboot_mod-commands_i386_pc_reboot.o
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

reboot_mod-commands_i386_pc_reboot.o: commands/i386/pc/reboot.c
	$(CC) -Icommands/i386/pc -I$(srcdir)/commands/i386/pc $(CPPFLAGS) $(CFLAGS) $(reboot_mod_CFLAGS) -c -o $@ $<

reboot_mod-commands_i386_pc_reboot.d: commands/i386/pc/reboot.c
	set -e; 	  $(CC) -Icommands/i386/pc -I$(srcdir)/commands/i386/pc $(CPPFLAGS) $(CFLAGS) $(reboot_mod_CFLAGS) -M $< 	  | sed 's,reboot\.o[ :]*,reboot_mod-commands_i386_pc_reboot.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include reboot_mod-commands_i386_pc_reboot.d

CLEANFILES += cmd-reboot_mod-commands_i386_pc_reboot.lst fs-reboot_mod-commands_i386_pc_reboot.lst
COMMANDFILES += cmd-reboot_mod-commands_i386_pc_reboot.lst
FSFILES += fs-reboot_mod-commands_i386_pc_reboot.lst

cmd-reboot_mod-commands_i386_pc_reboot.lst: commands/i386/pc/reboot.c gencmdlist.sh
	set -e; 	  $(CC) -Icommands/i386/pc -I$(srcdir)/commands/i386/pc $(CPPFLAGS) $(CFLAGS) $(reboot_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh reboot > $@ || (rm -f $@; exit 1)

fs-reboot_mod-commands_i386_pc_reboot.lst: commands/i386/pc/reboot.c genfslist.sh
	set -e; 	  $(CC) -Icommands/i386/pc -I$(srcdir)/commands/i386/pc $(CPPFLAGS) $(CFLAGS) $(reboot_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh reboot > $@ || (rm -f $@; exit 1)


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
	$(CC) $(halt_mod_LDFLAGS) $(LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-halt.o: halt_mod-commands_i386_pc_halt.o
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

halt_mod-commands_i386_pc_halt.o: commands/i386/pc/halt.c
	$(CC) -Icommands/i386/pc -I$(srcdir)/commands/i386/pc $(CPPFLAGS) $(CFLAGS) $(halt_mod_CFLAGS) -c -o $@ $<

halt_mod-commands_i386_pc_halt.d: commands/i386/pc/halt.c
	set -e; 	  $(CC) -Icommands/i386/pc -I$(srcdir)/commands/i386/pc $(CPPFLAGS) $(CFLAGS) $(halt_mod_CFLAGS) -M $< 	  | sed 's,halt\.o[ :]*,halt_mod-commands_i386_pc_halt.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include halt_mod-commands_i386_pc_halt.d

CLEANFILES += cmd-halt_mod-commands_i386_pc_halt.lst fs-halt_mod-commands_i386_pc_halt.lst
COMMANDFILES += cmd-halt_mod-commands_i386_pc_halt.lst
FSFILES += fs-halt_mod-commands_i386_pc_halt.lst

cmd-halt_mod-commands_i386_pc_halt.lst: commands/i386/pc/halt.c gencmdlist.sh
	set -e; 	  $(CC) -Icommands/i386/pc -I$(srcdir)/commands/i386/pc $(CPPFLAGS) $(CFLAGS) $(halt_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh halt > $@ || (rm -f $@; exit 1)

fs-halt_mod-commands_i386_pc_halt.lst: commands/i386/pc/halt.c genfslist.sh
	set -e; 	  $(CC) -Icommands/i386/pc -I$(srcdir)/commands/i386/pc $(CPPFLAGS) $(CFLAGS) $(halt_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh halt > $@ || (rm -f $@; exit 1)


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
	$(CC) $(_multiboot_mod_LDFLAGS) $(LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-_multiboot.o: _multiboot_mod-loader_i386_pc_multiboot.o
	-rm -f $@
	$(CC) $(_multiboot_mod_LDFLAGS) $(LDFLAGS) -Wl,-r,-d -o $@ $^

mod-_multiboot.o: mod-_multiboot.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(_multiboot_mod_CFLAGS) -c -o $@ $<

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
	$(CC) -Iloader/i386/pc -I$(srcdir)/loader/i386/pc $(CPPFLAGS) $(CFLAGS) $(_multiboot_mod_CFLAGS) -c -o $@ $<

_multiboot_mod-loader_i386_pc_multiboot.d: loader/i386/pc/multiboot.c
	set -e; 	  $(CC) -Iloader/i386/pc -I$(srcdir)/loader/i386/pc $(CPPFLAGS) $(CFLAGS) $(_multiboot_mod_CFLAGS) -M $< 	  | sed 's,multiboot\.o[ :]*,_multiboot_mod-loader_i386_pc_multiboot.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include _multiboot_mod-loader_i386_pc_multiboot.d

CLEANFILES += cmd-_multiboot_mod-loader_i386_pc_multiboot.lst fs-_multiboot_mod-loader_i386_pc_multiboot.lst
COMMANDFILES += cmd-_multiboot_mod-loader_i386_pc_multiboot.lst
FSFILES += fs-_multiboot_mod-loader_i386_pc_multiboot.lst

cmd-_multiboot_mod-loader_i386_pc_multiboot.lst: loader/i386/pc/multiboot.c gencmdlist.sh
	set -e; 	  $(CC) -Iloader/i386/pc -I$(srcdir)/loader/i386/pc $(CPPFLAGS) $(CFLAGS) $(_multiboot_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh _multiboot > $@ || (rm -f $@; exit 1)

fs-_multiboot_mod-loader_i386_pc_multiboot.lst: loader/i386/pc/multiboot.c genfslist.sh
	set -e; 	  $(CC) -Iloader/i386/pc -I$(srcdir)/loader/i386/pc $(CPPFLAGS) $(CFLAGS) $(_multiboot_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh _multiboot > $@ || (rm -f $@; exit 1)


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
	$(CC) $(multiboot_mod_LDFLAGS) $(LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-multiboot.o: multiboot_mod-loader_i386_pc_multiboot_normal.o
	-rm -f $@
	$(CC) $(multiboot_mod_LDFLAGS) $(LDFLAGS) -Wl,-r,-d -o $@ $^

mod-multiboot.o: mod-multiboot.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(multiboot_mod_CFLAGS) -c -o $@ $<

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
	$(CC) -Iloader/i386/pc -I$(srcdir)/loader/i386/pc $(CPPFLAGS) $(CFLAGS) $(multiboot_mod_CFLAGS) -c -o $@ $<

multiboot_mod-loader_i386_pc_multiboot_normal.d: loader/i386/pc/multiboot_normal.c
	set -e; 	  $(CC) -Iloader/i386/pc -I$(srcdir)/loader/i386/pc $(CPPFLAGS) $(CFLAGS) $(multiboot_mod_CFLAGS) -M $< 	  | sed 's,multiboot_normal\.o[ :]*,multiboot_mod-loader_i386_pc_multiboot_normal.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include multiboot_mod-loader_i386_pc_multiboot_normal.d

CLEANFILES += cmd-multiboot_mod-loader_i386_pc_multiboot_normal.lst fs-multiboot_mod-loader_i386_pc_multiboot_normal.lst
COMMANDFILES += cmd-multiboot_mod-loader_i386_pc_multiboot_normal.lst
FSFILES += fs-multiboot_mod-loader_i386_pc_multiboot_normal.lst

cmd-multiboot_mod-loader_i386_pc_multiboot_normal.lst: loader/i386/pc/multiboot_normal.c gencmdlist.sh
	set -e; 	  $(CC) -Iloader/i386/pc -I$(srcdir)/loader/i386/pc $(CPPFLAGS) $(CFLAGS) $(multiboot_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh multiboot > $@ || (rm -f $@; exit 1)

fs-multiboot_mod-loader_i386_pc_multiboot_normal.lst: loader/i386/pc/multiboot_normal.c genfslist.sh
	set -e; 	  $(CC) -Iloader/i386/pc -I$(srcdir)/loader/i386/pc $(CPPFLAGS) $(CFLAGS) $(multiboot_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh multiboot > $@ || (rm -f $@; exit 1)


multiboot_mod_CFLAGS = $(COMMON_CFLAGS)
multiboot_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For vbe.mod.
vbe_mod_SOURCES = video/i386/pc/vbe.c video/i386/pc/vbeblit.c \
		  video/i386/pc/vbefill.c
CLEANFILES += vbe.mod mod-vbe.o mod-vbe.c pre-vbe.o vbe_mod-video_i386_pc_vbe.o vbe_mod-video_i386_pc_vbeblit.o vbe_mod-video_i386_pc_vbefill.o und-vbe.lst
ifneq ($(vbe_mod_EXPORTS),no)
CLEANFILES += def-vbe.lst
DEFSYMFILES += def-vbe.lst
endif
MOSTLYCLEANFILES += vbe_mod-video_i386_pc_vbe.d vbe_mod-video_i386_pc_vbeblit.d vbe_mod-video_i386_pc_vbefill.d
UNDSYMFILES += und-vbe.lst

vbe.mod: pre-vbe.o mod-vbe.o
	-rm -f $@
	$(CC) $(vbe_mod_LDFLAGS) $(LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-vbe.o: vbe_mod-video_i386_pc_vbe.o vbe_mod-video_i386_pc_vbeblit.o vbe_mod-video_i386_pc_vbefill.o
	-rm -f $@
	$(CC) $(vbe_mod_LDFLAGS) $(LDFLAGS) -Wl,-r,-d -o $@ $^

mod-vbe.o: mod-vbe.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(vbe_mod_CFLAGS) -c -o $@ $<

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
	$(CC) -Ivideo/i386/pc -I$(srcdir)/video/i386/pc $(CPPFLAGS) $(CFLAGS) $(vbe_mod_CFLAGS) -c -o $@ $<

vbe_mod-video_i386_pc_vbe.d: video/i386/pc/vbe.c
	set -e; 	  $(CC) -Ivideo/i386/pc -I$(srcdir)/video/i386/pc $(CPPFLAGS) $(CFLAGS) $(vbe_mod_CFLAGS) -M $< 	  | sed 's,vbe\.o[ :]*,vbe_mod-video_i386_pc_vbe.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include vbe_mod-video_i386_pc_vbe.d

CLEANFILES += cmd-vbe_mod-video_i386_pc_vbe.lst fs-vbe_mod-video_i386_pc_vbe.lst
COMMANDFILES += cmd-vbe_mod-video_i386_pc_vbe.lst
FSFILES += fs-vbe_mod-video_i386_pc_vbe.lst

cmd-vbe_mod-video_i386_pc_vbe.lst: video/i386/pc/vbe.c gencmdlist.sh
	set -e; 	  $(CC) -Ivideo/i386/pc -I$(srcdir)/video/i386/pc $(CPPFLAGS) $(CFLAGS) $(vbe_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh vbe > $@ || (rm -f $@; exit 1)

fs-vbe_mod-video_i386_pc_vbe.lst: video/i386/pc/vbe.c genfslist.sh
	set -e; 	  $(CC) -Ivideo/i386/pc -I$(srcdir)/video/i386/pc $(CPPFLAGS) $(CFLAGS) $(vbe_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh vbe > $@ || (rm -f $@; exit 1)


vbe_mod-video_i386_pc_vbeblit.o: video/i386/pc/vbeblit.c
	$(CC) -Ivideo/i386/pc -I$(srcdir)/video/i386/pc $(CPPFLAGS) $(CFLAGS) $(vbe_mod_CFLAGS) -c -o $@ $<

vbe_mod-video_i386_pc_vbeblit.d: video/i386/pc/vbeblit.c
	set -e; 	  $(CC) -Ivideo/i386/pc -I$(srcdir)/video/i386/pc $(CPPFLAGS) $(CFLAGS) $(vbe_mod_CFLAGS) -M $< 	  | sed 's,vbeblit\.o[ :]*,vbe_mod-video_i386_pc_vbeblit.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include vbe_mod-video_i386_pc_vbeblit.d

CLEANFILES += cmd-vbe_mod-video_i386_pc_vbeblit.lst fs-vbe_mod-video_i386_pc_vbeblit.lst
COMMANDFILES += cmd-vbe_mod-video_i386_pc_vbeblit.lst
FSFILES += fs-vbe_mod-video_i386_pc_vbeblit.lst

cmd-vbe_mod-video_i386_pc_vbeblit.lst: video/i386/pc/vbeblit.c gencmdlist.sh
	set -e; 	  $(CC) -Ivideo/i386/pc -I$(srcdir)/video/i386/pc $(CPPFLAGS) $(CFLAGS) $(vbe_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh vbe > $@ || (rm -f $@; exit 1)

fs-vbe_mod-video_i386_pc_vbeblit.lst: video/i386/pc/vbeblit.c genfslist.sh
	set -e; 	  $(CC) -Ivideo/i386/pc -I$(srcdir)/video/i386/pc $(CPPFLAGS) $(CFLAGS) $(vbe_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh vbe > $@ || (rm -f $@; exit 1)


vbe_mod-video_i386_pc_vbefill.o: video/i386/pc/vbefill.c
	$(CC) -Ivideo/i386/pc -I$(srcdir)/video/i386/pc $(CPPFLAGS) $(CFLAGS) $(vbe_mod_CFLAGS) -c -o $@ $<

vbe_mod-video_i386_pc_vbefill.d: video/i386/pc/vbefill.c
	set -e; 	  $(CC) -Ivideo/i386/pc -I$(srcdir)/video/i386/pc $(CPPFLAGS) $(CFLAGS) $(vbe_mod_CFLAGS) -M $< 	  | sed 's,vbefill\.o[ :]*,vbe_mod-video_i386_pc_vbefill.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include vbe_mod-video_i386_pc_vbefill.d

CLEANFILES += cmd-vbe_mod-video_i386_pc_vbefill.lst fs-vbe_mod-video_i386_pc_vbefill.lst
COMMANDFILES += cmd-vbe_mod-video_i386_pc_vbefill.lst
FSFILES += fs-vbe_mod-video_i386_pc_vbefill.lst

cmd-vbe_mod-video_i386_pc_vbefill.lst: video/i386/pc/vbefill.c gencmdlist.sh
	set -e; 	  $(CC) -Ivideo/i386/pc -I$(srcdir)/video/i386/pc $(CPPFLAGS) $(CFLAGS) $(vbe_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh vbe > $@ || (rm -f $@; exit 1)

fs-vbe_mod-video_i386_pc_vbefill.lst: video/i386/pc/vbefill.c genfslist.sh
	set -e; 	  $(CC) -Ivideo/i386/pc -I$(srcdir)/video/i386/pc $(CPPFLAGS) $(CFLAGS) $(vbe_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh vbe > $@ || (rm -f $@; exit 1)


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
	$(CC) $(vbeinfo_mod_LDFLAGS) $(LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-vbeinfo.o: vbeinfo_mod-commands_i386_pc_vbeinfo.o
	-rm -f $@
	$(CC) $(vbeinfo_mod_LDFLAGS) $(LDFLAGS) -Wl,-r,-d -o $@ $^

mod-vbeinfo.o: mod-vbeinfo.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(vbeinfo_mod_CFLAGS) -c -o $@ $<

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
	$(CC) -Icommands/i386/pc -I$(srcdir)/commands/i386/pc $(CPPFLAGS) $(CFLAGS) $(vbeinfo_mod_CFLAGS) -c -o $@ $<

vbeinfo_mod-commands_i386_pc_vbeinfo.d: commands/i386/pc/vbeinfo.c
	set -e; 	  $(CC) -Icommands/i386/pc -I$(srcdir)/commands/i386/pc $(CPPFLAGS) $(CFLAGS) $(vbeinfo_mod_CFLAGS) -M $< 	  | sed 's,vbeinfo\.o[ :]*,vbeinfo_mod-commands_i386_pc_vbeinfo.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include vbeinfo_mod-commands_i386_pc_vbeinfo.d

CLEANFILES += cmd-vbeinfo_mod-commands_i386_pc_vbeinfo.lst fs-vbeinfo_mod-commands_i386_pc_vbeinfo.lst
COMMANDFILES += cmd-vbeinfo_mod-commands_i386_pc_vbeinfo.lst
FSFILES += fs-vbeinfo_mod-commands_i386_pc_vbeinfo.lst

cmd-vbeinfo_mod-commands_i386_pc_vbeinfo.lst: commands/i386/pc/vbeinfo.c gencmdlist.sh
	set -e; 	  $(CC) -Icommands/i386/pc -I$(srcdir)/commands/i386/pc $(CPPFLAGS) $(CFLAGS) $(vbeinfo_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh vbeinfo > $@ || (rm -f $@; exit 1)

fs-vbeinfo_mod-commands_i386_pc_vbeinfo.lst: commands/i386/pc/vbeinfo.c genfslist.sh
	set -e; 	  $(CC) -Icommands/i386/pc -I$(srcdir)/commands/i386/pc $(CPPFLAGS) $(CFLAGS) $(vbeinfo_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh vbeinfo > $@ || (rm -f $@; exit 1)


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
	$(CC) $(vbetest_mod_LDFLAGS) $(LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-vbetest.o: vbetest_mod-commands_i386_pc_vbetest.o
	-rm -f $@
	$(CC) $(vbetest_mod_LDFLAGS) $(LDFLAGS) -Wl,-r,-d -o $@ $^

mod-vbetest.o: mod-vbetest.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(vbetest_mod_CFLAGS) -c -o $@ $<

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
	$(CC) -Icommands/i386/pc -I$(srcdir)/commands/i386/pc $(CPPFLAGS) $(CFLAGS) $(vbetest_mod_CFLAGS) -c -o $@ $<

vbetest_mod-commands_i386_pc_vbetest.d: commands/i386/pc/vbetest.c
	set -e; 	  $(CC) -Icommands/i386/pc -I$(srcdir)/commands/i386/pc $(CPPFLAGS) $(CFLAGS) $(vbetest_mod_CFLAGS) -M $< 	  | sed 's,vbetest\.o[ :]*,vbetest_mod-commands_i386_pc_vbetest.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include vbetest_mod-commands_i386_pc_vbetest.d

CLEANFILES += cmd-vbetest_mod-commands_i386_pc_vbetest.lst fs-vbetest_mod-commands_i386_pc_vbetest.lst
COMMANDFILES += cmd-vbetest_mod-commands_i386_pc_vbetest.lst
FSFILES += fs-vbetest_mod-commands_i386_pc_vbetest.lst

cmd-vbetest_mod-commands_i386_pc_vbetest.lst: commands/i386/pc/vbetest.c gencmdlist.sh
	set -e; 	  $(CC) -Icommands/i386/pc -I$(srcdir)/commands/i386/pc $(CPPFLAGS) $(CFLAGS) $(vbetest_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh vbetest > $@ || (rm -f $@; exit 1)

fs-vbetest_mod-commands_i386_pc_vbetest.lst: commands/i386/pc/vbetest.c genfslist.sh
	set -e; 	  $(CC) -Icommands/i386/pc -I$(srcdir)/commands/i386/pc $(CPPFLAGS) $(CFLAGS) $(vbetest_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh vbetest > $@ || (rm -f $@; exit 1)


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
	$(CC) $(play_mod_LDFLAGS) $(LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-play.o: play_mod-commands_i386_pc_play.o
	-rm -f $@
	$(CC) $(play_mod_LDFLAGS) $(LDFLAGS) -Wl,-r,-d -o $@ $^

mod-play.o: mod-play.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(play_mod_CFLAGS) -c -o $@ $<

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
	$(CC) -Icommands/i386/pc -I$(srcdir)/commands/i386/pc $(CPPFLAGS) $(CFLAGS) $(play_mod_CFLAGS) -c -o $@ $<

play_mod-commands_i386_pc_play.d: commands/i386/pc/play.c
	set -e; 	  $(CC) -Icommands/i386/pc -I$(srcdir)/commands/i386/pc $(CPPFLAGS) $(CFLAGS) $(play_mod_CFLAGS) -M $< 	  | sed 's,play\.o[ :]*,play_mod-commands_i386_pc_play.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include play_mod-commands_i386_pc_play.d

CLEANFILES += cmd-play_mod-commands_i386_pc_play.lst fs-play_mod-commands_i386_pc_play.lst
COMMANDFILES += cmd-play_mod-commands_i386_pc_play.lst
FSFILES += fs-play_mod-commands_i386_pc_play.lst

cmd-play_mod-commands_i386_pc_play.lst: commands/i386/pc/play.c gencmdlist.sh
	set -e; 	  $(CC) -Icommands/i386/pc -I$(srcdir)/commands/i386/pc $(CPPFLAGS) $(CFLAGS) $(play_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh play > $@ || (rm -f $@; exit 1)

fs-play_mod-commands_i386_pc_play.lst: commands/i386/pc/play.c genfslist.sh
	set -e; 	  $(CC) -Icommands/i386/pc -I$(srcdir)/commands/i386/pc $(CPPFLAGS) $(CFLAGS) $(play_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh play > $@ || (rm -f $@; exit 1)


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
	$(CC) $(video_mod_LDFLAGS) $(LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-video.o: video_mod-video_video.o
	-rm -f $@
	$(CC) $(video_mod_LDFLAGS) $(LDFLAGS) -Wl,-r,-d -o $@ $^

mod-video.o: mod-video.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(video_mod_CFLAGS) -c -o $@ $<

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
	$(CC) -Ivideo -I$(srcdir)/video $(CPPFLAGS) $(CFLAGS) $(video_mod_CFLAGS) -c -o $@ $<

video_mod-video_video.d: video/video.c
	set -e; 	  $(CC) -Ivideo -I$(srcdir)/video $(CPPFLAGS) $(CFLAGS) $(video_mod_CFLAGS) -M $< 	  | sed 's,video\.o[ :]*,video_mod-video_video.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include video_mod-video_video.d

CLEANFILES += cmd-video_mod-video_video.lst fs-video_mod-video_video.lst
COMMANDFILES += cmd-video_mod-video_video.lst
FSFILES += fs-video_mod-video_video.lst

cmd-video_mod-video_video.lst: video/video.c gencmdlist.sh
	set -e; 	  $(CC) -Ivideo -I$(srcdir)/video $(CPPFLAGS) $(CFLAGS) $(video_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh video > $@ || (rm -f $@; exit 1)

fs-video_mod-video_video.lst: video/video.c genfslist.sh
	set -e; 	  $(CC) -Ivideo -I$(srcdir)/video $(CPPFLAGS) $(CFLAGS) $(video_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh video > $@ || (rm -f $@; exit 1)


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
	$(CC) $(gfxterm_mod_LDFLAGS) $(LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-gfxterm.o: gfxterm_mod-term_gfxterm.o
	-rm -f $@
	$(CC) $(gfxterm_mod_LDFLAGS) $(LDFLAGS) -Wl,-r,-d -o $@ $^

mod-gfxterm.o: mod-gfxterm.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(gfxterm_mod_CFLAGS) -c -o $@ $<

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
	$(CC) -Iterm -I$(srcdir)/term $(CPPFLAGS) $(CFLAGS) $(gfxterm_mod_CFLAGS) -c -o $@ $<

gfxterm_mod-term_gfxterm.d: term/gfxterm.c
	set -e; 	  $(CC) -Iterm -I$(srcdir)/term $(CPPFLAGS) $(CFLAGS) $(gfxterm_mod_CFLAGS) -M $< 	  | sed 's,gfxterm\.o[ :]*,gfxterm_mod-term_gfxterm.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include gfxterm_mod-term_gfxterm.d

CLEANFILES += cmd-gfxterm_mod-term_gfxterm.lst fs-gfxterm_mod-term_gfxterm.lst
COMMANDFILES += cmd-gfxterm_mod-term_gfxterm.lst
FSFILES += fs-gfxterm_mod-term_gfxterm.lst

cmd-gfxterm_mod-term_gfxterm.lst: term/gfxterm.c gencmdlist.sh
	set -e; 	  $(CC) -Iterm -I$(srcdir)/term $(CPPFLAGS) $(CFLAGS) $(gfxterm_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh gfxterm > $@ || (rm -f $@; exit 1)

fs-gfxterm_mod-term_gfxterm.lst: term/gfxterm.c genfslist.sh
	set -e; 	  $(CC) -Iterm -I$(srcdir)/term $(CPPFLAGS) $(CFLAGS) $(gfxterm_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh gfxterm > $@ || (rm -f $@; exit 1)


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
	$(CC) $(videotest_mod_LDFLAGS) $(LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-videotest.o: videotest_mod-commands_videotest.o
	-rm -f $@
	$(CC) $(videotest_mod_LDFLAGS) $(LDFLAGS) -Wl,-r,-d -o $@ $^

mod-videotest.o: mod-videotest.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(videotest_mod_CFLAGS) -c -o $@ $<

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
	$(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(videotest_mod_CFLAGS) -c -o $@ $<

videotest_mod-commands_videotest.d: commands/videotest.c
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(videotest_mod_CFLAGS) -M $< 	  | sed 's,videotest\.o[ :]*,videotest_mod-commands_videotest.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include videotest_mod-commands_videotest.d

CLEANFILES += cmd-videotest_mod-commands_videotest.lst fs-videotest_mod-commands_videotest.lst
COMMANDFILES += cmd-videotest_mod-commands_videotest.lst
FSFILES += fs-videotest_mod-commands_videotest.lst

cmd-videotest_mod-commands_videotest.lst: commands/videotest.c gencmdlist.sh
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(videotest_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh videotest > $@ || (rm -f $@; exit 1)

fs-videotest_mod-commands_videotest.lst: commands/videotest.c genfslist.sh
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(videotest_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh videotest > $@ || (rm -f $@; exit 1)


videotest_mod_CFLAGS = $(COMMON_CFLAGS)
videotest_mod_LDFLAGS = $(COMMON_LDFLAGS)

include $(srcdir)/conf/common.mk
