#include	"netboot.h"
#include	"nic.h"

#include	"netboot_config.h"

#undef	INCLUDE_PCI
#if	defined(INCLUDE_NEPCI) || defined(INCLUDE_EEPRO100) || defined (INCLUDE_3C59X)
	/* || others later */
#if	defined(NETBOOT32)		/* only for 32 bit machines */
#define	INCLUDE_PCI
/* #include	<linux/pci.h> */
#include	"pci.h"
static unsigned short	pci_addrs[16];

struct pci_device	pci_nic_list[] = {
#ifdef	INCLUDE_NEPCI
	{ PCI_VENDOR_ID_REALTEK,	PCI_DEVICE_ID_REALTEK_8029,
		"Realtek 8029"},
	{ PCI_VENDOR_ID_WINBOND2,	PCI_DEVICE_ID_WINBOND2_89C940,
		"Winbond NE2000-PCI"}, /* "Winbond 89C940" */
	{ PCI_VENDOR_ID_COMPEX,		PCI_DEVICE_ID_COMPEX_RL2000,
		"Compex ReadyLink 2000"},
	{ PCI_VENDOR_ID_KTI,		PCI_DEVICE_ID_KTI_ET32P2,
		"KTI ET32P2"},
	{PCI_VENDOR_ID_NETVIN,		PCI_DEVICE_ID_NETVIN_NV5000SC,
         	"NetVin NV5000"},
	{PCI_VENDOR_ID_VIA,		PCI_DEVICE_ID_VIA_82C926,
	 	"VIA 82C926 Amazon"},
	{PCI_VENDOR_ID_SURECOM,		PCI_DEVICE_ID_SURECOM_NE34,
	 	"SureCom NE34"},
#endif
#ifdef	INCLUDE_EEPRO100
	{ PCI_VENDOR_ID_INTEL,		PCI_DEVICE_ID_INTEL_82557,
		"Intel EtherExpressPro100"},
#endif
#ifdef INCLUDE_3C59X
	{PCI_VENDOR_ID_VORTEX,	PCI_DEVICE_ID_VORTEX_3c595,	"3c595"},

#endif       

/* other PCI NICs go here */
	{0,}
};
#endif	/* NETBOOT32 */
#endif	/* INCLUDE_*PCI */

struct dispatch_table
{
	char		*nic_name;
	struct nic	*(*eth_probe)(struct nic *, unsigned short *);
	unsigned short	*probe_addrs;		/* for probe overrides */
};

#ifdef	INCLUDE_WD
extern struct nic	*wd_probe(struct nic *, unsigned short *);
#endif

#ifdef	INCLUDE_T503
extern struct nic	*t503_probe(struct nic *, unsigned short *);
#endif

#if	defined(INCLUDE_NE)
extern struct nic	*ne_probe(struct nic *, unsigned short *);
#endif

#if	defined(INCLUDE_NEPCI)
extern struct nic	*nepci_probe(struct nic *, unsigned short *);
#endif

#ifdef	INCLUDE_T509
extern struct nic	*t509_probe(struct nic *, unsigned short *);
#endif

#ifdef	INCLUDE_EEPRO100
extern struct nic	*eepro100_probe(struct nic *, unsigned short *);
#endif

#ifdef	INCLUDE_CS
extern struct nic	*cs89x0_probe(struct nic *, unsigned short *);
#endif

#ifdef	INCLUDE_NE2100
extern struct nic	*ne2100_probe(struct nic *, unsigned short *);
#endif

#ifdef	INCLUDE_3C59X
extern struct nic	*VX_probe(struct nic *, unsigned short *);
#endif


/*
 *	NIC probing is in order of appearance in this table.
 *	If for some reason you want to change the order,
 *	just rearrange the entries (bracketed by the #ifdef/#endif)
 */
static struct dispatch_table	NIC[] =
{
#ifdef	INCLUDE_WD
	{ "WD", wd_probe, 0 },
#endif
#ifdef	INCLUDE_T503
	{ "3C503", t503_probe, 0 },
#endif
#ifdef	INCLUDE_NE
	{ "NE*000", ne_probe, 0 },
#endif
#ifdef	INCLUDE_T509
	{ "3C5x9", t509_probe, 0 },
#endif
#ifdef	INCLUDE_EEPRO100
	{ "EEPRO100", eepro100_probe, 0 },
#endif
#ifdef	INCLUDE_CS
	{ "CS89x0", cs89x0_probe, 0 },
#endif
#ifdef	INCLUDE_NE2100
	{ "NE2100", ne2100_probe, 0 },
#endif
#ifdef	INCLUDE_NEPCI
	{ "NE*000/PCI", nepci_probe, pci_addrs },
#endif

#ifdef INCLUDE_3C59X
	{"VorTex/PCI", VX_probe, pci_addrs},
#endif

	/* this entry must always be last to mark the end of list */
	{ 0, 0, 0 }
};

#define	NIC_TABLE_SIZE	(sizeof(NIC)/sizeof(NIC[0]))

static int eth_dummy(struct nic *nic)
{
	return (0);
}

char	packet[ETH_MAX_PACKET];

struct nic	nic =
{
#ifdef	NETBOOT32
	(void (*)(struct nic *))eth_dummy,	/* reset */
	eth_dummy,				/* poll */
	(void (*)(struct nic *, char *,
		unsigned int, unsigned int,
		char *))eth_dummy,		/* transmit */
	(void (*)(struct nic *))eth_dummy,	/* disable */
#endif
/* bcc has problems with complicated casts */
#ifdef	NETBOOT16
	eth_dummy,
	eth_dummy,
	eth_dummy,
	eth_dummy,
#endif
	1,			/* aui */
	arptable[ARP_CLIENT].node,	/* node_addr */
	packet,			/* packet */
	0,			/* packetlen */
	0			/* priv_data */
};

#if 0
void print_config(void)
{
	struct dispatch_table	*t;

#ifdef	NETBOOT32
	printf("Etherboot/32 version " VERSION
#ifdef	NFS_BOOT
	" ESC for menu"
#endif
	" for ");
#endif
#ifdef	NETBOOT16
	/* you need to edit the version manually because bcc doesn't
	do string literal concatenation yet */
	printf("Etherboot/16 version 3.2 for ");
#endif
	for (t = NIC; t->nic_name != 0; ++t)
		printf("[%s]", t->nic_name);
	printf("\r\n");
}
#endif

void eth_reset(void)
{
	(*nic.reset)(&nic);
}

int eth_probe(void)
{
	struct dispatch_table	*t;

#ifdef	INCLUDE_PCI
	pci_addrs[0] = 0;
	eth_pci_init(pci_nic_list, pci_addrs);
	pci_addrs[1] = 0;
#endif
	printf("Probing...");
	for (t = NIC; t->nic_name != 0; ++t)
	{
		printf("[%s]", t->nic_name);
		if ((*t->eth_probe)(&nic, t->probe_addrs))
			return (1);
	}
	return (0);
}

int eth_poll(void)
{
	return ((*nic.poll)(&nic));
}

void eth_transmit(char *d, unsigned int t, unsigned int s, char *p)
{
	(*nic.transmit)(&nic, d, t, s, p);
	twiddle();
}

void eth_disable(void)
{
	(*nic.disable)(&nic);
}
