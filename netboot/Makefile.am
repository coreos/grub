# For <shared.h> and <stage1.h>.
INCLUDES = -I$(top_srcdir)/stage2 -I$(top_srcdir)/stage1

# Don't build the netboot support by default.
if NETBOOT_SUPPORT
LIBDRIVERS = libdrivers.a
else
LIBDRIVERS =
endif

noinst_LIBRARIES = $(LIBDRIVERS)

libdrivers_a_SOURCES = byteorder.h config.c etherboot.h \
	fsys_tftp.c if.h linux-asm-io.h linux-asm-string.h \
	main.c misc.c netboot_config.h netdevice.h nic.h osdep.h \
	pci.c pci.h
EXTRA_libdrivers_a_SOURCES = 3c509.c 3c509.h 3c90x.c cs89x0.c \
	cs89x0.h eepro100.c epic100.c epic100.h i82586.c lance.c \
	ns8390.c ns8390.h ntulip.c rtl8139.c sk_g16.c sk_g16.h \
	smc9000.c smc9000.h tiara.c tulip.c tulip.h via-rhine.c
libdrivers_a_CFLAGS = $(STAGE2_CFLAGS) -fno-builtin -nostdinc \
	-DFSYS_TFTP $(NET_CFLAGS) $(NET_EXTRAFLAGS)
# Filled by configure.
libdrivers_a_LIBADD = @NETBOOT_DRIVERS@
libdrivers_a_DEPENDENCIES = $(libdrivers_a_LIBADD)

EXTRA_DIST = README.netboot 3c90x.txt ntulip.txt

# These below are several special rules for the device drivers.
# We cannot use a simple rule for them...

# What objects are derived from a driver?
3c509_drivers = 3c509.o 3c529.o
#3c59x_drivers = 3c59x.o
3c90x_drivers = 3c90x.o
cs89x0_drivers = cs89x0.o
eepro100_drivers = eepro100.o
epic100_drivers = epic100.o
i82586_drivers = 3c507.o exos205.o ni5210.o
lance_drivers = lancepci.o ne2100.o ni6510.o
ns8390_drivers = 3c503.o ne.o nepci.o wd.o
ntulip_drivers = ntulip.o
rtl8139_drivers = rtl8139.o
sk_g16_drivers = sk_g16.o
smc9000_drivers = smc9000.o
tiara_drivers = tiara.o
tulip_drivers = tulip.o
via_rhine_drivers = via_rhine.o

# Is it really necessary to specify dependecies explicitly?
$(ns8390_drivers): ns8390.c ns8390.h
$(ns8390_drivers): %.o: ns8390.c
	$(COMPILE) $(STAGE2_CFLAGS) -fno-builtin -nostdinc \
	  $(NET_EXTRAFLAGS) $($(basename $@)_o_CFLAGS) -o $@ -c $<

$(i82586_drivers): i82586.c
$(i82586_drivers): %.o: i82586.c
	$(COMPILE) $(STAGE2_CFLAGS) -fno-builtin -nostdinc \
	  $(NET_EXTRAFLAGS) $($(basename $@)_o_CFLAGS) -o $@ -c $<

$(3c509_drivers): 3c509.c 3c509.h
$(3c509_drivers): %.o: 3c509.c
	$(COMPILE) $(STAGE2_CFLAGS) -fno-builtin -nostdinc \
	  $(NET_EXTRAFLAGS) $($(basename $@)_o_CFLAGS) -o $@ -c $<

#$(3c59x_drivers): 3c59x.c 3c509.h
#$(3c59x_drivers): %.o: 3c59x.c
#	$(COMPILE) $(STAGE2_CFLAGS) -fno-builtin -nostdinc \
#	  $(NET_EXTRAFLAGS) $($(basename $@)_o_CFLAGS) -o $@ -c $<

$(3c90x_drivers): 3c90x.c
$(3c90x_drivers): %.o: 3c90x.c
	$(COMPILE) $(STAGE2_CFLAGS) -fno-builtin -nostdinc \
	  $(NET_EXTRAFLAGS) $($(basename $@)_o_CFLAGS) -o $@ -c $<

$(cs89x0_drivers): cs89x0.c cs89x0.h
$(cs89x0_drivers): %.o: cs89x0.c
	$(COMPILE) $(STAGE2_CFLAGS) -fno-builtin -nostdinc \
	  $(NET_EXTRAFLAGS) $($(basename $@)_o_CFLAGS) -o $@ -c $<

$(eepro100_drivers): eepro100.c
$(eepro100_drivers): %.o: eepro100.c
	$(COMPILE) $(STAGE2_CFLAGS) -fno-builtin -nostdinc \
	  $(NET_EXTRAFLAGS) $($(basename $@)_o_CFLAGS) -o $@ -c $<

$(epic100_drivers): epic100.c epic100.h
$(epic100_drivers): %.o: epic100.c
	$(COMPILE) $(STAGE2_CFLAGS) -fno-builtin -nostdinc \
	  $(NET_EXTRAFLAGS) $($(basename $@)_o_CFLAGS) -o $@ -c $<

$(lance_drivers): lance.c
$(lance_drivers): %.o: lance.c
	$(COMPILE) $(STAGE2_CFLAGS) -fno-builtin -nostdinc \
	  $(NET_EXTRAFLAGS) $($(basename $@)_o_CFLAGS) -o $@ -c $<

$(ntulip_drivers): ntulip.c
$(ntulip_drivers): %.o: ntulip.c
	$(COMPILE) $(STAGE2_CFLAGS) -fno-builtin -nostdinc \
	  $(NET_EXTRAFLAGS) $($(basename $@)_o_CFLAGS) -o $@ -c $<

$(rtl8139_drivers): rtl8139.c
$(rtl8139_drivers): %.o: rtl8139.c
	$(COMPILE) $(STAGE2_CFLAGS) -fno-builtin -nostdinc \
	  $(NET_EXTRAFLAGS) $($(basename $@)_o_CFLAGS) -o $@ -c $<

$(sk_g16_drivers): sk_g16.c sk_g16.h
$(sk_g16_drivers): %.o: sk_g16.c
	$(COMPILE) $(STAGE2_CFLAGS) -fno-builtin -nostdinc \
	  $(NET_EXTRAFLAGS) $($(basename $@)_o_CFLAGS) -o $@ -c $<

$(smc9000_drivers): smc9000.c smc9000.h
$(smc9000_drivers): %.o: smc9000.c
	$(COMPILE) $(STAGE2_CFLAGS) -fno-builtin -nostdinc \
	  $(NET_EXTRAFLAGS) $($(basename $@)_o_CFLAGS) -o $@ -c $<

$(tiara_drivers): tiara.c
$(tiara_drivers): %.o: tiara.c
	$(COMPILE) $(STAGE2_CFLAGS) -fno-builtin -nostdinc \
	  $(NET_EXTRAFLAGS) $($(basename $@)_o_CFLAGS) -o $@ -c $<

$(tulip_drivers): tulip.c tulip.h
$(tulip_drivers): %.o: tulip.c
	$(COMPILE) $(STAGE2_CFLAGS) -fno-builtin -nostdinc \
	  $(NET_EXTRAFLAGS) $($(basename $@)_o_CFLAGS) -o $@ -c $<

$(via_rhine_drivers): via-rhine.c
$(via_rhine_drivers): %.o: via-rhine.c
	$(COMPILE) $(STAGE2_CFLAGS) -fno-builtin -nostdinc \
	  $(NET_EXTRAFLAGS) $($(basename $@)_o_CFLAGS) -o $@ -c $<


# Per-object flags.
3c509_o_CFLAGS = -DINCLUDE_3C509=1
3c529_o_CFLAGS = -DINCLUDE_3C529=1
#3c59x_o_CFLAGS = -DINCLUDE_3C59X=1
3c90x_o_CFLAGS = -DINCLUDE_3C90X=1
cs89x0_o_CFLAGS = -DINCLUDE_CS89X0=1
eepro100_o_CFLAGS = -DINCLUDE_EEPRO100=1
epic100_o_CFLAGS = -DINCLUDE_EPIC100=1
3c507_o_CFLAGS = -DINCLUDE_3C507=1
exos205_o_CFLAGS = -DINCLUDE_EXOS205=1
ni5210_o_CFLAGS = -DINCLUDE_NI5210=1
lancepci_o_CFLAGS = -DINCLUDE_LANCEPCI=1
ne2100_o_CFLAGS = -DINCLUDE_NE2100=1
ni6510_o_CFLAGS = -DINCLUDE_NI6510=1
3c503_o_CFLAGS = -DINCLUDE_3C503=1
ne_o_CFLAGS = -DINCLUDE_NE=1
nepci_o_CFLAGS = -DINCLUDE_NEPCI=1
wd_o_CFLAGS = -DINCLUDE_WD=1
ntulip_o_CFLAGS = -DINCLUDE_NTULIP=1
rtl8139_o_CFLAGS = -DINCLUDE_RTL8139=1
sk_g16_o_CFLAGS = -DINCLUDE_SK_G16=1
smc9000_o_CFLAGS = -DINCLUDE_SMC9000=1
tiara_o_CFLAGS = -DINCLUDE_TIARA=1
tulip_o_CFLAGS = -DINCLUDE_TULIP=1
via_rhine_o_CFLAGS = -DINCLUDE_VIA_RHINE=1
