pkgdatadir = $(datadir)/$(PACKAGE)/$(host_cpu)-$(host_vendor)
nodist_pkgdata_DATA = stage1 stage1_lba

BUILT_SOURCES = stage2_size.h
CLEANFILES = $(pkgdata_DATA) $(BUILT_SOURCES)

# We can't use builtins or standard includes.
AM_CFLAGS = $(STAGE1_CFLAGS) -fno-builtin -nostdinc
LDFLAGS = -nostdlib -Wl,-N -Wl,-Ttext -Wl,7C00

noinst_PROGRAMS = stage1.exec stage1_lba.exec
stage1_exec_SOURCES = stage1.S stage1.h stage2_size.h
stage1_lba_exec_SOURCES = stage1_lba.S stage1.h stage2_size.h

stage2_size.h: ../stage2/stage2
	-rm -f stage2_size.h
	set dummy `ls -l ../stage2/stage2`; \
	echo "#define STAGE2_SIZE $$6" > stage2_size.h

%: %.exec
	$(OBJCOPY) -O binary $< $@
