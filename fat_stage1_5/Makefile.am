IMPORTANT_SIZE_LIMIT = 81920

pkgdatadir = $(datadir)/$(PACKAGE)/$(host_cpu)-$(host_vendor)
pkgdata_DATA = fat_stage1_5
CLEANFILES = $(pkgdata_DATA) $(pkgdata_DATA)T
noinst_PROGRAMS = fat_stage1_5.exec

# We can't use builtins or standard includes.
COMPILE = $(CC) -fno-builtin -nostdinc -DFSYS_FAT=1 -DNO_DECOMPRESSION=1 \
	-DSTAGE1_5=1 -DCONFIG_FILE_ASM='.string "/boot/grub/stage2"' \
	$(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS)
INCLUDES = -I$(top_srcdir)/shared_src

# asm.o absolutely needs to come first!
fat_stage1_5_exec_LDADD = asm.o common.o char_io.o disk_io.o \
	stage1_5.o fsys_fat.o bios.o

# FIXME: Automake hackery.
DEP_FILES = .deps/dummy.P $(wildcard .deps/*.P)
fat_stage1_5_exec_SOURCES = dummy.c # needed to drag in C Makefile variables
DISTFILES = $(DIST_COMMON)

fat_stage1_5.exec: $(fat_stage1_5_exec_LDADD)
	$(LD) -N -Ttext 2000 -o $@ $(fat_stage1_5_exec_LDADD)

fat_stage1_5: fat_stage1_5.exec
	rm -f $@T $@
	$(OBJCOPY) -O binary fat_stage1_5.exec $@T
	@set dummy `ls -l $@T`; size=$$6; \
	if test $$size -le $(IMPORTANT_SIZE_LIMIT); then \
	  :; \
	else \
	  echo "WARNING: stage 1.5 is too big for boot block area ($$size > $(IMPORTANT_SIZE_LIMIT))."; \
	fi
	mv -f $@T $@

@SHARED_SRC_RULES@
