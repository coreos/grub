#
#  GRUB fat_stage1_5 makefile
#

COMPILE = $(CC) $(CFLAGS) $(CPPFLAGS) $(SHARED_FLAGS) \
	-DFSYS_FAT -DNO_DECOMPRESSION -DNO_FANCY_STUFF \
	-DCONFIG_FILE_ASM=".string \"/boot/grub/stage2\"" \
	-I../shared_src

# FIXME: Is this really true?
# There are problems inheriting flags that work fine with stage1 and stage2
#   so...  don't use them at all!
#CFLAGS= $(SHARED_FLAGS)

# "asm.o" absolutely has to come first in the link line!
SHARED_OBJS=	asm.o char_io.o common.o disk_io.o stage1_5.o fsys_fat.o

all:	../bin/fat_stage1_5

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

fsys_fat.o:	../shared_src/fsys_fat.c ../shared_src/filesys.h \
		../shared_src/pc_slice.h ../shared_src/shared.h
	$(COMPILE) -c ../shared_src/fsys_fat.c

# "asm.o" absolutely has to come first in the link line!
fat_stage1_5.exec: $(SHARED_OBJS)
	$(LD) -N -Ttext 2000 -o $@ $(SHARED_OBJS)

../bin/fat_stage1_5: fat_stage1_5.exec
	$(OBJCOPY) -O binary -S $< $@

clean:
	rm -f *.o *.exec
