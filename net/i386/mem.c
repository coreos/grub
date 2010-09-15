#include <grub/types.h>
#include <grub/err.h>
#include <grub/misc.h>
#include <grub/mm.h>


void *grub_net_malloc (grub_size_t size)
{
  return grub_malloc (size);
}
