#
#  GRUB program makefile
#

CFLAGS = -g -O2
COMPILE = $(CC) $(CPPFLAGS) $(CFLAGS) $(SHARED_FLAGS) -I../shared_src
LINK = $(CC) $(CFLAGS) $(LDFLAGS)

SHARED_OBJS=	common.o char_io.o boot.o cmdline.o gunzip.o \
	disk_io.o stage2.o fsys_ffs.o fsys_ext2fs.o fsys_fat.o

all:	grub

boot.o:	../shared_src/boot.c ../shared_src/shared.h
	$(COMPILE) -c ../shared_src/boot.c

char_io.o:	../shared_src/char_io.c ../shared_src/shared.h
	$(COMPILE) -c ../shared_src/char_io.c

cmdline.o:	../shared_src/cmdline.c ../shared_src/shared.h
	$(COMPILE) -c ../shared_src/cmdline.c

common.o:	../shared_src/common.c ../shared_src/shared.h
	$(COMPILE) -c ../shared_src/common.c

gunzip.o:	../shared_src/gunzip.c ../shared_src/shared.h
	$(COMPILE) -c ../shared_src/gunzip.c

disk_io.o:	../shared_src/disk_io.c ../shared_src/filesys.h \
		../shared_src/pc_slice.h ../shared_src/shared.h
	$(COMPILE) -c ../shared_src/disk_io.c

stage2.o:	../shared_src/stage2.c ../shared_src/shared.h
	$(COMPILE) -c ../shared_src/stage2.c

fsys_ffs.o:	../shared_src/fsys_ffs.c ../shared_src/filesys.h \
		../shared_src/pc_slice.h ../shared_src/shared.h \
		../shared_src/defs.h ../shared_src/disk_inode.h \
		../shared_src/disk_inode_ffs.h ../shared_src/dir.h \
		../shared_src/fs.h
	$(COMPILE) -c ../shared_src/fsys_ffs.c

fsys_ext2fs.o:	../shared_src/fsys_ext2fs.c ../shared_src/filesys.h \
		../shared_src/pc_slice.h ../shared_src/shared.h
	$(COMPILE) -c ../shared_src/fsys_ext2fs.c

fsys_fat.o:	../shared_src/fsys_fat.c ../shared_src/filesys.h \
		../shared_src/pc_slice.h ../shared_src/shared.h \
		../shared_src/fat.h
	$(COMPILE) -c ../shared_src/fsys_fat.c

main.o: main.c ../shared_src/shared.h
	$(COMPILE) -c main.c

asmstub.o: asmstub.c ../shared_src/shared.h
	$(COMPILE) -c asmstub.c

grub:	main.o asmstub.o $(SHARED_OBJS)
	$(LINK) -o $@ main.o asmstub.o $(SHARED_OBJS)

clean:
	rm -f *.o grub
