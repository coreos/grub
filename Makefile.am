# Stage 1 is built after Stage 2, so that the size of Stage 2 is
# encoded into Stage 1.
SUBDIRS = netboot stage2 stage1 lib grub docs debian
EXTRA_DIST = BUGS

# We get $(PACKAGE) and $(VERSION) from debian/changelog.
configure: debian/changelog
