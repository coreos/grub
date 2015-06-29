#define VERITY_ARG " verity.usrhash="
#define VERITY_HASH_OFFSET 0x40
#define VERITY_HASH_LENGTH 64

static inline void grub_pass_verity_hash(struct linux_kernel_header *lh,
					 char *cmdline)
{
  char *buf = (char *)lh;
  grub_size_t cmdline_len;
  int i;

  for (i=VERITY_HASH_OFFSET; i<VERITY_HASH_OFFSET + VERITY_HASH_LENGTH; i++)
    {
      if (buf[i] < '0' || buf[i] > '9') // Not a number
	if (buf[i] < 'a' || buf[i] > 'f') // Not a hex letter
	  return;
    }

  grub_memcpy (cmdline + grub_strlen(cmdline), VERITY_ARG,
	       sizeof (VERITY_ARG));
  cmdline_len = grub_strlen(cmdline);
  grub_memcpy (cmdline + cmdline_len, buf + VERITY_HASH_OFFSET,
	       VERITY_HASH_LENGTH);
  cmdline[cmdline_len + VERITY_HASH_LENGTH] = '\0';
}
