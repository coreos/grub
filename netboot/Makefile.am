
# For <stage1.h>.
INCLUDES = -I$(top_srcdir)/stage1

if NETBOOT_SUPPORT
LIBDRIVERS = libdrivers.a
else
LIBDRIVERS =
endif

noinst_LIBRARIES = $(LIBDRIVERS)

DRIVERS = 3c509.c 3c509.h 3c59x.c cs89x0.c cs89x0.h eepro100.c \
	lance.c

libdrivers_a_SOURCES = byteorder.h config.c netboot_config.h \
	fsys_tftp.c if.h io.h ip.c ip.h netboot.h netdevice.h nic.h \
	pci.c pci.h $(DRIVERS)
libdrivers_a_CFLAGS = $(STAGE2_CFLAGS) -fno-builtin -nostdinc \
	-DFSYS_TFTP $(NET_CFLAGS) $(NET_EXTRAFLAGS)

libdrivers_a_LIBADD = nepci.o ne.o wd.o t503.o

$(libdrivers_a_LIBADD): ns8390.c ns8390.h
$(libdrivers_a_LIBADD): %.o: ns8390.c
	$(COMPILE) $($(basename $@)_o_CFLAGS) -o $@ -c $<

nepci_o_CFLAGS = $(filter-out -DINCLUDE_T503=1 -DINCLUDE_NE=1 \
	-DINCLUDE_WD=1, $(libdrivers_a_CFLAGS))
ne_o_CFLAGS = $(filter-out -DINCLUDE_T503=1 -DINCLUDE_NEPCI=1 \
	-DINCLUDE_WD=1, $(libdrivers_a_CFLAGS))
wd_o_CFLAGS = $(filter-out -DINCLUDE_T503=1 -DINCLUDE_NE=1 \
	-DINCLUDE_NEPCI=1, $(libdrivers_a_CFLAGS))
t503_o_CFLAGS = $(filter-out -DINCLUDE_NE=1 -DINCLUDE_NEPCI=1 \
	-DINCLUDE_WD=1, $(libdrivers_a_CFLAGS))
