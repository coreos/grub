/* 3c59x.c: A 3Com 3c59x/3c90x "Vortex/Boomerang" ethernet driver for linux. */
/*
	Written 1995 by Donald Becker.

	This software may be used and distributed according to the terms
	of the GNU Public License, incorporated herein by reference.

	This driver is for the 3Com "Vortex" series ethercards.  Members of
	the series include the 3c590 PCI EtherLink III and 3c595-Tx PCI Fast
	EtherLink.

	The author may be reached as becker@CESDIS.gsfc.nasa.gov, or C/O
	Center of Excellence in Space Data and Information Sciences
	   Code 930.5, Goddard Space Flight Center, Greenbelt MD 20771
*/

#ifdef INCLUDE_3C59X


#define lx_outb(a,b) outb(b,a)
#define lx_outw(a,b) outw(b,a)
#define lx_outl(a,b) outl(b,a)


static int __jiffies__;

#define jiffies ((__jiffies__+=1000))

void cli ()
{
  __asm__ __volatile__("cli");
}
#define HZ 1

void memset (char *add, int v, int s)
{
  while (s--)
	*add++ = (char )v;
}

static char *version = "3c59x.c:v0.30-all 12/23/96 becker@cesdis.gsfc.nasa.gov\n";

/* "Knobs" that turn on special features. */
/* Enable the automatic media selection code. */
#define AUTOMEDIA 1

/* Allow the use of bus master transfers instead of programmed-I/O for the
   Tx process.  Bus master transfers are always disabled by default, but
   iff this is set they may be turned on using 'options'. */

#define NO_VORTEX_BUS_MASTER 


#include "netboot.h"
#include "nic.h"
#include "3c509.h"
#include "pci.h"

#include "if.h"

#include "netdevice.h"

struct device card;

/* "Knobs" for adjusting internal parameters. */
/* Put out somewhat more debugging messages. (0 - no msg, 1 minimal msgs). */
#define VORTEX_DEBUG 2

/* Number of times to check to see if the Tx FIFO has space, used in some
   limited cases. */
#define WAIT_TX_AVAIL 200

/* Operational parameter that usually are not changed. */
/* Time in jiffies before concluding Tx hung */
#define TX_TIMEOUT	((400*HZ)/1000)

/* The total size is twice that of the original EtherLinkIII series: the
   runtime register window, window 1, is now always mapped in. */
#define VORTEX_TOTAL_SIZE 0x20

#ifdef HAVE_DEVLIST
struct netdev_entry tc59x_drv =
{"Vortex", vortex_pci_probe, VORTEX_TOTAL_SIZE, NULL};
#endif

#ifdef VORTEX_DEBUG
int vortex_debug = VORTEX_DEBUG;
#else
int vortex_debug = 1;
#endif

/* Caution!  These entries must be consistent, with the EISA ones last. */
static int product_ids[] = {0x5900, 0x5950, 0x5951, 0x5952, 0x9000,
								0x9001, 0x9050, 0x9051, 0, 0};
static const char *product_names[] = {
	"3c590 Vortex 10Mbps",
	"3c595 Vortex 100baseTX",
	"3c595 Vortex 100baseT4",
	"3c595 Vortex 100base-MII",
	"3c900 Boomerang 10baseT",
	"3c900 Boomerang 10Mbps/Combo",
	"3c905 Boomerang 100baseTx",
	"3c905 Boomerang 100baseT4",
	"3c592 EISA 10mbps Demon/Vortex",
	"3c597 EISA Fast Demon/Vortex",
};
#define DEMON10_INDEX 8
#define DEMON100_INDEX 9

/*
				Theory of Operation

I. Board Compatibility

This device driver is designed for the 3Com FastEtherLink, 3Com's PCI to
10/100baseT adapter.  It also works with the 3c590, a similar product
with only a 10Mbs interface.

II. Board-specific settings

PCI bus devices are configured by the system at boot time, so no jumpers
need to be set on the board.  The system BIOS should be set to assign the
PCI INTA signal to an otherwise unused system IRQ line.  While it's
physically possible to shared PCI interrupt lines, the 1.2.0 kernel doesn't
support it.

III. Driver operation

The 3c59x series use an interface that's very similar to the previous 3c5x9
series.  The primary interface is two programmed-I/O FIFOs, with an
alternate single-contiguous-region bus-master transfer (see next).

One extension that is advertised in a very large font is that the adapters
are capable of being bus masters.  Unfortunately this capability is only for
a single contiguous region making it less useful than the list of transfer
regions available with the DEC Tulip or AMD PCnet.  Given the significant
performance impact of taking an extra interrupt for each transfer, using
DMA transfers is a win only with large blocks.

IIIC. Synchronization
The driver runs as two independent, single-threaded flows of control.  One
is the send-packet routine, which enforces single-threaded use by the
dev->tbusy flag.  The other thread is the interrupt handler, which is single
threaded by the hardware and other software.

IV. Notes

Thanks to Cameron Spitzer and Terry Murphy of 3Com for providing both
3c590 and 3c595 boards.
The name "Vortex" is the internal 3Com project name for the PCI ASIC, and
the EISA version is called "Demon".  According to Terry these names come
from rides at the local amusement park.

The new chips support both ethernet (1.5K) and FDDI (4.5K) packet sizes!
This driver only supports ethernet packets because of the skbuff allocation
limit of 4K.
*/

#define TCOM_VENDOR_ID	0x10B7		/* 3Com's manufacturer's ID. */

/* Operational definitions.
   These are not used by other compilation units and thus are not
   exported in a ".h" file.

   First the windows.  There are eight register windows, with the command
   and status registers available in each.
   */
#define EL3WINDOW(win_num) lx_outw(SelectWindow + (win_num), ioaddr + EL3_CMD)
#define EL3_CMD 0x0e
#define EL3_STATUS 0x0e

/* The top five bits written to EL3_CMD are a command, the lower
   11 bits are the parameter, if applicable.
   Note that 11 parameters bits was fine for ethernet, but the new chip
   can handle FDDI length frames (~4500 octets) and now parameters count
   32-bit 'Dwords' rather than octets. */

enum vortex_cmd {
	TotalReset = 0<<11, SelectWindow = 1<<11, StartCoax = 2<<11,
	RxDisable = 3<<11, RxEnable = 4<<11, RxReset = 5<<11, RxDiscard = 8<<11,
	TxEnable = 9<<11, TxDisable = 10<<11, TxReset = 11<<11,
	FakeIntr = 12<<11, AckIntr = 13<<11, SetIntrEnb = 14<<11,
	SetStatusEnb = 15<<11, SetRxFilter = 16<<11, SetRxThreshold = 17<<11,
	SetTxThreshold = 18<<11, SetTxStart = 19<<11,
	StartDMAUp = 20<<11, StartDMADown = (20<<11)+1, StatsEnable = 21<<11,
	StatsDisable = 22<<11, StopCoax = 23<<11,};

/* The SetRxFilter command accepts the following classes: */
enum RxFilter {
	RxStation = 1, RxMulticast = 2, RxBroadcast = 4, RxProm = 8 };

/* Bits in the general status register. */
enum vortex_status {
	IntLatch = 0x0001, AdapterFailure = 0x0002, TxComplete = 0x0004,
	TxAvailable = 0x0008, RxComplete = 0x0010, RxEarly = 0x0020,
	IntReq = 0x0040, StatsFull = 0x0080, DMADone = 1<<8,
	DMAInProgress = 1<<11,			/* DMA controller is still busy.*/
	CmdInProgress = 1<<12,			/* EL3_CMD is still busy.*/
};

/* Register window 1 offsets, the window used in normal operation.
   On the Vortex this window is always mapped at offsets 0x10-0x1f. */
enum Window1 {
	TX_FIFO = 0x10,  RX_FIFO = 0x10,  RxErrors = 0x14,
	RxStatus = 0x18,  Timer=0x1A, TxStatus = 0x1B,
	TxFree = 0x1C, /* Remaining free bytes in Tx buffer. */
};
enum Window0 {
	Wn0EepromCmd = 10,		/* Window 0: EEPROM command register. */
	Wn0EepromData = 12,		/* Window 0: EEPROM results register. */
};
enum Win0_EEPROM_bits {
	EEPROM_Read = 0x80, EEPROM_WRITE = 0x40, EEPROM_ERASE = 0xC0,
	EEPROM_EWENB = 0x30,		/* Enable erasing/writing for 10 msec. */
	EEPROM_EWDIS = 0x00,		/* Disable EWENB before 10 msec timeout. */
};
/* EEPROM locations. */
enum eeprom_offset {
	PhysAddr01=0, PhysAddr23=1, PhysAddr45=2, ModelID=3,
	EtherLink3ID=7, IFXcvrIO=8, IRQLine=9,
	NodeAddr01=10, NodeAddr23=11, NodeAddr45=12,
	DriverTune=13, Checksum=15};

enum Window3 {			/* Window 3: MAC/config bits. */
	Wn3_Config=0, Wn3_MAC_Ctrl=6, Wn3_Options=8,
};
union wn3_config {
	int i;
	struct w3_config_fields {
		unsigned int ram_size:3, ram_width:1, ram_speed:2, rom_size:2;
		int pad8:8;
		unsigned int ram_split:2, pad18:2, xcvr:3, pad21:1, autoselect:1;
		int pad24:7;
	} u;
};

enum Window4 {
	Wn4_Media = 0x0A,		/* Window 4: Various transcvr/media bits. */
};
enum Win4_Media_bits {
	Media_SQE = 0x0008,		/* Enable SQE error counting for AUI. */
	Media_10TP = 0x00C0,	/* Enable link beat and jabber for 10baseT. */
	Media_Lnk = 0x0080,		/* Enable just link beat for 100TX/100FX. */
	Media_LnkBeat = 0x0800,
};
enum Window7 {					/* Window 7: Bus Master control. */
	Wn7_MasterAddr = 0, Wn7_MasterLen = 6, Wn7_MasterStatus = 12,
};

struct vortex_private {
	char devname[8];			/* "ethN" string, also for kernel debug. */
	const char *product_name;
	int options;				/* User-settable misc. driver options. */
	int last_rx_packets;		/* For media autoselection. */
	unsigned int available_media:8,	/* From Wn3_Options */
	  media_override:3, 			/* Passed-in media type. */
	  default_media:3,			/* Read from the EEPROM. */
	  full_duplex:1, bus_master:1, autoselect:1;
};

/* The action to take with a media selection timer tick.
   Note that we deviate from the 3Com order by checking 10base2 before AUI.
 */
static struct media_table {
  char *name;
  unsigned int media_bits:16,		/* Bits to set in Wn4_Media register. */
	mask:8,				/* The transceiver-present bit in Wn3_Config.*/
	next:8;				/* The media type to try next. */
  short wait;			/* Time before we check media status. */
} media_tbl[] = {
  {	"10baseT",   Media_10TP,0x08, 3 /* 10baseT->10base2 */, (14*HZ)/10},
  { "10Mbs AUI", Media_SQE, 0x20, 8 /* AUI->default */, (1*HZ)/10},
  { "undefined", 0,			0x80, 0 /* Undefined */, 0},
  { "10base2",   0,			0x10, 1 /* 10base2->AUI. */, (1*HZ)/10},
  { "100baseTX", Media_Lnk, 0x02, 5 /* 100baseTX->100baseFX */, (14*HZ)/10},
  { "100baseFX", Media_Lnk, 0x04, 6 /* 100baseFX->MII */, (14*HZ)/10},
  { "MII",		 0,			0x40, 0 /* MII->10baseT */, (14*HZ)/10},
  { "undefined", 0,			0x01, 0 /* Undefined/100baseT4 */, 0},
  { "Default",	 0,			0xFF, 0 /* Use default */, 0},
};

static int vortex_scan(struct device *dev);
static int vortex_found_device(struct device *dev, int ioaddr, int irq,
							   int product_index, int options);
static int vortex_probe1(struct device *dev);
static int vortex_open(struct device *dev);
static void vortex_timer(unsigned long arg);
static int vortex_start_xmit(struct device *dev,char *data, int len);
static int vortex_rx(struct device *dev, char *data, int len);

static int vortex_close(struct device *dev);
static void update_stats(int addr, struct device *dev);
static struct enet_statistics *vortex_get_stats(struct device *dev);
static void set_rx_mode(struct device *dev);
#ifndef NEW_MULTICAST
static void set_multicast_list(struct device *dev, int num_addrs, void *addrs);
#endif


/* Unlike the other PCI cards the 59x cards don't need a large contiguous
   memory region, so making the driver a loadable module is feasible.

   Unfortunately maximizing the shared code between the integrated and
   module version of the driver results in a complicated set of initialization
   procedures.
   init_module() -- modules /  tc59x_init()  -- built-in
		The wrappers for vortex_scan()
   vortex_scan()  		 The common routine that scans for PCI and EISA cards
   vortex_found_device() Allocate a device structure when we find a card.
					Different versions exist for modules and built-in.
   vortex_probe1()		Fill in the device structure -- this is separated
					so that the modules code can put it in dev->init.
*/
/* This driver uses 'options' to pass the media type, full-duplex flag, etc. */
/* Note: this is the only limit on the number of cards supported!! */
static int options[8] = { -1, -1, -1, -1, -1, -1, -1, -1,};
/* Variables to work-around the Compaq PCI BIOS32 problem. */
static int compaq_ioaddr = 0, compaq_irq = 0, compaq_prod_id = 0;

int tc59x_probe(struct device *dev)
{
	int cards_found = 0;

	cards_found = vortex_scan(dev);

	if (vortex_debug > 0  &&  cards_found)
		printf(version);

	return cards_found ? 0 : -1;
}

static int vortex_scan(struct device *dev)
{
	int cards_found = 0;

#ifndef NO_PCI
/* 	if (pcibios_present()) { */
	if (1) {
		static int pci_index = 0;
		static int board_index = 0;
		for (; product_ids[board_index]; board_index++, pci_index = 0) {
			for (; pci_index < 16; pci_index++) {
				unsigned char pci_bus, pci_device_fn, pci_irq_line;
				unsigned char pci_latency;
				unsigned int pci_ioaddr;
				unsigned short pci_command;

				struct pci_device pcidev;

				eth_pci_init (&pcidev, &pci_ioaddr);
				/* Remove I/O space marker in bit 0. */

#ifdef VORTEX_BUS_MASTER
				/* Get and check the bus-master and latency values.
				   Some PCI BIOSes fail to set the master-enable bit, and
				   the latency timer must be set to the maximum value to avoid
				   data corruption that occurs when the timer expires during
				   a transfer.  Yes, it's a bug. */
				pcibios_read_config_word(pci_bus, pci_device_fn,
										 PCI_COMMAND, &pci_command);
				if ( ! (pci_command & PCI_COMMAND_MASTER)) {
					printf("  PCI Master Bit has not been set! Setting...\n");
					pci_command |= PCI_COMMAND_MASTER;
					pcibios_write_config_word(pci_bus, pci_device_fn,
											  PCI_COMMAND, pci_command);
				}
				pcibios_read_config_byte(pci_bus, pci_device_fn,
										 PCI_LATENCY_TIMER, &pci_latency);
				if (pci_latency != 255) {
					printf("  3Com EtherLink III: Overriding PCI latency"
						   " timer (CFLT) setting of %d, new value is 255.\n",
						   pci_latency);
					pcibios_write_config_byte(pci_bus, pci_device_fn,
											  PCI_LATENCY_TIMER, 255);
				}
#endif  /* VORTEX_BUS_MASTER */
				vortex_found_device(dev, pci_ioaddr, pci_irq_line, board_index,
									dev && dev->mem_start ? dev->mem_start
									: options[cards_found]);
				dev = 0;
				cards_found++;
			}
		}
	}
#endif /* NO_PCI */


	/* Special code to work-around the Compaq PCI BIOS32 problem. */
	if (compaq_ioaddr) {
		vortex_found_device(dev, compaq_ioaddr, compaq_irq, compaq_prod_id,
							dev && dev->mem_start ? dev->mem_start : options[cards_found]);
		cards_found++;
		dev = 0;
	}

	return cards_found;
}

static int vortex_found_device(struct device *dev, int ioaddr, int irq,
							   int product_index, int options)
{
	struct vortex_private *vp;
	static struct vortex_private vps;

	if (dev) {
	    /*dev->priv = kmalloc(sizeof (struct vortex_private), GFP_KERNEL); */
	    dev->priv = &vps;
		memset(dev->priv, 0, sizeof (struct vortex_private));
	}
#if 0
	dev = init_etherdev(dev, sizeof(struct vortex_private));
#endif
	dev->base_addr = ioaddr;
	dev->irq = irq;
	vp  = (struct vortex_private *)dev->priv;
	vp->product_name = product_names[product_index];
	vp->options = options;
	if (options >= 0) {
		vp->media_override = ((options & 7) == 2)  ?  0  :  options & 7;
		vp->full_duplex = (options & 8) ? 1 : 0;
		vp->bus_master = (options & 16) ? 1 : 0;
	} else {
		vp->media_override = 7;
		vp->full_duplex = 0;
		vp->bus_master = 0;
	}

	vortex_probe1(dev);
	return 0;
}

static int vortex_probe1(struct device *dev)
{
	int ioaddr = dev->base_addr;
	struct vortex_private *vp = (struct vortex_private *)dev->priv;
	int i;

	printf("%s: 3Com %s at %#3x,", dev->name,
		   vp->product_name, ioaddr);

	/* Read the station address from the EEPROM. */
	EL3WINDOW(0);
	for (i = 0; i < 3; i++) {
		short *phys_addr = (short *)dev->dev_addr;
		int timer;
		lx_outw(EEPROM_Read + PhysAddr01 + i, ioaddr + Wn0EepromCmd);
		/* Pause for at least 162 us. for the read to take place. */
		for (timer = 162*4 + 400; timer >= 0; timer--) {
			SLOW_DOWN_IO;
			if ((inw(ioaddr + Wn0EepromCmd) & 0x8000) == 0)
				break;
		}
		phys_addr[i] = htons(inw(ioaddr + Wn0EepromData));
	}
	for (i = 0; i < 6; i++)
		printf("%c%x", i ? ':' : ' ', dev->dev_addr[i]);
	printf(", IRQ %d\n", dev->irq);
	/* Tell them about an invalid IRQ. */
	if (vortex_debug && (dev->irq <= 0 || dev->irq > 15))
		printf(" *** Warning: this IRQ is unlikely to work!\n");

	{
		char *ram_split[] = {"5:3", "3:1", "1:1", "3:5"};
		union wn3_config config;
		EL3WINDOW(3);
		vp->available_media = inw(ioaddr + Wn3_Options);
		config.i = inl(ioaddr + Wn3_Config);
		if (vortex_debug > 1)
			printf("  Internal config register is %4.4x, transceivers %#x.\n",
				   config.i, inw(ioaddr + Wn3_Options));
		printf("  %dK %s-wide RAM %s Rx:Tx split, %s%s interface.\n",
			   8 << config.u.ram_size,
			   config.u.ram_width ? "word" : "byte",
			   ram_split[config.u.ram_split],
			   config.u.autoselect ? "autoselect/" : "",
			   media_tbl[config.u.xcvr].name);
		dev->if_port = config.u.xcvr;
		vp->default_media = config.u.xcvr;
		vp->autoselect = config.u.autoselect;
	}

#if 0
	/* We do a request_region() only to register /proc/ioports info. */
	request_region(ioaddr, VORTEX_TOTAL_SIZE, vp->product_name);
#endif
	return 0;
}


static int
vortex_open(struct device *dev)
{
	int ioaddr = dev->base_addr;
	struct vortex_private *vp = (struct vortex_private *)dev->priv;
	union wn3_config config;
	int i;

	/* Before initializing select the active media port. */
	EL3WINDOW(3);
	if (vp->full_duplex)
		lx_outb(0x20, ioaddr + Wn3_MAC_Ctrl); /* Set the full-duplex bit. */
	config.i = inl(ioaddr + Wn3_Config);

	if (vp->media_override != 7) {
		if (vortex_debug > 1)
			printf("%s: Media override to transceiver %d (%s).\n",
				   dev->name, vp->media_override,
				   media_tbl[vp->media_override].name);
		dev->if_port = vp->media_override;
	} else if (vp->autoselect) {
		/* Find first available media type, starting with 100baseTx. */
		dev->if_port = 4;
		while (! (vp->available_media & media_tbl[dev->if_port].mask))
			dev->if_port = media_tbl[dev->if_port].next;

		if (vortex_debug > 1)
			printf("%s: Initial media type %s.\n",
				   dev->name, media_tbl[dev->if_port].name);

#if 0
		vp->timer.expires = RUN_AT(media_tbl[dev->if_port].wait);
		vp->timer.data = (unsigned long)dev;
		vp->timer.function = &vortex_timer;    /* timer handler */
		add_timer(&vp->timer);
#endif
	} else
		dev->if_port = vp->default_media;

	config.u.xcvr = dev->if_port;
	lx_outl(config.i, ioaddr + Wn3_Config);

	if (vortex_debug > 1) {
		printf("%s: vortex_open() InternalConfig %8.8x.\n",
			dev->name, config.i);
	}

	lx_outw(TxReset, ioaddr + EL3_CMD);
	for (i = 20; i >= 0 ; i--)
		if ( ! (inw(ioaddr + EL3_STATUS) & CmdInProgress))
			break;

	lx_outw(RxReset, ioaddr + EL3_CMD);
	/* Wait a few ticks for the RxReset command to complete. */
	for (i = 20; i >= 0 ; i--)
		if ( ! (inw(ioaddr + EL3_STATUS) & CmdInProgress))
			break;

	lx_outw(SetStatusEnb | 0x00, ioaddr + EL3_CMD);

#if 0
#ifdef SA_SHIRQ		/* Use the now-standard shared IRQ implementation. */
	if (request_irq(dev->irq, &vortex_interrupt, SA_SHIRQ,
					vp->product_name, dev)) {
		return -EAGAIN;
	}
#else
#ifdef USE_SHARED_IRQ  /* Use my shared IRQ implementation. */
	i = request_shared_irq(dev->irq, &vortex_interrupt, dev, vp->product_name);
	if (i)						/* Error */
		return i;
#else
	if (dev->irq == 0  ||  irq2dev_map[dev->irq] != NULL)
		return -EAGAIN;
	irq2dev_map[dev->irq] = dev;
	if (REQUEST_IRQ(dev->irq, &vortex_interrupt, 0, vp->product_name, NULL)) {
		irq2dev_map[dev->irq] = NULL;
		return -EAGAIN;
	}
#endif  /* USE_SHARED_IRQ */
#endif  /* SA_SHIRQ */
#endif
	if (vortex_debug > 1) {
		EL3WINDOW(4);
		printf("%s: vortex_open() irq %d media status %4.4x.\n",
			   dev->name, dev->irq, inw(ioaddr + Wn4_Media));
	}

	/* Set the station address and mask in window 2 each time opened. */
	EL3WINDOW(2);
	for (i = 0; i < 6; i++)
		lx_outb(dev->dev_addr[i], ioaddr + i);
	for (; i < 12; i+=2)
		lx_outw(0, ioaddr + i);

	if (dev->if_port == 3)
		/* Start the thinnet transceiver. We should really wait 50ms...*/
		lx_outw(StartCoax, ioaddr + EL3_CMD);
	EL3WINDOW(4);
	lx_outw((inw(ioaddr + Wn4_Media) & ~(Media_10TP|Media_SQE)) |
		 media_tbl[dev->if_port].media_bits, ioaddr + Wn4_Media);

	/* Switch to the stats window, and clear all stats by reading. */
	lx_outw(StatsDisable, ioaddr + EL3_CMD);
	EL3WINDOW(6);
	for (i = 0; i < 10; i++)	
		inb(ioaddr + i);
	inw(ioaddr + 10);
	inw(ioaddr + 12);
	/* New: On the Vortex we must also clear the BadSSD counter. */
	EL3WINDOW(4);
	inb(ioaddr + 12);

	/* Switch to register set 7 for normal use. */
	EL3WINDOW(7);

	/* Set reciever mode: presumably accept b-case and phys addr only. */
	set_rx_mode(dev);
	lx_outw(StatsEnable, ioaddr + EL3_CMD); /* Turn on statistics. */

	dev->tbusy = 0;
	dev->interrupt = 0;
	dev->start = 1;

	lx_outw(RxEnable, ioaddr + EL3_CMD); /* Enable the receiver. */
	lx_outw(TxEnable, ioaddr + EL3_CMD); /* Enable transmitter. */
	/* Allow status bits to be seen. */
	lx_outw(SetStatusEnb | 0x1ff, ioaddr + EL3_CMD);
	/* Ack all pending events, and set active indicator mask. */
	lx_outw(AckIntr | IntLatch | TxAvailable | RxEarly | IntReq,
		 ioaddr + EL3_CMD);
	lx_outw(SetIntrEnb | IntLatch | TxAvailable | RxComplete | StatsFull
		 | DMADone, ioaddr + EL3_CMD);

#ifdef MODULE
	MOD_INC_USE_COUNT;
#endif

	return 0;
}

static void vortex_timer(unsigned long data)
{
#ifdef AUTOMEDIA
	struct device *dev = (struct device *)data;
	struct vortex_private *vp = (struct vortex_private *)dev->priv;
	int ioaddr = dev->base_addr;
	unsigned long flags;
	int ok = 0;

	if (vortex_debug > 1)
		printf("%s: Media selection timer tick happened, %s.\n",
			   dev->name, media_tbl[dev->if_port].name);

	save_flags(flags);	cli(); {
	  int old_window = inw(ioaddr + EL3_CMD) >> 13;
	  int media_status;
	  EL3WINDOW(4);
	  media_status = inw(ioaddr + Wn4_Media);
	  switch (dev->if_port) {
	  case 0:  case 4:  case 5:		/* 10baseT, 100baseTX, 100baseFX  */
		if (media_status & Media_LnkBeat) {
		  ok = 1;
		  if (vortex_debug > 1)
			printf("%s: Media %s has link beat, %x.\n",
				   dev->name, media_tbl[dev->if_port].name, media_status);
		} else if (vortex_debug > 1)
		  printf("%s: Media %s is has no link beat, %x.\n",
				   dev->name, media_tbl[dev->if_port].name, media_status);
 
		break;
	  default:					/* Other media types handled by Tx timeouts. */
		if (vortex_debug > 1)
		  printf("%s: Media %s is has no indication, %x.\n",
				 dev->name, media_tbl[dev->if_port].name, media_status);
		ok = 1;
	  }
	  if ( ! ok) {
		union wn3_config config;

		do {
			dev->if_port = media_tbl[dev->if_port].next;
		} while ( ! (vp->available_media & media_tbl[dev->if_port].mask));
		if (dev->if_port == 8) { /* Go back to default. */
		  dev->if_port = vp->default_media;
		  if (vortex_debug > 1)
			printf("%s: Media selection failing, using default %s port.\n",
				   dev->name, media_tbl[dev->if_port].name);
		} else {
		  if (vortex_debug > 1)
			printf("%s: Media selection failed, now trying %s port.\n",
				   dev->name, media_tbl[dev->if_port].name);
#if 0
		  vp->timer.expires = RUN_AT(media_tbl[dev->if_port].wait);
		  add_timer(&vp->timer);
#endif
		}
		lx_outw((media_status & ~(Media_10TP|Media_SQE)) |
			 media_tbl[dev->if_port].media_bits, ioaddr + Wn4_Media);

		EL3WINDOW(3);
		config.i = inl(ioaddr + Wn3_Config);
		config.u.xcvr = dev->if_port;
		lx_outl(config.i, ioaddr + Wn3_Config);

		lx_outw(dev->if_port == 3 ? StartCoax : StopCoax, ioaddr + EL3_CMD);
	  }
	  EL3WINDOW(old_window);
	}   restore_flags(flags);
	if (vortex_debug > 1)
	  printf("%s: Media selection timer finished, %s.\n",
			 dev->name, media_tbl[dev->if_port].name);

#endif /* AUTOMEDIA*/
	return;
}

static int
vortex_start_xmit(struct device *dev, char *data, int len)
{
	struct vortex_private *vp = (struct vortex_private *)dev->priv;
	int ioaddr = dev->base_addr;

	/* Part of the following code is inspired by code from Giuseppe Ciaccio,
	   ciaccio@disi.unige.it.
	   It works around a ?bug? in the 8K Vortex that only occurs on some
	   systems: the TxAvailable interrupt seems to be lost.
	   The ugly work-around is to busy-wait for room available in the Tx
	   buffer before deciding the transmitter is actually hung.
	   This busy-wait should never really occur, since the problem is that
	   there actually *is*  room in the Tx FIFO.

	   This pointed out an optimization -- we can ignore dev->tbusy if
	   we actually have room for this packet.
	   */

	if (dev->tbusy) {
		/* Transmitter timeout, serious problems. */
		int tickssofar = jiffies - dev->trans_start;
		int i;

		if (tickssofar < TX_TIMEOUT-2)		/* We probably aren't empty. */
			return 1;
		/* Wait a while to see if there really is room. */
		for (i = WAIT_TX_AVAIL; i >= 0; i--)
			if (inw(ioaddr + TxFree) > len)
			  break;
		if ( i < 0) {
			if (tickssofar < TX_TIMEOUT)
				return 1;
			printf("%s: transmit timed out, tx_status %2.2x status %4.4x.\n",
				   dev->name, inb(ioaddr + TxStatus), inw(ioaddr + EL3_STATUS));
			/* Issue TX_RESET and TX_START commands. */
			lx_outw(TxReset, ioaddr + EL3_CMD);
			for (i = 20; i >= 0 ; i--)
				if ( ! (inw(ioaddr + EL3_STATUS) & CmdInProgress))
					break;
			lx_outw(TxEnable, ioaddr + EL3_CMD);
			dev->trans_start = jiffies;
			dev->tbusy = 0;
#if 0
			vp->stats.tx_errors++;
			vp->stats.tx_dropped++;
#endif
			return 0;			/* Yes, silently *drop* the packet! */
		}
	}

	/* Block a timer-based transmit from overlapping.  This could better be
	   done with atomic_swap(1, dev->tbusy), but set_bit() works as well.
	   If this ever occurs the queue layer is doing something evil! */
#if 0
	if (set_bit(0, (void*)&dev->tbusy) != 0) {
		printf("%s: Transmitter access conflict.\n", dev->name);
		return 1;
	}
#endif
	/* Put out the doubleword header... */
	lx_outl(len, ioaddr + TX_FIFO);
#ifdef VORTEX_BUS_MASTER
	if (vp->bus_master) {
		/* Set the bus-master controller to transfer the packet. */
		lx_outl((int)(data), ioaddr + Wn7_MasterAddr);
		lx_outw((len + 3) & ~3, ioaddr + Wn7_MasterLen);
/* 		vp->tx_skb = skb; */
		lx_outw(StartDMADown, ioaddr + EL3_CMD);
		/* dev->tbusy will be cleared at the DMADone interrupt. */
	} else {
		/* ... and the packet rounded to a doubleword. */
		outsl(ioaddr + TX_FIFO, data, (len + 3) >> 2);
/* 		dev_kfree_skb (skb, FREE_WRITE); */
		if (inw(ioaddr + TxFree) > 1536) {
			dev->tbusy = 0;
		} else
			/* Interrupt us when the FIFO has room for max-sized packet. */
			lx_outw(SetTxThreshold + (1536>>2), ioaddr + EL3_CMD);
	}
#else
	/* ... and the packet rounded to a doubleword. */
	outsl(ioaddr + TX_FIFO, data, (len + 3) >> 2);
/* 	dev_kfree_skb (skb, FREE_WRITE); */
	if (inw(ioaddr + TxFree) > 1536) {
		dev->tbusy = 0;
	} else
		/* Interrupt us when the FIFO has room for max-sized packet. */
		lx_outw(SetTxThreshold + (1536>>2), ioaddr + EL3_CMD);
#endif  /* bus master */

	dev->trans_start = jiffies;

	/* Clear the Tx status stack. */
	{
		short tx_status;
		int i = 4;

		while (--i > 0	&&	(tx_status = inb(ioaddr + TxStatus)) > 0) {
			if (tx_status & 0x3C) {		/* A Tx-disabling error occurred.  */
				if (vortex_debug > 2)
				  printf("%s: Tx error, status %2.2x.\n",
						 dev->name, tx_status);
#if 0
				if (tx_status & 0x04) vp->stats.tx_fifo_errors++;
				if (tx_status & 0x38) vp->stats.tx_aborted_errors++;
#endif
				if (tx_status & 0x30) {
					int j;
					lx_outw(TxReset, ioaddr + EL3_CMD);
					for (j = 20; j >= 0 ; j--)
						if ( ! (inw(ioaddr + EL3_STATUS) & CmdInProgress))
							break;
				}
				lx_outw(TxEnable, ioaddr + EL3_CMD);
			}
			lx_outb(0x00, ioaddr + TxStatus); /* Pop the status stack. */
		}
	}
	return 0;
}

#if 0

/* The interrupt handler does all of the Rx thread work and cleans up
   after the Tx thread. */
static void vortex_interrupt IRQ(int irq, void *dev_id, struct pt_regs *regs)
{
#ifdef SA_SHIRQ		/* Use the now-standard shared IRQ implementation. */
	struct device *dev = dev_id;
#else
#ifdef USE_SHARED_IRQ
	struct device *dev = (struct device *)(irq == 0 ? regs : irq2dev_map[irq]);
#else
	struct device *dev = (struct device *)(irq2dev_map[irq]);
#endif
#endif
	struct vortex_private *lp;
	int ioaddr, status;
	int latency;
	int i = 0;

	if (dev->interrupt)
		printf("%s: Re-entering the interrupt handler.\n", dev->name);
	dev->interrupt = 1;

	ioaddr = dev->base_addr;
	latency = inb(ioaddr + Timer);
	lp = (struct vortex_private *)dev->priv;

	status = inw(ioaddr + EL3_STATUS);

	if (vortex_debug > 4)
		printf("%s: interrupt, status %4.4x, timer %d.\n", dev->name,
			   status, latency);
	if ((status & 0xE000) != 0xE000) {
		static int donedidthis=0;
		/* Some interrupt controllers store a bogus interrupt from boot-time.
		   Ignore a single early interrupt, but don't hang the machine for
		   other interrupt problems. */
		if (donedidthis++ > 1) {
			printf("%s: Bogus interrupt, bailing. Status %4.4x, start=%d.\n",
				   dev->name, status, dev->start);
			FREE_IRQ(dev->irq, dev);
		}
	}

	do {
		if (vortex_debug > 5)
				printf("%s: In interrupt loop, status %4.4x.\n",
					   dev->name, status);
		if (status & RxComplete)
			vortex_rx(dev);

		if (status & TxAvailable) {
			if (vortex_debug > 5)
				printf("	TX room bit was handled.\n");
			/* There's room in the FIFO for a full-sized packet. */
			lx_outw(AckIntr | TxAvailable, ioaddr + EL3_CMD);
			dev->tbusy = 0;
			mark_bh(NET_BH);
		}
#ifdef VORTEX_BUS_MASTER
		if (status & DMADone) {
			lx_outw(0x1000, ioaddr + Wn7_MasterStatus); /* Ack the event. */
			dev->tbusy = 0;
			/* dev_kfree_skb (lp->tx_skb, FREE_WRITE); Release the transfered buffer */
/* 			mark_bh(NET_BH); */
		}
#endif
		if (status & (AdapterFailure | RxEarly | StatsFull)) {
			/* Handle all uncommon interrupts at once. */
			if (status & RxEarly) {				/* Rx early is unused. */
				vortex_rx(dev);
				lx_outw(AckIntr | RxEarly, ioaddr + EL3_CMD);
			}
			if (status & StatsFull) { 	/* Empty statistics. */
				static int DoneDidThat = 0;
				if (vortex_debug > 4)
					printf("%s: Updating stats.\n", dev->name);
				update_stats(ioaddr, dev);
				/* DEBUG HACK: Disable statistics as an interrupt source. */
				/* This occurs when we have the wrong media type! */
				if (DoneDidThat == 0  &&
					inw(ioaddr + EL3_STATUS) & StatsFull) {
					int win, reg;
					printf("%s: Updating stats failed, disabling stats as an"
						   " interrupt source.\n", dev->name);
					for (win = 0; win < 8; win++) {
						EL3WINDOW(win);
						printf("\n Vortex window %d:", win);
						for (reg = 0; reg < 16; reg++)
							printf(" %2.2x", inb(ioaddr+reg));
					}
					EL3WINDOW(7);
					lx_outw(SetIntrEnb | 0x18, ioaddr + EL3_CMD);
					DoneDidThat++;
				}
			}
			if (status & AdapterFailure) {
				/* Adapter failure requires Rx reset and reinit. */
				lx_outw(RxReset, ioaddr + EL3_CMD);
				/* Set the Rx filter to the current state. */
				set_rx_mode(dev);
				lx_outw(RxEnable, ioaddr + EL3_CMD); /* Re-enable the receiver. */
				lx_outw(AckIntr | AdapterFailure, ioaddr + EL3_CMD);
			}
		}

		if (++i > 10) {
			printf("%s: Infinite loop in interrupt, status %4.4x.  "
				   "Disabling functions (%4.4x).\n",
				   dev->name, status, SetStatusEnb | ((~status) & 0xFE));
			/* Disable all pending interrupts. */
			lx_outw(SetStatusEnb | ((~status) & 0xFE), ioaddr + EL3_CMD);
			lx_outw(AckIntr | 0xFF, ioaddr + EL3_CMD);
			break;
		}
		/* Acknowledge the IRQ. */
		lx_outw(AckIntr | IntReq | IntLatch, ioaddr + EL3_CMD);

	} while ((status = inw(ioaddr + EL3_STATUS)) & (IntLatch | RxComplete));

	if (vortex_debug > 4)
		printf("%s: exiting interrupt, status %4.4x.\n", dev->name, status);

	dev->interrupt = 0;
	return;
}

#endif

static int
vortex_rx(struct device *dev, char *data, int len)
{
	struct vortex_private *vp = (struct vortex_private *)dev->priv;
	int ioaddr = dev->base_addr;
	int i;
	short rx_status;

	if (vortex_debug > 5)
		printf("   In rx_packet(), status %4.4x, rx_status %4.4x.\n",
			   inw(ioaddr+EL3_STATUS), inw(ioaddr+RxStatus));
	while ((rx_status = inw(ioaddr + RxStatus)) > 0) {
		if (rx_status & 0x4000) { /* Error, update stats. */
			unsigned char rx_error = inb(ioaddr + RxErrors);
			if (vortex_debug > 4)
				printf(" Rx error: status %2.2x.\n", rx_error);
#if 0
			vp->stats.rx_errors++;
			if (rx_error & 0x01)  vp->stats.rx_over_errors++;
			if (rx_error & 0x02)  vp->stats.rx_length_errors++;
			if (rx_error & 0x04)  vp->stats.rx_frame_errors++;
			if (rx_error & 0x08)  vp->stats.rx_crc_errors++;
			if (rx_error & 0x10)  vp->stats.rx_length_errors++;
#endif
		} else {
			/* The packet length: up to 4.5K!. */
			short pkt_len = rx_status & 0x1fff;
/* 			struct sk_buff *skb; */

/* 			skb = DEV_ALLOC_SKB(pkt_len + 5); */
			if (vortex_debug > 4)
				printf("Receiving packet size %d status %4.4x.\n",
					   pkt_len, rx_status);

			if (1) {
			  insl(ioaddr + RX_FIFO, data, (pkt_len + 3) >> 2);
			  lx_outw(RxDiscard, ioaddr + EL3_CMD); /* Pop top Rx packet. */

			  for (i = 200; i >= 0; i--)
				if ( ! (inw(ioaddr + EL3_STATUS) & CmdInProgress))
				  break;
			}

#if 0
			if (skb != NULL) {
				skb->dev = dev;
#if defined (LINUX_VERSION_CODE) && LINUX_VERSION_CODE >= 0x10300
				skb_reserve(skb, 2);	/* Align IP on 16 byte boundaries */
				/* 'skb_put()' points to the start of sk_buff data area. */
				insl(ioaddr + RX_FIFO, skb_put(skb, pkt_len),
					 (pkt_len + 3) >> 2);
				skb->protocol = eth_type_trans(skb, dev);
#else
				skb->len = pkt_len;
				/* 'skb->data' points to the start of sk_buff data area. */
				insl(ioaddr + RX_FIFO, skb->data, (pkt_len + 3) >> 2);
#endif  /* KERNEL_1_3_0 */
				netif_rx(skb);
				lx_outw(RxDiscard, ioaddr + EL3_CMD); /* Pop top Rx packet. */
				/* Wait a limited time to go to next packet. */
				for (i = 200; i >= 0; i--)
					if ( ! (inw(ioaddr + EL3_STATUS) & CmdInProgress))
						break;
				vp->stats.rx_packets++;
				continue;
			} else if (vortex_debug)
				printf("%s: Couldn't allocate a sk_buff of size %d.\n",
					   dev->name, pkt_len);
#endif
		}
#if 0
		vp->stats.rx_dropped++;
#endif
		lx_outw(RxDiscard, ioaddr + EL3_CMD);
		/* Wait a limited time to skip this packet. */
		for (i = 200; i >= 0; i--)
			if ( ! (inw(ioaddr + EL3_STATUS) & CmdInProgress))
				break;
	}

	return 0;
}

static int
vortex_close(struct device *dev)
{
	struct vortex_private *vp = (struct vortex_private *)dev->priv;
	int ioaddr = dev->base_addr;

	dev->start = 0;
	dev->tbusy = 1;

	if (vortex_debug > 1)
		printf("%s: vortex_close() status %4.4x, Tx status %2.2x.\n",
			   dev->name, inw(ioaddr + EL3_STATUS), inb(ioaddr + TxStatus));

#if 0
	del_timer(&vp->timer);
#endif
	/* Turn off statistics ASAP.  We update lp->stats below. */
	lx_outw(StatsDisable, ioaddr + EL3_CMD);

	/* Disable the receiver and transmitter. */
	lx_outw(RxDisable, ioaddr + EL3_CMD);
	lx_outw(TxDisable, ioaddr + EL3_CMD);

	if (dev->if_port == 3)
		/* Turn off thinnet power.  Green! */
		lx_outw(StopCoax, ioaddr + EL3_CMD);
#if 0

#ifdef SA_SHIRQ
	FREE_IRQ(dev->irq, dev);
#else
#ifdef USE_SHARED_IRQ
	free_shared_irq(dev->irq, dev);
#else
	FREE_IRQ(dev->irq, NULL);
	/* Mmmm, we should disable all interrupt sources here. */
	irq2dev_map[dev->irq] = 0;
#endif
#endif

	update_stats(ioaddr, dev);
#ifdef MODULE
	MOD_DEC_USE_COUNT;
#endif

#endif
	return 0;
}


/* This new version of set_rx_mode() supports v1.4 kernels.
   The Vortex chip has no documented multicast filter, so the only
   multicast setting is to receive all multicast frames.  At least
   the chip has a very clean way to set the mode, unlike many others. */
static void
set_rx_mode(struct device *dev)
{
	short ioaddr = dev->base_addr;
	short new_mode;

	if (dev->flags & IFF_PROMISC) {
		if (vortex_debug > 3)
			printf("%s: Setting promiscuous mode.\n", dev->name);
		new_mode = SetRxFilter|RxStation|RxMulticast|RxBroadcast|RxProm;
	} else	if (/*(dev->mc_list)  || */ (dev->flags & IFF_ALLMULTI)) {
		new_mode = SetRxFilter|RxStation|RxMulticast|RxBroadcast;
	} else 
		new_mode = SetRxFilter | RxStation | RxBroadcast;

	lx_outw(new_mode, ioaddr + EL3_CMD);
}


/*
 * Local variables:
 *  compile-command: "gcc -DMODULE -D__KERNEL__ -I/usr/src/linux/net/inet -Wall -Wstrict-prototypes -O6 -m486 -c 3c59x.c -o ../../modules/3c59x.o"
 *  c-indent-level: 4
 *  tab-width: 4
 * End:
 */
void VX_reset (struct nic *nic)
{ 
  printf ("reset");
}

int VX_poll (struct nic *nic)
{
//  printf ("Polling %x %d\n", nic->packet, nic->packetlen);
}

void VX_transmit (struct nic *nic, char *d,
				unsigned int t, unsigned int s, char *p)
{
  int i;
 
  printf ("Transmit type %d, %x %d\n", t, p, s);
  printf ("SRCAddr : ");
  for (i = 0; i < 6; i++)
	printf ("%x:", ((int)nic->node_addr[i]) & 0xff);
  printf ("\nDest : ");
  for (i = 0; i < 6; i++)
	printf ("%x:", ((int)d[i]) & 0xff);
  printf ("\n");

}

void VX_disable (struct nic *nic)
{
  printf ("Disable\n");
}

struct nic *VX_probe(struct nic *nic, unsigned short *probe_addrs)
{

  if (probe_addrs) {
	card.mem_start = probe_addrs[0];
	vortex_probe1(&card);

	printf ("Probe 3c93 at %x\n", card.mem_start);
	nic->priv_data = &card;
	nic->reset = VX_reset;
	nic->poll = VX_poll;
	nic->transmit = VX_transmit;
	nic->disable = VX_disable;
	return 1;
  } 
  printf ("Failed : No ProbeAddrs\n");
  return 0;
}

#endif /* INCLUDE_3C59X */
