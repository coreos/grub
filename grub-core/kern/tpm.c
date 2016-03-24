#include <grub/err.h>
#include <grub/i18n.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/tpm.h>
#include <grub/term.h>

grub_err_t
grub_tpm_measure (unsigned char *buf, grub_size_t size, grub_uint8_t pcr,
		  const char *kind, const char *description)
{
  grub_err_t ret;
  char *desc = grub_xasprintf("%s %s", kind, description);
  if (!desc)
    return GRUB_ERR_OUT_OF_MEMORY;
  ret = grub_tpm_log_event(buf, size, pcr, description);
  grub_free(desc);
  return ret;
}
