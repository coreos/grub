#ifndef GRUB_SYSTEM_CPU_HEADER
#define GRUB_SYSTEM_CPU_HEADER

enum
  {
    GRUB_ARM_MACHINE_TYPE_RASPBERRY_PI = 3138,
    GRUB_ARM_MACHINE_TYPE_FDT = 0xFFFFFFFF
  };

void grub_arm_disable_caches_mmu (void);

#endif /* ! GRUB_SYSTEM_CPU_HEADER */

