#ifndef GRUB_MIPS_ASM_HEADER
#define GRUB_MIPS_ASM_HEADER	1

#if defined(_MIPS_SIM) && defined(_ABIN32) && _MIPS_SIM == _ABIN32
#define GRUB_ASM_T4 $a4
#define GRUB_ASM_T5 $a5
#else
#define GRUB_ASM_T4 $t4
#define GRUB_ASM_T5 $t5
#endif
#endif
