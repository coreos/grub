/*  
 * eepro100.c -- This file implements the eepro100 driver for etherboot.
 *
 *
 * Copyright (C) AW Computer Systems.
 * written by R.E.Wolff -- R.E.Wolff@BitWizard.nl
 *
 *
 * AW Computer Systems is contributing to the free software community
 * by paying for this driver and then putting the result under GPL.
 * 
 * If you need a Linux device driver, please contact BitWizard for a
 * quote. 
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 *              date       version  by   what
 *  Written:    May 29 1997  V0.10  REW  Initial revision.
 * changes:     May 31 1997  V0.90  REW  Works!
 *              Jun 1  1997  V0.91  REW  Cleanup
 *              Jun 2  1997  V0.92  REW  Add some code documentation
 *              Jul 25 1997  V1.00  REW  Tested by AW to work in a PROM
 *                                       Cleanup for publication
 *
 * This is the etherboot-3.1 compatible intel etherexpress Pro/100B 
 * driver. 
 * 
 * It was written from scratch, with Donald Beckers eepro100.c kernel
 * driver as a guideline. Mostly the 82557 related definitions and the
 * lower level routines have been cut-and-pasted into this source.
 * 
 * The driver was finished before Intel got the NDA out of the closet.
 * I still don't have the docs. 
 * */



/* Philosophy of this driver.
 * 
 * Probing: 
 *
 * Using a subset of "bios32" and "pci" functions of the linux kernel,
 * the pci 82557 chip is detected.
 *
 * 
 * Initialization: 
 *
 *
 * The chip is then initialized to "know" its ethernet address, and to
 * start recieving packets. The Linux driver has a whole transmit and
 * recieve ring of buffers. This is neat if you need high performance:
 * you can write the buffers asynchronously to the chip reading the
 * buffers and transmitting them over the network.  Performance is NOT
 * an issue here. We can boot a 400k kernel in about two
 * seconds. (Theory: 0.4 seconds). Booting a system is going to take
 * about half a minute anyway, so getting 10 times closer to the
 * theoretical limit is going to make a difference of a few percent.
 *
 *
 * Transmitting and recieving.
 *
 * We have only one transmit descriptor. It has two buffer descriptors:
 * one for the header, and the other for the data. 
 * We have only one receive buffer. The chip is told to recieve packets,
 * and suspend itself once it got one. The recieve (poll) routine simply
 * looks at the recieve buffer to see if there is already a packet there.
 * if there is, the buffer is copied, and the reciever is restarted. 
 * 
 * Caveats:
 *
 * The etherboot framework moves the code to the 32k segment from
 * 0x98000 to 0xa0000. There is just a little room between the end of
 * this driver and the 0xa0000 address. If you compile in too many
 * features, this will overflow.
 * The number under "hex" in the output of size that scrolls by while
 * compiling should be less than 8000. Maybe even the stack is up there,
 * so that you need even more headroom. 
 * 
 * If you run into trouble, the method used to give "pci.c" dynamic
 * allocation should be used to allocate the larger variables (like
 * the packet buffers)
 * */

#ifdef INCLUDE_EEPRO100


/* The etherboot authors seem to dislike the argument ordering in
 * outb macros that Linux uses. I disklike the confusion that this
 * has caused even more.... This file uses the Linux argument ordering.  */
#define LINUX_OUT_MACROS

#include "netboot.h"
#include "nic.h"
#include <linux/bios32.h>
#include <linux/pci.h>

static int ioaddr;

typedef unsigned char  u8;
typedef   signed char  s8;
typedef unsigned short u16;
typedef   signed short s16;
typedef unsigned int   u32;
typedef   signed int   s32;


enum speedo_offsets {
  SCBStatus = 0, SCBCmd = 2,      /* Rx/Command Unit command and status. */
  SCBPointer = 4,                 /* General purpose pointer. */
  SCBPort = 8,                    /* Misc. commands and operands.  */
  SCBflash = 12, SCBeeprom = 14,  /* EEPROM and flash memory control. */
  SCBCtrlMDI = 16,                /* MDI interface control. */
  SCBEarlyRx = 20,                /* Early receive byte count. */
};


static int read_eeprom(int ioaddr, int location);
static void udelay (int val);
void hd (void *where, int n);


/***********************************************************************/
/*                       I82557 related defines                        */
/***********************************************************************/

/* Serial EEPROM section.
   A "bit" grungy, but we work our way through bit-by-bit :->. */
/*  EEPROM_Ctrl bits. */
#define EE_SHIFT_CLK    0x01    /* EEPROM shift clock. */
#define EE_CS           0x02    /* EEPROM chip select. */
#define EE_DATA_WRITE   0x04    /* EEPROM chip data in. */
#define EE_WRITE_0      0x01
#define EE_WRITE_1      0x05
#define EE_DATA_READ    0x08    /* EEPROM chip data out. */
#define EE_ENB          (0x4800 | EE_CS)


/* Delay between EEPROM clock transitions.
   This is a "nasty" timing loop, but PC compatible machines are defined
   to delay an ISA compatible period for the SLOW_DOWN_IO macro.  */
#define eeprom_delay(nanosec)   do { int _i = 3; while (--_i > 0) \
                                     { __SLOW_DOWN_IO; }} while (0)

/* The EEPROM commands include the alway-set leading bit. */
#define EE_WRITE_CMD    (5 << 6)
#define EE_READ_CMD     (6 << 6)
#define EE_ERASE_CMD    (7 << 6)

/* The SCB accepts the following controls for the Tx and Rx units: */
#define  CU_START       0x0010
#define  CU_RESUME      0x0020
#define  CU_STATSADDR   0x0040
#define  CU_SHOWSTATS   0x0050  /* Dump statistics counters. */
#define  CU_CMD_BASE    0x0060  /* Base address to add to add CU commands. */
#define  CU_DUMPSTATS   0x0070  /* Dump then reset stats counters. */

#define  RX_START       0x0001
#define  RX_RESUME      0x0002
#define  RX_ABORT       0x0004
#define  RX_ADDR_LOAD   0x0006
#define  RX_RESUMENR    0x0007
#define INT_MASK        0x0100
#define DRVR_INT        0x0200          /* Driver generated interrupt. */

enum phy_chips { NonSuchPhy=0, I82553AB, I82553C, I82503, DP83840, S80C240,
                                         S80C24, PhyUndefined, DP83840A=10, };



/* Commands that can be put in a command list entry. */
enum commands {
  CmdNOp = 0, 
  CmdIASetup = 1, 
  CmdConfigure = 2, 
  CmdMulticastList = 3,
  CmdTx = 4, 
  CmdTDR = 5, 
  CmdDump = 6, 
  CmdDiagnose = 7,

  /* And some extra flags: */
  CmdSuspend = 0x4000,      /* Suspend after completion. */
  CmdIntr = 0x2000,         /* Interrupt after completion. */
  CmdTxFlex = 0x0008,       /* Use "Flexible mode" for CmdTx command. */
};


/* How to wait for the command unit to accept a command.
   Typically this takes 0 ticks. */
static inline void wait_for_cmd_done(int cmd_ioaddr)
{
  short wait = 100;
  do   ;
  while(inb(cmd_ioaddr) && --wait >= 0);
}


/* Elements of the dump_statistics block. This block must be lword aligned. */
struct speedo_stats {
        u32 tx_good_frames;
        u32 tx_coll16_errs;
        u32 tx_late_colls;
        u32 tx_underruns;
        u32 tx_lost_carrier;
        u32 tx_deferred;
        u32 tx_one_colls;
        u32 tx_multi_colls;
        u32 tx_total_colls;
        u32 rx_good_frames;
        u32 rx_crc_errs;
        u32 rx_align_errs;
        u32 rx_resource_errs;
        u32 rx_overrun_errs;
        u32 rx_colls_errs;
        u32 rx_runt_errs;
        u32 done_marker;
} lstats;


/* A speedo3 TX buffer descriptor with two buffers... */
struct TxFD {
  volatile s16 status;
  s16 command;
  u32 link;          /* void * */
  u32 tx_desc_addr;  /* (almost) Always points to the tx_buf_addr element. */
  s32 count;         /* # of TBD (=2), Tx start thresh., etc. */
                     /* This constitutes two "TBD" entries: hdr and data */
  u32 tx_buf_addr0;  /* void *, header of frame to be transmitted.  */
  s32 tx_buf_size0;  /* Length of Tx hdr. */
  u32 tx_buf_addr1;  /* void *, data to be transmitted.  */
  s32 tx_buf_size1;  /* Length of Tx data. */
} txfd; 



/* The Speedo3 Rx buffer descriptors. */
struct RxFD {               /* Receive frame descriptor. */
  volatile s16 status;
  s16 command;
  u32 link;                 /* struct RxFD * */
  u32 rx_buf_addr;          /* void * */
  u16 count;
  u16 size;
  char packet[1518];
} rxfd;



static int congenb = 0;         /* Enable congestion control in the DP83840. */
static int txfifo = 8;          /* Tx FIFO threshold in 4 byte units, 0-15 */
static int rxfifo = 8;          /* Rx FIFO threshold, default 32 bytes. */
static int txdmacount = 0;      /* Tx DMA burst length, 0-127, default 0. */
static int rxdmacount = 0;      /* Rx DMA length, 0 means no preemption. */



/* I don't understand a byte in this structure. It was copied from the
 * Linux kernel initialization for the eepro100. -- REW */
struct ConfCmd {
  s16 status;
  s16 command;
  u32 link;
  unsigned char data[22];
} confcmd = {
  0, CmdConfigure,
  (u32) & txfd, 
  {22, 0x08, 0, 0,  0, 0x80, 0x32, 0x03,  1, /* 1=Use MII  0=Use AUI */
   0, 0x2E, 0,  0x60, 0,
   0xf2, 0x48,   0, 0x40, 0xf2, 0x80,        /* 0x40=Force full-duplex */
   0x3f, 0x05, }
};



#define TIME_OUT 1000000

/* The "pci" code needs to allocate a few structures. It wants to
 * increment the "kernel memory ends here" pointer to allocate memory.
 * I guess that a machine with PCI has more than 2Mb of memory, so
 * that is where those things are put. Those structures don't survive
 * the "jump to kernel start".
 *   */
#define MEM_START 0x200000   /* Memory starts at 2Mb for now....  */
#define MEM_END   0

static unsigned short eeprom [0x40];

static void calibrate_delay (void);


/***********************************************************************/
/*                       Locally used functions                        */
/***********************************************************************/


/* Support function: mdio_write
 *
 * This probably writes to the "physical media interface chip". 
 * -- REW
 */


static int mdio_write(int ioaddr, int phy_id, int location, int value)
{
  int val, boguscnt = 64*4;         /* <64 usec. to complete, typ 27 ticks */

  outl(0x04000000 | (location<<16) | (phy_id<<21) | value,
       ioaddr + SCBCtrlMDI);
  do {
    udelay(16);

    val = inl(ioaddr + SCBCtrlMDI);
    if (--boguscnt < 0) {
      printf(" mdio_write() timed out with val = %8.8x.\n", val);
    }
  } while (! (val & 0x10000000));
  return val & 0xffff;
}


/* Support function: mdio_read
 *
 * This probably reads a register in the "physical media interface chip". 
 * -- REW
 */
static int mdio_read(int ioaddr, int phy_id, int location)
{
  int val, boguscnt = 64*4;               /* <64 usec. to complete, typ 27 ticks */
  outl(0x08000000 | (location<<16) | (phy_id<<21), ioaddr + SCBCtrlMDI);
  do {
    udelay(16);

    val = inl(ioaddr + SCBCtrlMDI);
    if (--boguscnt < 0) {
      printf( " mdio_read() timed out with val = %8.8x.\n", val);
    }
  } while (! (val & 0x10000000));
  return val & 0xffff;
}


/* Support function: read_eeprom
 * reads a value from the eeprom at a specified location.
 * Arguments: ioaddr:    address of the 82557 chip
 *            location:  address of the location to read from the eeprom.
 * returns:   value read from eeprom at location.
 */
static int read_eeprom(int ioaddr, int location)
{
        int i;
        unsigned short retval = 0;
        int ee_addr = ioaddr + SCBeeprom;
        int read_cmd = location | EE_READ_CMD;
        
        outw(EE_ENB & ~EE_CS, ee_addr);
        outw(EE_ENB, ee_addr);
        
        /* Shift the read command bits out. */
        for (i = 10; i >= 0; i--) {
                short dataval = (read_cmd & (1 << i)) ? EE_DATA_WRITE : 0;
                outw(EE_ENB | dataval, ee_addr);
                eeprom_delay(100);
                outw(EE_ENB | dataval | EE_SHIFT_CLK, ee_addr);
                eeprom_delay(150);
                outw(EE_ENB | dataval, ee_addr);        /* Finish EEPROM a clock tick. */
                eeprom_delay(250);
        }
        outw(EE_ENB, ee_addr);
        
        for (i = 15; i >= 0; i--) {
                outw(EE_ENB | EE_SHIFT_CLK, ee_addr);
                eeprom_delay(100);
                retval = (retval << 1) | ((inw(ee_addr) & EE_DATA_READ) ? 1 : 0);
                outw(EE_ENB, ee_addr);
                eeprom_delay(100);
        }

        /* Terminate the EEPROM access. */
        outw(EE_ENB & ~EE_CS, ee_addr);
        return retval;
}


static void inline whereami (char *str)
{
#if 0
  printf ("%s\r\n", str);
  sleep (2);
#endif
}



/***********************************************************************/
/*                    Externally visible functions                     */
/***********************************************************************/


/* function: eepro100_reset / eth_reset
 * resets the card. This is used to allow Linux to probe the card again
 * from a "virginal" state.... 
 * Arguments: none
 *            
 * returns:   void.
 */

static void eepro100_reset(struct nic *nic)
{
  outl(0, ioaddr + SCBPort);
}



/* function: eepro100_transmit / eth_transmit
 * This transmits a packet.
 *
 * Arguments: char d[6]:          destination ethernet address.
 *            unsigned short t:   ethernet protocol type.
 *            unsigned short s:   size of the data-part of the packet.
 *            char *p:            the data for the packet.
 * returns:   void.
 */

static void eepro100_transmit(struct nic *nic, char *d, unsigned int t, unsigned int s, char *p)
{
  struct eth_hdr {
    unsigned char dst_addr[6];
    unsigned char src_addr[6];
    unsigned short type;
  } hdr;
  unsigned short status;
  int to;
  int s1, s2;
  
  status = inw(ioaddr + SCBStatus);
  /* Acknowledge all of the current interrupt sources ASAP. */
  outw(status & 0xfc00, ioaddr + SCBStatus);
  
#ifdef DEBUG
  printf ("transmitting type %x packet (%d bytes). status = %x, cmd=%x\r\n", 
	  t, s, status, inw (ioaddr + SCBCmd));
#endif


  memcpy (&hdr.dst_addr, d, 6);
  memcpy (&hdr.src_addr, &arptable[ARP_CLIENT].node, 6);

  hdr.type = htons (t);

  txfd.status = 0;
  txfd.command = CmdSuspend | CmdTx | CmdTxFlex;
  txfd.link   = virt_to_bus (&txfd);
  txfd.count   = 0x02208000;
  txfd.tx_desc_addr = (u32)&txfd.tx_buf_addr0;

  txfd.tx_buf_addr0 = virt_to_bus (&hdr);
  txfd.tx_buf_size0 = sizeof (hdr);

  txfd.tx_buf_addr1 = virt_to_bus (p);
  txfd.tx_buf_size1 = s;

#ifdef DEBUG
  printf ("txfd: \r\n");
  hd (&txfd, sizeof (txfd));
#endif

  outl(virt_to_bus(&txfd), ioaddr + SCBPointer);
  outw(INT_MASK | CU_START, ioaddr + SCBCmd);
  wait_for_cmd_done(ioaddr + SCBCmd);

  s1 = inw (ioaddr + SCBStatus);
  to = TIME_OUT;
  while (!txfd.status && --to)
    /* Wait */;
  s2 = inw (ioaddr + SCBStatus);

#ifdef DEBUG 
  printf ("Tx: Loop executed %d times.\r\n", TIME_OUT-to);
  printf ("s1 = %x, s2 = %x.\r\n", s1, s2);
#endif
}


/* function: eepro100_poll / eth_poll
 * This recieves a packet from the network. 
 *
 * Arguments: none
 *            
 * returns:   1 if a packet was recieved. 
 *            0 if no pacet was recieved.
 * side effects:
 *            returns the packet in the array nic->packet.
 *            returns the length of the packet in nic->packetlen.
 */

static int eepro100_poll(struct nic *nic)
{
  int to;

  to = TIME_OUT;
  while (!rxfd.status && --to)
    /* Wait */;

  /* Ok. We got a packet. Now restart the reciever.... */
  rxfd.status = 0;
  rxfd.command = 0xc000;
  outl(virt_to_bus(&rxfd), ioaddr + SCBPointer);
  outw(INT_MASK | RX_START, ioaddr + SCBCmd);
  wait_for_cmd_done(ioaddr + SCBCmd);

  if (to) {
#ifdef DEBUG
    printf ("Got a packet: Len = %d.\r\n", rxfd.count & 0x3fff);
#endif
    nic->packetlen =  rxfd.count & 0x3fff;
    bcopy (rxfd.packet, nic->packet, sizeof (rxfd.packet));
#ifdef DEBUG
    hd (nic->packet, 0x30);
#endif
    return 1;
  } else
    return 0;
}

static void eepro100_disable(struct nic *nic)
{
}

/* exported function: eepro100_probe / eth_probe
 * initializes a card
 *            
 * side effects: 
 *            leaves the ioaddress of the 82557 chip in the variable ioaddr.
 *            leaves the 82557 initialized, and ready to recieve packets.
 */

struct nic *eepro100_probe(struct nic *nic, unsigned short *probeaddrs)
{
  int pci_index;
  u16 sum = 0;
  int i, j, to;
  unsigned short value;
  int options;
  int promisc;

  if (probeaddrs == 0 || probeaddrs[0] == 0)
          return 0;

  ioaddr = probeaddrs[0] & ~3; /* Mask the bit that says "this is an io addr" */

  /* Ok. Got one. Read the eeprom. */
  for (j = 0, i = 0; i < 0x40; i++) {
	value = read_eeprom(ioaddr, i);
	eeprom[i] = value;
	sum += value;
  }
  
  printf ("Ethernet addr: ");
  for (i=0;i<6;i++) {
	arptable[ARP_CLIENT].node[i] =  (eeprom[i/2] >> (8*(i&1))) & 0xff;
	printf ("%b%c", arptable[ARP_CLIENT].node[i] , i < 5?':':' '); 
  }
  printf ("\r\n");

  if (sum != 0xBABA)
	printf("eepro100: Invalid EEPROM checksum %#4.4x, "
	       "check settings before activating this device!\r\n", sum);
  outl(0, ioaddr + SCBPort);
  udelay (10);

  whereami ("Got eeprom.");

  outl(virt_to_bus(&lstats), ioaddr + SCBPointer);
  outw(INT_MASK | CU_STATSADDR, ioaddr + SCBCmd);
  wait_for_cmd_done(ioaddr + SCBCmd);
  
  whereami ("set stats addr.");
  /* INIT RX stuff. */
  
  /* Base = 0 */
  outl(0, ioaddr + SCBPointer);
  outw(INT_MASK | RX_ADDR_LOAD, ioaddr + SCBCmd);
  wait_for_cmd_done(ioaddr + SCBCmd);

  whereami ("set rx base addr.");
  
  rxfd.status  = 0x0001;
  rxfd.command = 0x0000;
  rxfd.link    = virt_to_bus(&rxfd);
  rxfd.rx_buf_addr = (int) &nic->packet;
  rxfd.count   = 0;
  rxfd.size    = 1528;
  
  outl(virt_to_bus(&rxfd), ioaddr + SCBPointer);
  outw(INT_MASK | RX_START, ioaddr + SCBCmd);
  wait_for_cmd_done(ioaddr + SCBCmd);

  whereami ("started RX process.");

  /* Start the reciever.... */
  rxfd.status = 0;
  rxfd.command = 0xc000;
  outl(virt_to_bus(&rxfd), ioaddr + SCBPointer);
  outw(INT_MASK | RX_START, ioaddr + SCBCmd);

  
  /* INIT TX stuff. */

  /* Base = 0 */
  outl(0, ioaddr + SCBPointer);
  outw(INT_MASK | CU_CMD_BASE, ioaddr + SCBCmd);
  wait_for_cmd_done(ioaddr + SCBCmd);

  whereami ("set TX base addr.");

  txfd.command      = (CmdIASetup);
  txfd.status       = 0x0000;
  txfd.link         = virt_to_bus (&confcmd);
  
  { 
	char *p = (char *)&txfd.tx_desc_addr;

	for (i=0;i<6;i++)
	p[i] = arptable[ARP_CLIENT].node[i];
  }
  
#ifdef DEBUG
  printf ("Setup_eaddr:\r\n");
  hd (&txfd, 0x20);
#endif
  /*      options = 0x40; */ /* 10mbps half duplex... */
  options = 0x00;            /* Autosense */

  promisc = 0;
  
  printf ("eeprom[6] = %x \r\n", eeprom[6]);
  if (   ((eeprom[6]>>8) & 0x3f) == DP83840
	  || ((eeprom[6]>>8) & 0x3f) == DP83840A) {
	int mdi_reg23 = mdio_read(ioaddr, eeprom[6] & 0x1f, 23) | 0x0422;
	if (congenb)
	  mdi_reg23 |= 0x0100;
	printf("  DP83840 specific setup, setting register 23 to %x.\r\n",
	       mdi_reg23);
	mdio_write(ioaddr, eeprom[6] & 0x1f, 23, mdi_reg23);
  }
  whereami ("Done DP8340 special setup.");
  if (options != 0) {
	mdio_write(ioaddr, eeprom[6] & 0x1f, 0,
		   ((options & 0x20) ? 0x2000 : 0) |    /* 100mbps? */
		   ((options & 0x10) ? 0x0100 : 0)); /* Full duplex? */
	whereami ("set mdio_register.");
  }

  confcmd.command  = CmdSuspend | CmdConfigure;
  confcmd.status   = 0x0000;
  confcmd.link     = virt_to_bus (&txfd);
  confcmd.data[1]  = (txfifo << 4) | rxfifo;
  confcmd.data[4]  = rxdmacount;
  confcmd.data[5]  = txdmacount + 0x80;
  confcmd.data[15] = promisc ? 0x49: 0x48;
  confcmd.data[19] = (options & 0x10) ? 0xC0 : 0x80;
  confcmd.data[21] = promisc ? 0x0D: 0x05;

  outl(virt_to_bus(&txfd), ioaddr + SCBPointer);
  outw(INT_MASK | CU_START, ioaddr + SCBCmd);
  wait_for_cmd_done(ioaddr + SCBCmd);

  whereami ("started TX thingy (config, iasetup).");

  to = TIME_OUT;
  while (!txfd.status && --to)
	/* Wait */;

#ifdef DEBUG 
  printf ("\r\nLoop executed %d times.\r\n", TIME_OUT-to);
#endif
  nic->reset = eepro100_reset;
  nic->poll = eepro100_poll;
  nic->transmit = eepro100_transmit;
  nic->disable = eepro100_disable;
  return nic;
}

static int loops_per_sec;
static int loops_per_usec;

static void udelay (int val)
{
  int c;

  for(c=0; c<val*loops_per_usec; c++) {
    /* Nothing */;
  }
}


#if 1

/* I don't have the possibility to test the below code. Murphey tells me
 * it won't work.
 */

static void calibrate_delay (void)
{
  loops_per_usec = 500;
}
#else


#define __delay udelay
#define jiffies currticks()
#define HZ 18

#define LPS_PREC 8

/* Copied from /usr/src/linux/init/main.c */
/* with just a few modifications. */
void calibrate_delay (void)
{
  int ticks;
  int loopbit;
  int lps_precision = LPS_PREC;

  loops_per_sec = (1<<12);
  loops_per_usec = 1;

  printk("Calibrating delay loop.. ");
  while (loops_per_sec <<= 1) {
    /* wait for "start of" clock tick */
    ticks = jiffies;
    while (ticks == jiffies)
      /* nothing */;
    /* Go .. */
    ticks = jiffies;
    __delay(loops_per_sec);
    ticks = jiffies - ticks;
    if (ticks)
      break;
  }

  /* Do a binary approximation to get loops_per_second set to equal one clock
     (up to lps_precision bits) */
  loops_per_sec >>= 1;
  loopbit = loops_per_sec;
  while ( lps_precision-- && (loopbit >>= 1) ) {
    loops_per_sec |= loopbit;
    ticks = jiffies;
    while (ticks == jiffies);
    ticks = jiffies;
    __delay(loops_per_sec);
    if (jiffies != ticks)   /* longer than 1 tick */
      loops_per_sec &= ~loopbit;
  }

  /* finally, adjust loops per second in terms of seconds instead of clocks */    
  loops_per_sec *= HZ;
  /* Round the value and print it */      
  printk("ok - %d.%d BogoMIPS\n",
	 (loops_per_sec+25000)/500000,
	 ((loops_per_sec+25000)/50000) % 10);
  loops_per_usec = loops_per_sec / 1000000;
}
#endif


/*********************************************************************/

#ifdef DEBUG

/* Hexdump a number of bytes from memory... */
void hd (void *where, int n)
{
  int i;

  while (n > 0) {
    printf ("%X ", where);
    for (i=0;i < ( (n>16)?16:n);i++)
      printf (" %b", ((char *)where)[i]);
    printf ("\n\r");
    n -= 16;
    where += 16;
  }
}
#endif

#endif /* INCLUDE_EEPRO100 */
