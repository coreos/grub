
static void
grub_file_progress_hook_real (grub_disk_addr_t sector,
			      unsigned offset, unsigned length,
			      void *data)
{
  grub_file_t file = data;
  file->progress_offset += length;
  show_progress (file->progress_offset, file->size);
}


GRUB_MOD_INIT (progress)
{
  grub_file_progress_hook = grub_file_progress_hook_real;
}

GRUB_MOD_FINI (progress)
{
  grub_file_progress_hook = 0;
}
