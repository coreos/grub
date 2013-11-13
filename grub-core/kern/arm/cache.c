#include <grub/dl.h>
#include <grub/cache.h>
#include <grub/arm/system.h>

/* This is only about cache architecture. It doesn't imply
   the CPU architecture.  */
static enum
  {
    ARCH_UNKNOWN,
    ARCH_ARMV5_WRITE_THROUGH,
    ARCH_ARMV6,
    ARCH_ARMV6_UNIFIED,
    ARCH_ARMV7
  } type = ARCH_UNKNOWN;

grub_uint32_t grub_arch_cache_dlinesz;
grub_uint32_t grub_arch_cache_ilinesz;

/* Prototypes for asm functions.  */
void grub_arch_sync_caches_armv6 (void *address, grub_size_t len);
void grub_arch_sync_caches_armv7 (void *address, grub_size_t len);
void grub_arm_disable_caches_mmu_armv6 (void);
void grub_arm_disable_caches_mmu_armv7 (void);
grub_uint32_t grub_arm_main_id (void);
grub_uint32_t grub_arm_cache_type (void);

static void
probe_caches (void)
{
  grub_uint32_t main_id, cache_type;

  /* Read main ID Register */
  main_id = grub_arm_main_id ();

  switch ((main_id >> 16) & 0xf)
    {
    case 0x3:
    case 0x4:
    case 0x5:
    case 0x6:
    case 0x7:
    case 0xf:
      break;
    default:
      grub_fatal ("Unsupported ARM ID 0x%x", main_id);
    }

  /* Read Cache Type Register */
  cache_type = grub_arm_cache_type ();

  switch (cache_type >> 24)
    {
    case 0x00:
    case 0x01:
      grub_arch_cache_dlinesz = 8 << ((cache_type >> 12) & 3);
      grub_arch_cache_ilinesz = 8 << (cache_type & 3);
      type = ARCH_ARMV5_WRITE_THROUGH;
      break;
    case 0x04:
    case 0x0a:
    case 0x0c:
    case 0x0e:
    case 0x1c:
      grub_arch_cache_dlinesz = 8 << ((cache_type >> 12) & 3);
      grub_arch_cache_ilinesz = 8 << (cache_type & 3);
      type = ARCH_ARMV6_UNIFIED;
      break;
    case 0x05:
    case 0x0b:
    case 0x0d:
    case 0x0f:
    case 0x1d:
      grub_arch_cache_dlinesz = 8 << ((cache_type >> 12) & 3);
      grub_arch_cache_ilinesz = 8 << (cache_type & 3);
      type = ARCH_ARMV6;
      break;
    case 0x80 ... 0x8f:
      grub_arch_cache_dlinesz = 4 << ((cache_type >> 16) & 0xf);
      grub_arch_cache_ilinesz = 4 << (cache_type & 0xf);
      type = ARCH_ARMV7;
      break;
    default:
      grub_fatal ("Unsupported cache type 0x%x", cache_type);
    }
}

void
grub_arch_sync_caches (void *address, grub_size_t len)
{
  if (type == ARCH_UNKNOWN)
    probe_caches ();
  switch (type)
    {
    case ARCH_ARMV6:
      grub_arch_sync_caches_armv6 (address, len);
      break;
    case ARCH_ARMV7:
      grub_arch_sync_caches_armv7 (address, len);
      break;
      /* Nothing to do.  */
    case ARCH_ARMV5_WRITE_THROUGH:
    case ARCH_ARMV6_UNIFIED:
      break;
      /* Pacify GCC.  */
    case ARCH_UNKNOWN:
      break;
    }
}

void
grub_arm_disable_caches_mmu (void)
{
  if (type == ARCH_UNKNOWN)
    probe_caches ();
  switch (type)
    {
    case ARCH_ARMV6_UNIFIED:
    case ARCH_ARMV6:
      grub_arm_disable_caches_mmu_armv6 ();
      break;
    case ARCH_ARMV7:
      grub_arm_disable_caches_mmu_armv7 ();
      break;
      /* Nothing to do.  */
    case ARCH_ARMV5_WRITE_THROUGH:
      break;
      /* Pacify GCC.  */
    case ARCH_UNKNOWN:
      break;
    }
}
