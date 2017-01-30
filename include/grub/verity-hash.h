/* CoreOS verity hash */

#define VERITY_ARG " verity.usrhash="
#define VERITY_ARG_LENGTH (sizeof (VERITY_ARG) - 1)
#define VERITY_HASH_LENGTH 64
#define VERITY_CMDLINE_LENGTH ((VERITY_ARG_LENGTH)+(VERITY_HASH_LENGTH))

#if defined(__aarch64__)
# define VERITY_HASH_OFFSET 512
#elif defined(__i386__) || defined(__amd64__)
# define VERITY_HASH_OFFSET 0x40
#else
# error Unsupported arch
#endif


/**
 * grub_pass_verity_hash - Reads the CoreOS verity hash value from a well known
 * kernel image offset and adds a kernel command line argument for it.
 *
 * @pImage: Kernel image buffer.
 * @cmdline: Kernel command line buffer.
 * @cmdline_max_len: Kernel command line buffer length.
 */

static inline void grub_pass_verity_hash(const void *pImage,
					 char *cmdline,
					 grub_size_t cmdline_max_len)
{
  const char *buf = pImage;
  grub_size_t cmdline_len;
  int i;

  for (i=VERITY_HASH_OFFSET; i<VERITY_HASH_OFFSET + VERITY_HASH_LENGTH; i++)
    {
      if (buf[i] < '0' || buf[i] > '9') // Not a number
	if (buf[i] < 'a' || buf[i] > 'f') // Not a hex letter
	  return;
    }

  cmdline_len = grub_strlen(cmdline);
  if (cmdline_len + VERITY_CMDLINE_LENGTH > cmdline_max_len)
    return;

  grub_memcpy (cmdline + cmdline_len, VERITY_ARG, VERITY_ARG_LENGTH);
  cmdline_len += VERITY_ARG_LENGTH;
  grub_memcpy (cmdline + cmdline_len, buf + VERITY_HASH_OFFSET,
	       VERITY_HASH_LENGTH);
  cmdline_len += VERITY_HASH_LENGTH;
  cmdline[cmdline_len] = '\0';
}
