/*
** Support for NE2000 PCI clones added David Monro June 1997
** Generalised for other PCI NICs by Ken Yap July 1997
**
** Most of this is taken from:
**
** /usr/src/linux/drivers/pci/pci.c
** /usr/src/linux/include/linux/pci.h
** /usr/src/linux/arch/i386/bios32.c
** /usr/src/linux/include/linux/bios32.h
** /usr/src/linux/drivers/net/ne.c
*/

#define PCIBIOS_PCI_FUNCTION_ID         0xb1XX
#define PCIBIOS_PCI_BIOS_PRESENT        0xb101
#define PCIBIOS_FIND_PCI_DEVICE         0xb102
#define PCIBIOS_FIND_PCI_CLASS_CODE     0xb103
#define PCIBIOS_GENERATE_SPECIAL_CYCLE  0xb106
#define PCIBIOS_READ_CONFIG_BYTE        0xb108
#define PCIBIOS_READ_CONFIG_WORD        0xb109
#define PCIBIOS_READ_CONFIG_DWORD       0xb10a
#define PCIBIOS_WRITE_CONFIG_BYTE       0xb10b
#define PCIBIOS_WRITE_CONFIG_WORD       0xb10c
#define PCIBIOS_WRITE_CONFIG_DWORD      0xb10d

#define PCI_VENDOR_ID           0x00    /* 16 bits */
#define PCI_DEVICE_ID           0x02    /* 16 bits */
#define PCI_COMMAND             0x04    /* 16 bits */

#define PCI_HEADER_TYPE         0x0e    /* 8 bits */

#define PCI_BASE_ADDRESS_0      0x10    /* 32 bits */

#define  PCI_BASE_ADDRESS_IO_MASK       (~0x03)

#define PCI_FUNC(devfn)           ((devfn) & 0x07)

#define BIOS32_SIGNATURE        (('_' << 0) + ('3' << 8) + ('2' << 16) + ('_' << 24))

/* PCI signature: "PCI " */
#define PCI_SIGNATURE           (('P' << 0) + ('C' << 8) + ('I' << 16) + (' ' << 24))                                
 
/* PCI service signature: "$PCI" */                       
#define PCI_SERVICE             (('$' << 0) + ('P' << 8) + ('C' << 16) + ('I' << 24))                    

union bios32 {
	struct {
		unsigned long signature;	/* _32_ */
		unsigned long entry;		/* 32 bit physical address */
		unsigned char revision;		/* Revision level, 0 */
		unsigned char length;		/* Length in paragraphs should be 01 */
		unsigned char checksum;		/* All bytes must add up to zero */
		unsigned char reserved[5];	/* Must be zero */
	} fields;
	char chars[16];
};

#define KERN_CODE_SEG	0x8	/* This _MUST_ match start.S */

/* Stuff for asm */
#define save_flags(x) \
__asm__ __volatile__("pushfl ; popl %0":"=g" (x): /* no input */ :"memory")

#define restore_flags(x) \
__asm__ __volatile__("pushl %0 ; popfl": /* no output */ :"g" (x):"memory")

#define PCI_VENDOR_ID_REALTEK           0x10ec
#define PCI_DEVICE_ID_REALTEK_8029      0x8029
#define PCI_VENDOR_ID_WINBOND2          0x1050
#define PCI_DEVICE_ID_WINBOND2_89C940   0x0940
#define PCI_VENDOR_ID_COMPEX            0x11f6
#define PCI_DEVICE_ID_COMPEX_RL2000     0x1401
#define PCI_VENDOR_ID_KTI               0x8e2e
#define PCI_DEVICE_ID_KTI_ET32P2        0x3000
#define PCI_VENDOR_ID_INTEL             0x8086
#define PCI_DEVICE_ID_INTEL_82557       0x1229
#define PCI_VENDOR_ID_NETVIN            0x4a14
#define PCI_DEVICE_ID_NETVIN_NV5000SC   0x5000
#define PCI_VENDOR_ID_VIA		0x1106
#define PCI_DEVICE_ID_VIA_82C926	0x0926
#define PCI_VENDOR_ID_SURECOM		0x10bd
#define PCI_DEVICE_ID_SURECOM_NE34	0x0e34
#define PCI_VENDOR_ID_VORTEX		0x1119
#define PCI_DEVICE_ID_VORTEX_3c595	0x1234 /* Correct value unknown */

struct pci_device {
	unsigned short	vendor, dev_id;
	char		*name;
};

extern void	eth_pci_init(struct pci_device *, unsigned short *);
