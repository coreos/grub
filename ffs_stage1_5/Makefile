#
#  GRUB ffs_stage1_5 makefile
#

IMPORTANT_SIZE_LIMIT = 7168
COMPILE= $(CC) $(CFLAGS) $(CPPFLAGS) $(SHARED_FLAGS) -DFSYS_FFS \
	-DNO_DECOMPRESSION -DNO_FANCY_STUFF -DNO_BLOCK_FILES \
	-DCONFIG_FILE_ASM=".string \"/boot/grub/stage2\"" \
	-I../shared_src

# "asm.o" absolutely has to come first in the link line!
SHARED_OBJS=	asm.o char_io.o common.o disk_io.o stage1_5.o fsys_ffs.o

all:	../bin/ffs_stage1_5

asm.o:	../shared_src/asm.S ../shared_src/shared.h
	$(COMPILE) -c ../shared_src/asm.S

char_io.o:	../shared_src/char_io.c ../shared_src/shared.h
	$(COMPILE) -c ../shared_src/char_io.c

common.o:	../shared_src/common.c ../shared_src/shared.h
	$(COMPILE) -c ../shared_src/common.c

disk_io.o:	../shared_src/disk_io.c ../shared_src/filesys.h \
		../shared_src/pc_slice.h ../shared_src/shared.h
	$(COMPILE) -c ../shared_src/disk_io.c

stage1_5.o:	../shared_src/stage1_5.c ../shared_src/shared.h
	$(COMPILE) -c ../shared_src/stage1_5.c

fsys_ffs.o:	../shared_src/fsys_ffs.c ../shared_src/filesys.h \
		../shared_src/pc_slice.h ../shared_src/shared.h \
		../shared_src/defs.h ../shared_src/disk_inode.h \
		../shared_src/disk_inode_ffs.h ../shared_src/dir.h \
		../shared_src/fs.h
	$(COMPILE) -c ../shared_src/fsys_ffs.c

# "asm.o" absolutely has to come first in the link line!
ffs_stage1_5.exec:	$(SHARED_OBJS)
	$(LD) -N -Ttext 2000 -o $@ $(SHARED_OBJS)

ffs_stage1_5: ffs_stage1_5.exec
	$(OBJCOPY) -O binary $< $@

../bin/ffs_stage1_5: ffs_stage1_5
	@set dummy `wc $<`; size=$$4; \
	if test $$size -le $(IMPORTANT_SIZE_LIMIT); then \
	  cp $< $@; \
	else \
	  echo "WARNING: stage 1.5 is too big for boot block area ($$size > $(IMPORTANT_SIZE_LIMIT))."; \
	fi

clean:
	rm -f *.o *.exec ffs_stage1_5
