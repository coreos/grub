#
#  GRUB stage1 makefile
#

CFLAGS= $(SHARED_FLAGS)

all: ../bin/stage1

stage1.o:	stage1.S Makefile ../Makefile
	$(CC) $(CFLAGS) -c -o stage1.o stage1.S

stage1.exec:	stage1.o Makefile ../Makefile
	$(LD) -N -Ttext 7C00 -o stage1.exec stage1.o

../bin/stage1:	stage1.exec Makefile ../Makefile
	$(OBJCOPY) -O binary stage1.exec ../bin/stage1

clean:
	rm -f stage1.o stage1.exec

