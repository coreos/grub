#
#  GRUB stage2 makefile
#

CFLAGS= $(SHARED_FLAGS) -I../shared_src

# "asm.o" absolutely must come first!!!
SHARED_OBJS=	asm.o common.o char_io.o boot.o cmdline.o gunzip.o \
	disk_io.o stage2.o fsys_ffs.o fsys_ext2fs.o fsys_fat.o

all:	../bin/stage2

asm.o:	../shared_src/asm.S ../shared_src/shared.h Makefile ../Makefile
	$(CC) $(CFLAGS) -c ../shared_src/asm.S

boot.o:	../shared_src/boot.c ../shared_src/shared.h Makefile ../Makefile
	$(CC) $(CFLAGS) -c ../shared_src/boot.c

char_io.o:	../shared_src/char_io.c ../shared_src/shared.h Makefile \
		../Makefile
	$(CC) $(CFLAGS) -c ../shared_src/char_io.c

cmdline.o:	../shared_src/cmdline.c ../shared_src/shared.h Makefile \
		../Makefile
	$(CC) $(CFLAGS) -c ../shared_src/cmdline.c

common.o:	../shared_src/common.c ../shared_src/shared.h Makefile \
		../Makefile
	$(CC) $(CFLAGS) -c ../shared_src/common.c

gunzip.o:	../shared_src/gunzip.c ../shared_src/shared.h Makefile \
		../Makefile
	$(CC) $(CFLAGS) -c ../shared_src/gunzip.c

disk_io.o:	../shared_src/disk_io.c ../shared_src/filesys.h \
		../shared_src/pc_slice.h ../shared_src/shared.h Makefile \
		../Makefile
	$(CC) $(CFLAGS) -c ../shared_src/disk_io.c

stage2.o:	../shared_src/stage2.c ../shared_src/shared.h Makefile \
		../Makefile
	$(CC) $(CFLAGS) -c ../shared_src/stage2.c

fsys_ffs.o:	../shared_src/fsys_ffs.c ../shared_src/filesys.h \
		../shared_src/pc_slice.h ../shared_src/shared.h \
		../shared_src/defs.h ../shared_src/disk_inode.h \
		../shared_src/disk_inode_ffs.h ../shared_src/dir.h \
		../shared_src/fs.h Makefile ../Makefile
	$(CC) $(CFLAGS) -c ../shared_src/fsys_ffs.c

fsys_ext2fs.o:	../shared_src/fsys_ext2fs.c ../shared_src/filesys.h \
		../shared_src/pc_slice.h ../shared_src/shared.h Makefile \
		../Makefile
	$(CC) $(CFLAGS) -c ../shared_src/fsys_ext2fs.c

fsys_fat.o:	../shared_src/fsys_fat.c ../shared_src/filesys.h \
		../shared_src/pc_slice.h ../shared_src/shared.h \
		../shared_src/fat.h Makefile ../Makefile
	$(CC) $(CFLAGS) -c ../shared_src/fsys_fat.c

# "asm.o" absolutely has to come first in the link line!
stage2.exec:	$(SHARED_OBJS) Makefile ../Makefile
	$(LD) -N -Ttext 8000 -o stage2.exec $(SHARED_OBJS)

../bin/stage2:	stage2.exec Makefile ../Makefile
	$(OBJCOPY) -O binary stage2.exec ../bin/stage2

clean:
	rm -f $(SHARED_OBJS) stage2.exec

