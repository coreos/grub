/* rtl8139.c - etherboot driver for the Realtek 8139 chipset

  ported from the linux driver written by Donald Becker
  by Rainer Bawidamann (Rainer.Bawidamann@informatik.uni-ulm.de) 1999

  This software may be used and distributed according to the terms
  of the GNU Public License, incorporated herein by reference.

  changes to the original driver:
  - removed support for interrupts, switchung to polling mode (yuck!)
  - removed support for the 8129 chip (external MII)
    and the 8139 with ids 0x1113/0x1211 (should be easy to add)

*/

/*********************************************************************/
/* Revision History                                                  */
/*********************************************************************/

/*

  18 Jan 2000   mdc@thinguin.org (Marty Connor)
     Drastically simplified error handling.  Basically, if any error
     in transmission or reception occurs, the card is reset.
     Also, pointed all transmit descriptors to the same buffer to
     save buffer space.  This should decrease driver size and avoid
     corruption because of exceeding 32K during runtime.

  28 Jul 1999   (Matthias Meixner - meixner@rbg.informatik.tu-darmstadt.de)
     rtl_poll was quite broken: it used the RxOK interrupt flag instead
     of the RxBufferEmpty flag which often resulted in very bad
     transmission performace - below 1kBytes/s.

*/

/* to get some global routines like printf */
#include "etherboot.h"
/* to get the interface to the body of the program */
#include "nic.h"
/* we have a PIC card */
#include "pci.h"

/* make things easier: the "linux kernel types" */
#define u16 unsigned short
#define u32 unsigned long
#define u8 unsigned char

/* PCI Tuning Parameters
   Threshold is bytes transferred to chip before transmission starts. */
#define TX_FIFO_THRESH 256      /* In bytes, rounded down to 32 byte units. */
#define RX_FIFO_THRESH  4       /* Rx buffer level before first PCI xfer.  */
#define RX_DMA_BURST    4       /* Maximum PCI burst, '4' is 256 bytes */
#define TX_DMA_BURST    4       /* Calculate as 16<<val. */
#define NUM_TX_DESC     4       /* Number of Tx descriptor registers. */
#define TX_BUF_SIZE 1536
#define RX_BUF_LEN_IDX 0
#define RX_BUF_LEN (8192 << RX_BUF_LEN_IDX)
#define TIME_OUT 4000000

#undef DEBUG_TX
#undef DEBUG_RX

/* Symbolic offsets to registers. */
enum RTL8129_registers {
	MAC0=0,			/* Ethernet hardware address. */
	MAR0=8,			/* Multicast filter. */
	TxStatus0=0x10,		/* Transmit status (Four 32bit registers). */
	TxAddr0=0x20,		/* Tx descriptors (also four 32bit). */
	RxBuf=0x30, RxEarlyCnt=0x34, RxEarlyStatus=0x36,
	ChipCmd=0x37, RxBufPtr=0x38, RxBufAddr=0x3A,
	IntrMask=0x3C, IntrStatus=0x3E,
	TxConfig=0x40, RxConfig=0x44,
	Timer=0x48,		/* A general-purpose counter. */
	RxMissed=0x4C,		/* 24 bits valid, write clears. */
	Cfg9346=0x50, Config0=0x51, Config1=0x52,
	FlashReg=0x54, GPPinData=0x58, GPPinDir=0x59, MII_SMI=0x5A, HltClk=0x5B,
	MultiIntr=0x5C, TxSummary=0x60,
	MII_BMCR=0x62, MII_BMSR=0x64, NWayAdvert=0x66, NWayLPAR=0x68,
	NWayExpansion=0x6A,
	/* Undocumented registers, but required for proper operation. */
	FIFOTMS=0x70, /* FIFO Test Mode Select */
	CSCR=0x74,    /* Chip Status and Configuration Register. */
	PARA78=0x78, PARA7c=0x7c,     /* Magic transceiver parameter register. */
};

enum ChipCmdBits {
	CmdReset=0x10, CmdRxEnb=0x08, CmdTxEnb=0x04, RxBufEmpty=0x01, };

/* Interrupt register bits, using my own meaningful names. */
enum IntrStatusBits {
	PCIErr=0x8000, PCSTimeout=0x4000,
	RxFIFOOver=0x40, RxUnderrun=0x20, RxOverflow=0x10,
	TxErr=0x08, TxOK=0x04, RxErr=0x02, RxOK=0x01,
};
enum TxStatusBits {
	TxHostOwns=0x2000, TxUnderrun=0x4000, TxStatOK=0x8000,
	TxOutOfWindow=0x20000000, TxAborted=0x40000000, TxCarrierLost=0x80000000,
};
enum RxStatusBits {
	RxMulticast=0x8000, RxPhysical=0x4000, RxBroadcast=0x2000,
	RxBadSymbol=0x0020, RxRunt=0x0010, RxTooLong=0x0008, RxCRCErr=0x0004,
	RxBadAlign=0x0002, RxStatusOK=0x0001,
};

enum CSCRBits {
	CSCR_LinkOKBit=0x0400, CSCR_LinkChangeBit=0x0800,
	CSCR_LinkStatusBits=0x0f000, CSCR_LinkDownOffCmd=0x003c0,
	CSCR_LinkDownCmd=0x0f3c0,
};

/* Bits in RxConfig. */
enum rx_mode_bits {
	RxCfgWrap=0x80,
	AcceptErr=0x20, AcceptRunt=0x10, AcceptBroadcast=0x08,
	AcceptMulticast=0x04, AcceptMyPhys=0x02, AcceptAllPhys=0x01,
};

/* Twister tuning parameters from RealTek.  Completely undocumented. */
/* and completly unused (RB) */
unsigned long param[4][4]={
	{0x0cb39de43,0x0cb39ce43,0x0fb38de03,0x0cb38de43},
	{0x0cb39de43,0x0cb39ce43,0x0cb39ce83,0x0cb39ce83},
	{0x0cb39de43,0x0cb39ce43,0x0cb39ce83,0x0cb39ce83},
	{0x0bb39de43,0x0bb39ce43,0x0bb39ce83,0x0bb39ce83}
};

/* I'm a bit unsure how to align correctly with gcc. this worked: */
static int ioaddr;
unsigned int cur_rx=0,cur_tx=0,tx_flag=0;
static unsigned char *tx_buf[NUM_TX_DESC]
__attribute__ ((aligned(4)));
static struct my {
	long dummy; /* align */
	unsigned char tx_bufs[TX_BUF_SIZE + 4];
	unsigned char rx_ring[RX_BUF_LEN + 16];
} my __attribute__ ((aligned(4)));
unsigned int full_duplex=0;

static void rtl_reset(struct nic *nic);
struct nic *rtl8139_probe(struct nic *nic, unsigned short *probeaddrs,
	struct pci_device *pci);
static int read_eeprom(long ioaddr, int location);
static void rtl_open(struct nic* nic);
static void rtl8129_init_ring();
static void rtl_transmit(struct nic *nic, char *destaddr,
	unsigned int type, unsigned int len, char *data);
static int rtl_poll(struct nic *nic);
static void rtl_disable(struct nic*);

static void rtl_reset(struct nic *nic)
{
	rtl_open(nic);
}

struct nic *rtl8139_probe(struct nic *nic, unsigned short *probeaddrs,
	struct pci_device *pci)
{
	int i;
	struct pci_device *p;

	printf("RTL8139 driver for Etherboot-4.2 ");
	if (probeaddrs == 0 || probeaddrs[0] == 0) {
		printf("ERROR: no probeaddrs given, using pci_device\n");
		for (p = pci; p; p++) {
			if ( ( (p->vendor == PCI_VENDOR_ID_REALTEK)
			    && (p->dev_id == PCI_DEVICE_ID_REALTEK_8139) )
			  || ( (p->vendor == PCI_VENDOR_ID_SMC_1211)
			    && (p->dev_id == PCI_DEVICE_ID_SMC_1211) ) ) {
				probeaddrs[0] = p->ioaddr;
				printf("rtl8139: probing %x (membase would be %x)\n",
					p->ioaddr, p->membase);
			}
		}
		return 0;
	}

	/* Mask the bit that says "this is an io addr" */
	ioaddr = probeaddrs[0] & ~3;

	/* Bring the chip out of low-power mode. */
	outb(0x00, ioaddr + Config1);

	if (read_eeprom(ioaddr, 0) != 0xffff) {
		unsigned short *ap = (unsigned short*)nic->node_addr;
		for (i = 0; i < 3; i++)
			*ap++ = read_eeprom(ioaddr, i + 7);
	} else {
		unsigned char *ap = (unsigned char*)nic->node_addr;
		for (i = 0; i < 6; i++)
			*ap++ = inb(ioaddr + MAC0 + i);
	}

	printf(" I/O %x ", ioaddr);

	for (i = 0; i < 6; i++)
		printf ("%b%c", nic->node_addr[i] , i < 5 ?':':' ');

	rtl_reset(nic);

	nic->reset = rtl_reset;
	nic->poll = rtl_poll;
	nic->transmit = rtl_transmit;
	nic->disable = rtl_disable;

	return nic;
}

/* Serial EEPROM section. */

/*  EEPROM_Ctrl bits. */
#define EE_SHIFT_CLK    0x04    /* EEPROM shift clock. */
#define EE_CS           0x08    /* EEPROM chip select. */
#define EE_DATA_WRITE   0x02    /* EEPROM chip data in. */
#define EE_WRITE_0      0x00
#define EE_WRITE_1      0x02
#define EE_DATA_READ    0x01    /* EEPROM chip data out. */
#define EE_ENB          (0x80 | EE_CS)

/*
	Delay between EEPROM clock transitions.
	No extra delay is needed with 33Mhz PCI, but 66Mhz may change this.
*/

#define eeprom_delay()  inl(ee_addr)

/* The EEPROM commands include the alway-set leading bit. */
#define EE_WRITE_CMD    (5 << 6)
#define EE_READ_CMD     (6 << 6)
#define EE_ERASE_CMD    (7 << 6)

static int read_eeprom(long ioaddr, int location)
{
	int i;
	unsigned retval = 0;
	long ee_addr = ioaddr + Cfg9346;
	int read_cmd = location | EE_READ_CMD;

	outb(EE_ENB & ~EE_CS, ee_addr);
	outb(EE_ENB, ee_addr);

	/* Shift the read command bits out. */
	for (i = 10; i >= 0; i--) {
		int dataval = (read_cmd & (1 << i)) ? EE_DATA_WRITE : 0;
		outb(EE_ENB | dataval, ee_addr);
		eeprom_delay();
		outb(EE_ENB | dataval | EE_SHIFT_CLK, ee_addr);
		eeprom_delay();
	}
	outb(EE_ENB, ee_addr);
	eeprom_delay();

	for (i = 16; i > 0; i--) {
		outb(EE_ENB | EE_SHIFT_CLK, ee_addr);
		eeprom_delay();
		retval = (retval << 1) | ((inb(ee_addr) & EE_DATA_READ) ? 1 : 0);
		outb(EE_ENB, ee_addr);
		eeprom_delay();
	}

	/* Terminate the EEPROM access. */
	outb(~EE_CS, ee_addr);
	return retval;
}

static void rtl_open(struct nic* nic)
{
	int i;

	outb(CmdReset, ioaddr + ChipCmd);

	rtl8129_init_ring();

	/* Check that the chip has finished the reset. */
	for (i = 1000; i > 0; i--)
		if ((inb(ioaddr + ChipCmd) & CmdReset) == 0)
			break;

	for (i = 0; i < 6; i++)
		outb(nic->node_addr[i], ioaddr + MAC0 + i);

	/* Must enable Tx/Rx before setting transfer thresholds! */
	outb(CmdRxEnb | CmdTxEnb, ioaddr + ChipCmd);
	outl((RX_FIFO_THRESH << 13) | (RX_BUF_LEN_IDX << 11) | (RX_DMA_BURST<<8),
		ioaddr + RxConfig);
	outl((TX_DMA_BURST<<8)|0x03000000, ioaddr + TxConfig);
	tx_flag = (TX_FIFO_THRESH<<11) & 0x003f0000;

	outb(0xC0, ioaddr + Cfg9346);
	outb(0x20, ioaddr + Config1); /* half-duplex */
	outb(0x00, ioaddr + Cfg9346);

#ifdef DEBUG_RX
	printf("rx start address is %X\n",(unsigned long) my.rx_ring);
#endif
	outl((unsigned long)my.rx_ring, ioaddr + RxBuf);

	/* Start the chip's Tx and Rx process. */
	outl(0, ioaddr + RxMissed);
	/* set_rx_mode */
	outb(AcceptBroadcast|AcceptMulticast|AcceptMyPhys, ioaddr + RxConfig);
	outl(0xFFFFFFFF, ioaddr + MAR0 + 0);
	outl(0xFFFFFFFF, ioaddr + MAR0 + 4);
	outb(CmdRxEnb | CmdTxEnb, ioaddr + ChipCmd);

	/* Disable all known interrupts by setting the interrupt mask. */
	outw(0, ioaddr + IntrMask);
}

/* Initialize the Rx and Tx rings, along with various 'dev' bits. */
static void rtl8129_init_ring()
{
	int i;

	cur_rx = 0;
	cur_tx = 0;

	for (i = 0; i < NUM_TX_DESC; i++) {
		tx_buf[i] = my.tx_bufs;
	}
}

static void rtl_transmit(struct nic *nic, char *destaddr,
	unsigned int type, unsigned int len, char *data)
{
	unsigned int i, status=0, to, nstype;
	unsigned long txstatus;
	unsigned char *txp = tx_buf[cur_tx];

	memcpy(txp, destaddr, ETHER_ADDR_SIZE);
	memcpy(txp + ETHER_ADDR_SIZE, nic->node_addr, ETHER_ADDR_SIZE);
	nstype = htons(type);
	memcpy(txp + 2 * ETHER_ADDR_SIZE, (char*)&nstype, 2);
	memcpy(txp + ETHER_HDR_SIZE, data, len);

	len += ETHER_HDR_SIZE;
#ifdef DEBUG_TX
	printf("sending %d bytes ethtype %x\n", len, type);
#endif
	outl((unsigned long) txp, ioaddr + TxAddr0 + cur_tx*4);
	/* Note: the chip doesn't have auto-pad! */
	/* 60 = ETH_ZLEN */
	outl(tx_flag | (len >= 60 ? len : 60),
		ioaddr + TxStatus0 + cur_tx*4);

	to = TIME_OUT;

	do {
		status = inw(ioaddr + IntrStatus);
		outw(status, ioaddr + IntrStatus);
		if ((status & (TxOK | TxErr | PCIErr)) != 0) break;
	} while (--to);

	txstatus = inl(ioaddr+ TxStatus0 + cur_tx*4);

	if (status & TxOK) {
		cur_tx = ++cur_tx % NUM_TX_DESC;
#ifdef DEBUG_TX
		printf("tx done (%d loops), status %x txstatus %X\n",
			TIME_OUT-to, status, txstatus);
#endif
		return;

	} else {
#ifdef DEBUG_TX
		printf("tx ERROR (%d loops), status %x txstatus %X\n",
			TIME_OUT-to, status, txstatus);
		printf("tx TIME-OUT, status %x\n", status);
#endif

		rtl_reset(nic);
		return;

	}
}

static int rtl_poll(struct nic *nic)
{
	unsigned int status=0;
	unsigned int ring_offset, rx_size, rx_status;

	status = inw(ioaddr + IntrStatus);
	outw(status, ioaddr + IntrStatus);

	if((inb(ioaddr + ChipCmd) & RxBufEmpty) && !(status & RxErr))
		return 0;

	if( status & RxErr ) {
		rtl_reset(nic);
		return 0;
	}

	ring_offset = cur_rx % RX_BUF_LEN;
	rx_status = *(unsigned int*)(my.rx_ring + ring_offset);
	rx_size = rx_status >> 16;

	if (rx_status & RxStatusOK) {
		nic->packetlen = rx_size;
		if (ring_offset+rx_size+4 > RX_BUF_LEN) {
			int semi_count = RX_BUF_LEN - ring_offset - 4;
			memcpy(nic->packet, (char*)&my.rx_ring[ring_offset+4], semi_count);
			memcpy(nic->packet+semi_count, my.rx_ring, rx_size-semi_count);
#ifdef DEBUG_RX
			printf("rtl_poll: rx packet %d + %d bytes\n", semi_count,rx_size-semi_count);
#endif
		} else {
			memcpy(nic->packet, (char*) &my.rx_ring[ring_offset+4], nic->packetlen);
#ifdef DEBUG_RX
			printf("rtl_poll: rx packet %d bytes at %X type %b%b status %x rxstatus %X\n",
				rx_size, (unsigned long) (my.rx_ring+ring_offset+4),
				my.rx_ring[ring_offset+4+12], my.rx_ring[ring_offset+4+13],
				status, rx_status);
#endif
		}
		cur_rx = (cur_rx + rx_size + 4 + 3) & ~3;
		outw(cur_rx - 16, ioaddr + RxBufPtr);
		return 1;

	} else {

		printf("rtl_poll: rx error status %x\n",status);
		rtl_reset(nic);
		return 0;

	}
}

static void rtl_disable(struct nic *nic)
{
}
