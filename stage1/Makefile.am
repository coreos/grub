pkgdatadir = $(datadir)/$(PACKAGE)/$(host_cpu)-$(host_vendor)
nodist_pkgdata_DATA = stage1

BUILT_SOURCES = stage2_size.h
CLEANFILES = $(nodist_pkgdata_DATA) $(BUILT_SOURCES)

# We can't use builtins or standard includes.
AM_CFLAGS = $(STAGE1_CFLAGS) -fno-builtin -nostdinc
LDFLAGS = -nostdlib -Wl,-N -Wl,-Ttext -Wl,7C00

noinst_PROGRAMS = stage1.exec
stage1_exec_SOURCES = stage1.S stage1.h

%: %.exec
	$(OBJCOPY) -O binary $< $@
