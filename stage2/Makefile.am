# For test target.
TESTS = size_test
noinst_SCRIPTS = $(TESTS)

# For dist target.
noinst_HEADERS = apic.h defs.h dir.h disk_inode.h disk_inode_ffs.h \
        fat.h filesys.h freebsd.h fs.h i386-elf.h imgact_aout.h \
        mb_header.h mb_info.h pc_slice.h shared.h smp-imps.h
EXTRA_DIST = $(noinst_SCRIPTS)

# For <stage1.h>.
INCLUDES = -I$(top_srcdir)/stage1

# The library for /sbin/grub.
noinst_LIBRARIES = libgrub.a
libgrub_a_SOURCES = boot.c common.c char_io.c cmdline.c disk_io.c \
	gunzip.c fsys_ffs.c fsys_ext2fs.c fsys_fat.c stage2.c
libgrub_a_CFLAGS = @GRUB_CFLAGS@ -DGRUB_UTIL=1 -DFSYS_EXT2FS=1 \
	-DFSYS_FAT=1 -DFSYS_FFS=1 -fwritable-strings

# Stage 2 and Stage 1.5's.
pkgdatadir = $(datadir)/$(PACKAGE)/$(host_cpu)-$(host_vendor)
nodist_pkgdata_DATA = stage2 e2fs_stage1_5 fat_stage1_5 ffs_stage1_5
CLEANFILES = $(pkgdata_DATA)
noinst_PROGRAMS = stage2.exec e2fs_stage1_5.exec fat_stage1_5.exec \
	ffs_stage1_5.exec
MOSTLYCLEANFILES = $(noinst_PROGRAMS)

STAGE2_LINK = -nostdlib -Wl,-N -Wl,-Ttext -Wl,8000
STAGE2_COMPILE = @STAGE2_CFLAGS@ -fno-builtin -nostdinc

STAGE1_5_LINK = -nostdlib -Wl,-N -Wl,-Ttext -Wl,2000
STAGE1_5_COMPILE = $(STAGE2_COMPILE) -DNO_DECOMPRESSION=1 -DSTAGE1_5=1 \
	-DCONFIG_FILE_ASM='.string "/boot/grub/stage2"'

# asm.S absolutely needs to come first!
# For stage2 target.
stage2_exec_SOURCES = asm.S boot.c common.c char_io.c cmdline.c \
	disk_io.c gunzip.c stage2.c bios.c smp-imps.c fsys_ext2fs.c \
	fsys_fat.c fsys_ffs.c
stage2_exec_CFLAGS = $(STAGE2_COMPILE) @FSYS_CFLAGS@
stage2_exec_LDFLAGS = $(STAGE2_LINK)

# For e2fs_stage1_5 target.
e2fs_stage1_5_exec_SOURCES = asm.S common.c char_io.c disk_io.c \
	stage1_5.c fsys_ext2fs.c bios.c
e2fs_stage1_5_exec_CFLAGS = $(STAGE1_5_COMPILE) -DFSYS_EXT2FS=1 \
	-DNO_BLOCK_FILES=1
e2fs_stage1_5_exec_LDFLAGS = $(STAGE1_5_LINK)

# For fat_stage1_5 target.
fat_stage1_5_exec_SOURCES = asm.S common.c char_io.c disk_io.c \
	stage1_5.c fsys_fat.c bios.c
fat_stage1_5_exec_CFLAGS = $(STAGE1_5_COMPILE) -DFSYS_FAT=1
fat_stage1_5_exec_LDFLAGS = $(STAGE1_5_LINK)

# For ffs_stage1_5 target.
ffs_stage1_5_exec_SOURCES = asm.S common.c char_io.c disk_io.c \
	stage1_5.c fsys_ffs.c bios.c

ffs_stage1_5_exec_CFLAGS = $(STAGE1_5_COMPILE) -DFSYS_FFS=1 \
	-DNO_BLOCK_FILES=1
ffs_stage1_5_exec_LDFLAGS = $(STAGE1_5_LINK)

# General rule for making a raw binary.
%: %.exec
	$(OBJCOPY) -O binary $< $@
