/*
** Support for NE2000 PCI clones added David Monro June 1997
** Generalised to other NICs by Ken Yap July 1997
**
** Most of this is taken from:
**
** /usr/src/linux/drivers/pci/pci.c
** /usr/src/linux/include/linux/pci.h
** /usr/src/linux/arch/i386/bios32.c
** /usr/src/linux/include/linux/bios32.h
** /usr/src/linux/drivers/net/ne.c
**
** Limitations: only finds devices on bus 0 (I figure that anybody with
** a multi-pcibus machine will have something better than an ne2000)
*/

#include "netboot.h"
#include "pci.h"

#define	DEBUG	1

static unsigned int pci_ioaddr = 0;

static unsigned long bios32_entry = 0;
static struct {
	unsigned long address;
	unsigned short segment;
} bios32_indirect = { 0, KERN_CODE_SEG };

static long pcibios_entry = 0;             
static struct {                    
	unsigned long address;            
	unsigned short segment;                
} pci_indirect = { 0, KERN_CODE_SEG };

static unsigned long bios32_service(unsigned long service)
{
	unsigned char return_code;	/* %al */
	unsigned long address;		/* %ebx */
	unsigned long length;		/* %ecx */
	unsigned long entry;		/* %edx */
	unsigned long flags;

	save_flags(flags);
	__asm__("lcall (%%edi)"
		: "=a" (return_code),
		  "=b" (address),
		  "=c" (length),
		  "=d" (entry)
		: "0" (service),
		  "1" (0),
		  "D" (&bios32_indirect));
	restore_flags(flags);

	switch (return_code) {
		case 0:
			return address + entry;
		case 0x80:	/* Not present */
			printf("bios32_service(%d) : not present\r\n", service);
			return 0;
		default: /* Shouldn't happen */
			printf("bios32_service(%d) : returned 0x%x, mail drew@colorado.edu\r\n",
				service, return_code);
			return 0;
	}
}

static int pcibios_read_config_byte(unsigned char bus,
        unsigned char device_fn, unsigned char where, unsigned char *value)
{
        unsigned long ret;
        unsigned long bx = (bus << 8) | device_fn;
        unsigned long flags;

        save_flags(flags);
        __asm__("lcall (%%esi)\n\t"
                "jc 1f\n\t"
                "xor %%ah, %%ah\n"
                "1:"
                : "=c" (*value),
                  "=a" (ret)
                : "1" (PCIBIOS_READ_CONFIG_BYTE),
                  "b" (bx),
                  "D" ((long) where),
                  "S" (&pci_indirect));
        restore_flags(flags);
        return (int) (ret & 0xff00) >> 8;
}

static int pcibios_read_config_dword (unsigned char bus,
        unsigned char device_fn, unsigned char where, unsigned int *value)
{
        unsigned long ret;
        unsigned long bx = (bus << 8) | device_fn;
        unsigned long flags;

        save_flags(flags);
        __asm__("lcall (%%esi)\n\t"
                "jc 1f\n\t"
                "xor %%ah, %%ah\n"
                "1:"
                : "=c" (*value),
                  "=a" (ret)
                : "1" (PCIBIOS_READ_CONFIG_DWORD),
                  "b" (bx),
                  "D" ((long) where),
                  "S" (&pci_indirect));
        restore_flags(flags);
        return (int) (ret & 0xff00) >> 8;
}

static void check_pcibios(void)
{
	unsigned long signature;          
	unsigned char present_status;
	unsigned char major_revision;
	unsigned char minor_revision;                                           
	unsigned long flags;                                            
	int pack;

	if ((pcibios_entry = bios32_service(PCI_SERVICE))) {
		pci_indirect.address = pcibios_entry;

		save_flags(flags);
		__asm__("lcall (%%edi)\n\t"
			"jc 1f\n\t"
			"xor %%ah, %%ah\n"
			"1:\tshl $8, %%eax\n\t"
			"movw %%bx, %%ax"
			: "=d" (signature),
			  "=a" (pack)
			: "1" (PCIBIOS_PCI_BIOS_PRESENT),
			  "D" (&pci_indirect)
			: "bx", "cx");
		restore_flags(flags);
		
		present_status = (pack >> 16) & 0xff;
		major_revision = (pack >> 8) & 0xff;
		minor_revision = pack & 0xff;
		if (present_status || (signature != PCI_SIGNATURE)) {
			printf("ERROR: BIOS32 says PCI BIOS, but no PCI "
				"BIOS????\r\n");
			pcibios_entry = 0;
		}
#if DEBUG
		if (pcibios_entry) {
			printf ("pcibios_init : PCI BIOS revision %b.%b"
				" entry at 0x%X\r\n", major_revision,
				minor_revision, pcibios_entry);
		}
#endif
	}
}

static void pcibios_init(void)
{
	union bios32 *check;
	unsigned char sum;
	int i, length;

	/*
	 * Follow the standard procedure for locating the BIOS32 Service
	 * directory by scanning the permissible address range from
	 * 0xe0000 through 0xfffff for a valid BIOS32 structure.
	 *
	 */

	for (check = (union bios32 *) 0xe0000; check <= (union bios32 *) 0xffff0; ++check) {
		if (check->fields.signature != BIOS32_SIGNATURE)
			continue;
		length = check->fields.length * 16;
		if (!length)
			continue;
		sum = 0;
		for (i = 0; i < length ; ++i)
			sum += check->chars[i];
		if (sum != 0)
			continue;
		if (check->fields.revision != 0) {
			printf("pcibios_init : unsupported revision %d at 0x%X, mail drew@colorado.edu\r\n",
				check->fields.revision, check);
			continue;
		}
#if DEBUG
		printf("pcibios_init : BIOS32 Service Directory "
			"structure at 0x%X\r\n", check);
#endif
		if (!bios32_entry) {
			if (check->fields.entry >= 0x100000) {
				printf("pcibios_init: entry in high "
					"memory, giving up\r\n");
				return;
			} else {
				bios32_entry = check->fields.entry;
#if DEBUG
				printf("pcibios_init : BIOS32 Service Directory"
					" entry at 0x%X\r\n", bios32_entry);
#endif
				bios32_indirect.address = bios32_entry;
			}
		}
	}
	if (bios32_entry)
		check_pcibios();
}

static void scan_bus(struct pci_device *pcidev)
{
	unsigned int devfn, l;
	unsigned char hdr_type = 0;
	unsigned short vendor, device;
	int i;

	for (devfn = 0; devfn < 0xff; ++devfn) {
		if (PCI_FUNC(devfn) == 0) {
			pcibios_read_config_byte(0, devfn,
				PCI_HEADER_TYPE, &hdr_type);
		} else if (!(hdr_type & 0x80)) {
			/* not a multi-function device */
			continue;
		}

		pcibios_read_config_dword(0, devfn,
			PCI_VENDOR_ID, &l);
		/* some broken boards return 0 if a slot is empty: */
		if (l == 0xffffffff || l == 0x00000000) {
			hdr_type = 0;
			continue;
		}
		vendor = l & 0xffff;
		device = (l >> 16) & 0xffff;
#if DEBUG
		printf("bus 0, function %x, vendor %x, device %x\r\n",
			devfn, vendor, device);
#endif
		for (i = 0; pcidev[i].vendor != 0; i++) {
			if (vendor == pcidev[i].vendor
				&& device == pcidev[i].dev_id) {
				pcibios_read_config_dword(0, devfn,
					PCI_BASE_ADDRESS_0, &pci_ioaddr);
				/* Strip the I/O address out of the
				 * returned value */
				pci_ioaddr &= PCI_BASE_ADDRESS_IO_MASK;
				printf("Found %s at 0x%x\r\n",
				pcidev[i].name, pci_ioaddr);
			}
		}
	}
}

void eth_pci_init(struct pci_device *pcidev, unsigned short *ioaddr)
{
	pcibios_init();
	if (!pcibios_entry) {
		printf("pci_init: no BIOS32 detected\r\n");
		return;
	}
	scan_bus(pcidev);
	/* we only return one address at the moment, maybe more later? */
	*ioaddr = pci_ioaddr;
}
