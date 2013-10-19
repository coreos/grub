/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2013  Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GRUB_UTIL_INSTALL_HEADER
#define GRUB_UTIL_INSTALL_HEADER	1

#include <sys/types.h>
#include <stdio.h>

#include <grub/device.h>
#include <grub/disk.h>
#include <grub/emu/hostfile.h>

typedef enum {
  GRUB_COMPRESSION_AUTO,
  GRUB_COMPRESSION_NONE,
  GRUB_COMPRESSION_XZ,
  GRUB_COMPRESSION_LZMA
} grub_compression_t;

struct grub_install_image_target_desc;

void
grub_install_generate_image (const char *dir, const char *prefix,
			     FILE *out,
			     const char *outname, char *mods[],
			     char *memdisk_path, char **pubkey_paths,
			     size_t npubkeys,
			     char *config_path,
			     const struct grub_install_image_target_desc *image_target,
			     int note,
			     grub_compression_t comp);

const struct grub_install_image_target_desc *
grub_install_get_image_target (const char *arg);

void
grub_util_bios_setup (const char *dir,
		      const char *boot_file, const char *core_file,
		      const char *dest, int force,
		      int fs_probe, int allow_floppy);
void
grub_util_sparc_setup (const char *dir,
		       const char *boot_file, const char *core_file,
		       const char *dest, int force,
		       int fs_probe, int allow_floppy);

char *
grub_install_get_image_targets_string (void);

const char *
grub_util_get_target_dirname (const struct grub_install_image_target_desc *t);

void
grub_install_get_blocklist (grub_device_t root_dev,
			    const char *core_path, const char *core_img,
			    size_t core_size,
			    void (*callback) (grub_disk_addr_t sector,
					      unsigned offset,
					      unsigned length,
					      void *data),
			    void *hook_data);

void
grub_util_create_envblk_file (const char *name);

void
grub_util_render_label (const char *label_font,
			const char *label_bgcolor,
			const char *label_color,
			const char *label_string,
			const char *label);

#endif
