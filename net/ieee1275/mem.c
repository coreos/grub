#include <grub/types.h>
#include <grub/err.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/ieee1275/ieee1275.h>
#include <grub/net/mem.h>
#define TRASHOLD_SIZE 5 * 1024 * 1024

void *grub_net_malloc (grub_size_t size)
{
  
  int found = 0; 
  grub_addr_t found_addr;

  if (size <= TRASHOLD_SIZE) 
    return grub_malloc (size);

  for (found_addr = 0x800000; found_addr <  + 2000 * 0x100000; found_addr += 0x100000)
  {
    if (grub_claimmap (found_addr , size) != -1)
    {
      found = 1;
      break;
    }
  }

  if (!found)
    grub_error (GRUB_ERR_OUT_OF_MEMORY, "out of memory");
  
  return found?(void *) found_addr:NULL;

}
