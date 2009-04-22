#include <stdio.h>
#include <grub/types.h>
#include <grub/util/deviceiter.h>
#include <grub/util/ofpath.h>

void
grub_util_emit_devicemap_entry (FILE *fp, char *name, int is_floppy UNUSED,
				int *num_fd UNUSED, int *num_hd UNUSED)
{
  const char *ofpath = grub_util_devname_to_ofpath (name);

  fprintf(fp, "(%s)\t%s\n", ofpath, name);
}
