SUBDIRS = grub stage1 stage2 stage2_debug \
	e2fs_stage1_5 fat_stage1_5 ffs_stage1_5 \
	docs shared_src debian
DISTCLEANFILES = ssrcrules.mk
EXTRA_DIST = BUGS

# We get $(PACKAGE) and $(VERSION) from debian/changelog.
configure: debian/changelog
