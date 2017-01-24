#include <stdio.h>
#include <string.h>

#include <grub/elf.h>
#include <grub/module_verifier.h>
#include <grub/misc.h>
#include <grub/util/misc.h>

struct grub_module_verifier_arch archs[] = {
  { "i386", 4, 0, EM_386, GRUB_MODULE_VERIFY_SUPPORTS_REL, (int[]){
      R_386_32,
      R_386_PC32,
      -1
    } },
  { "x86_64", 8, 0, EM_X86_64, GRUB_MODULE_VERIFY_SUPPORTS_RELA, (int[]){
      R_X86_64_64,
      R_X86_64_PC64,
      /* R_X86_64_32, R_X86_64_32S are supported but shouldn't be used because of their limited range.  */
      -1
    }, (int[]){
      R_X86_64_PC32,
      -1
    }
  },
  { "powerpc", 4, 1, EM_PPC, GRUB_MODULE_VERIFY_SUPPORTS_RELA, (int[]){
      GRUB_ELF_R_PPC_ADDR16_LO,
      GRUB_ELF_R_PPC_REL24, /* It has limited range but GRUB adds trampolines when necessarry.  */
      GRUB_ELF_R_PPC_ADDR16_HA,
      GRUB_ELF_R_PPC_ADDR32,
      GRUB_ELF_R_PPC_REL32,
      -1
    } },
  { "sparc64", 8, 1, EM_SPARCV9, GRUB_MODULE_VERIFY_SUPPORTS_RELA, (int[]){
      R_SPARC_WDISP30, /* It has limited range but GRUB adds trampolines when necessarry. */
      R_SPARC_HH22,
      R_SPARC_HM10,
      R_SPARC_LM22,
      R_SPARC_LO10,
      R_SPARC_64,
      R_SPARC_OLO10,
      /* Following 2 relocations have limited range but unfortunately
	 clang generates them, as it doesn't implement mcmodel=large properly.
	 At least our heap and core are under 4G, so it's not a problem
	 usually. */
      R_SPARC_HI22,
      R_SPARC_32,
      -1
    } },
  { "ia64", 8, 0, EM_IA_64, GRUB_MODULE_VERIFY_SUPPORTS_RELA, (int[]){
      R_IA64_PCREL21B, /* We should verify that it's pointing either
			  to a function or to a section in the same module.
			  Checking that external symbol is a function is
			  non-trivial and I have never seen this relocation used
			  for anything else, so assume that it always points to a
			  function.
		       */
      R_IA64_SEGREL64LSB,
      R_IA64_FPTR64LSB,
      R_IA64_DIR64LSB,
      R_IA64_PCREL64LSB,
      R_IA64_LTOFF22X,
      R_IA64_LTOFF22,
      R_IA64_LTOFF_FPTR22,
      R_IA64_LDXMOV,
      -1
    }, (int[]){
      R_IA64_GPREL22,
      -1
    } },
  { "mipsel", 4, 0, EM_MIPS, GRUB_MODULE_VERIFY_SUPPORTS_REL | GRUB_MODULE_VERIFY_SUPPORTS_RELA, (int[]){
      R_MIPS_HI16,
      R_MIPS_LO16,
      R_MIPS_32,
      R_MIPS_GPREL32,
      R_MIPS_26,
      R_MIPS_GOT16,
      R_MIPS_CALL16,
      R_MIPS_JALR,
      -1
    } },
  { "mips", 4, 1, EM_MIPS, GRUB_MODULE_VERIFY_SUPPORTS_REL | GRUB_MODULE_VERIFY_SUPPORTS_RELA, (int[]){
      R_MIPS_HI16,
      R_MIPS_LO16,
      R_MIPS_32,
      R_MIPS_GPREL32,
      R_MIPS_26,
      R_MIPS_GOT16,
      R_MIPS_CALL16,
      R_MIPS_JALR,
      -1
    } },
  { "arm", 4, 0, EM_ARM, GRUB_MODULE_VERIFY_SUPPORTS_REL, (int[]){
      /* Some relocations are range-limited but trampolines are added when necessarry. */
      R_ARM_ABS32,
      R_ARM_CALL,
      R_ARM_JUMP24,
      R_ARM_THM_CALL,
      R_ARM_THM_JUMP24,
      R_ARM_V4BX,
      R_ARM_THM_MOVW_ABS_NC,
      R_ARM_THM_MOVT_ABS,
      R_ARM_THM_JUMP19,
      -1
    } },
  { "arm64", 8, 0, EM_AARCH64, GRUB_MODULE_VERIFY_SUPPORTS_REL | GRUB_MODULE_VERIFY_SUPPORTS_RELA, (int[]){
      R_AARCH64_ABS64,
      R_AARCH64_CALL26,
      R_AARCH64_JUMP26,
      -1
    }, (int[]){
      R_AARCH64_ADR_PREL_PG_HI21,
      R_AARCH64_ADD_ABS_LO12_NC,
      R_AARCH64_LDST64_ABS_LO12_NC,
      -1
    }
  },
};


int
main (int argc, char **argv)
{
  size_t module_size;
  unsigned arch;
  char *module_img;
  if (argc != 3) {
    fprintf (stderr, "usage: %s FILE ARCH\n", argv[0]);
    return 1;
  }

  for (arch = 0; arch < ARRAY_SIZE(archs); arch++)
    if (strcmp(archs[arch].name, argv[2]) == 0)
      break;
  if (arch == ARRAY_SIZE(archs))
    grub_util_error("unknown arch: %s", argv[2]);

  module_size = grub_util_get_image_size (argv[1]);
  module_img = grub_util_read_image (argv[1]);
  if (archs[arch].voidp_sizeof == 8)
    grub_module_verify64(module_img, module_size, &archs[arch]);
  else
    grub_module_verify32(module_img, module_size, &archs[arch]);
  return 0;
}
