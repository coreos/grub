sbin_PROGRAMS = grub

AM_CFLAGS = @GRUB_CFLAGS@ -DGRUB_UTIL=1 -fwritable-strings \
	-I$(top_srcdir)/stage2

grub_SOURCES = main.c asmstub.c
grub_LDADD = ../stage2/libgrub.a @GRUB_LIBS@
