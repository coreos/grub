
/**************************************************************************
NETBOOT -  BOOTP/TFTP Bootstrap Program

Author: Martin Renters
  Date: May/94

 This code is based heavily on David Greenman's if_ed.c driver

 Copyright (C) 1993-1994, David Greenman, Martin Renters.
  This software may be used, modified, copied, distributed, and sold, in
  both source and binary form provided that the above copyright and these
  terms are retained. Under no circumstances are the authors responsible for
  the proper functioning of this software, nor do the authors assume any
  responsibility for damages incurred with its use.

3c503 support added by Bill Paul (wpaul@ctr.columbia.edu) on 11/15/94
SMC8416 support added by Bill Paul (wpaul@ctr.columbia.edu) on 12/25/94

**************************************************************************/

#if defined(INCLUDE_NE) || defined(INCLUDE_NEPCI) || defined(INCLUDE_WD) || defined(INCLUDE_T503)

#include "netboot.h"
#include "nic.h"
#include "ns8390.h"

static unsigned char	eth_vendor, eth_flags, eth_laar;
static unsigned short	eth_nic_base, eth_asic_base;
static unsigned char	eth_memsize, eth_rx_start, eth_tx_start;
static Address		eth_bmem, eth_rmem;

#if	defined(INCLUDE_WD)
#define	eth_probe	wd_probe
#else
#if	defined(INCLUDE_T503)
#define	eth_probe	t503_probe
#else
#if	defined(INCLUDE_NE)
#define	eth_probe	ne_probe
#else
#if	defined(INCLUDE_NEPCI)
#define	eth_probe	nepci_probe
#else
Error you must define one of the above
#endif
#endif
#endif
#endif



static void net_bcopy (char *s, char *d, int l)
{
	while (l--)
		*d++ = *s++;
}


#if	defined(INCLUDE_NE) || defined(INCLUDE_NEPCI)
/**************************************************************************
ETH_PIO_READ - Read a frame via Programmed I/O
**************************************************************************/
static void eth_pio_read(src, dst, cnt)
	unsigned int src;
	unsigned char *dst;
	unsigned int cnt;
{
	if (eth_flags & FLAG_16BIT) { ++cnt; cnt &= ~1; }
	outb(eth_nic_base + D8390_P0_COMMAND, D8390_COMMAND_RD2 |
		D8390_COMMAND_STA);
	outb(eth_nic_base + D8390_P0_RBCR0, cnt);
	outb(eth_nic_base + D8390_P0_RBCR1, cnt>>8);
	outb(eth_nic_base + D8390_P0_RSAR0, src);
	outb(eth_nic_base + D8390_P0_RSAR1, src>>8);
	outb(eth_nic_base + D8390_P0_COMMAND, D8390_COMMAND_RD0 |
		D8390_COMMAND_STA);
	if (eth_flags & FLAG_16BIT) {
		cnt >>= 1;	/* number of words */
		while (cnt--) {
			*((unsigned short *)dst) = inw(eth_asic_base + NE_DATA);
			dst += 2;
		}
	}
	else {
		while (cnt--)
			*(dst++) = inb(eth_asic_base + NE_DATA);
	}
}

/**************************************************************************
ETH_PIO_WRITE - Write a frame via Programmed I/O
**************************************************************************/
static void eth_pio_write(src, dst, cnt)
	unsigned char *src;
	unsigned int dst;
	unsigned int cnt;
{
	if (eth_flags & FLAG_16BIT) { ++cnt; cnt &= ~1; }
	outb(eth_nic_base + D8390_P0_COMMAND, D8390_COMMAND_RD2 |
		D8390_COMMAND_STA);
	outb(eth_nic_base + D8390_P0_ISR, D8390_ISR_RDC);
	outb(eth_nic_base + D8390_P0_RBCR0, cnt);
	outb(eth_nic_base + D8390_P0_RBCR1, cnt>>8);
	outb(eth_nic_base + D8390_P0_RSAR0, dst);
	outb(eth_nic_base + D8390_P0_RSAR1, dst>>8);
	outb(eth_nic_base + D8390_P0_COMMAND, D8390_COMMAND_RD1 |
		D8390_COMMAND_STA);
	if (eth_flags & FLAG_16BIT) {
		cnt >>= 1;	/* number of words */
		while (cnt--) {
			outw_p(eth_asic_base + NE_DATA, *((unsigned short *)src));
			src += 2;
		}
	}
	else {
		while (cnt--)
			outb_p(eth_asic_base + NE_DATA, *(src++));
	}
	while((inb_p(eth_nic_base + D8390_P0_ISR) & D8390_ISR_RDC)
		!= D8390_ISR_RDC);
}
#else
/**************************************************************************
ETH_PIO_READ - Dummy routine when NE2000 not compiled in
**************************************************************************/
static void eth_pio_read(unsigned int src, unsigned char *dst, unsigned int cnt) {}
#endif

/**************************************************************************
ETH_RESET - Reset adapter
**************************************************************************/
static void eth_reset(struct nic *nic)
{
	int i;
	if (eth_flags & FLAG_790)
		outb(eth_nic_base+D8390_P0_COMMAND,
			D8390_COMMAND_PS0 | D8390_COMMAND_STP);
	else
		outb(eth_nic_base+D8390_P0_COMMAND,
			D8390_COMMAND_PS0 | D8390_COMMAND_RD2 |
			D8390_COMMAND_STP);
	if (eth_flags & FLAG_16BIT)
		outb(eth_nic_base+D8390_P0_DCR, 0x49);
	else
		outb(eth_nic_base+D8390_P0_DCR, 0x48);
	outb(eth_nic_base+D8390_P0_RBCR0, 0);
	outb(eth_nic_base+D8390_P0_RBCR1, 0);
	outb(eth_nic_base+D8390_P0_RCR, 0x20);	/* monitor mode */
	outb(eth_nic_base+D8390_P0_TCR, 2);
	outb(eth_nic_base+D8390_P0_TPSR, eth_tx_start);
	outb(eth_nic_base+D8390_P0_PSTART, eth_rx_start);
	if (eth_flags & FLAG_790) outb(eth_nic_base + 0x09, 0);
	outb(eth_nic_base+D8390_P0_PSTOP, eth_memsize);
	outb(eth_nic_base+D8390_P0_BOUND, eth_memsize - 1);
	outb(eth_nic_base+D8390_P0_ISR, 0xFF);
	outb(eth_nic_base+D8390_P0_IMR, 0);
	if (eth_flags & FLAG_790)
		outb(eth_nic_base+D8390_P0_COMMAND, D8390_COMMAND_PS1 |
			D8390_COMMAND_STP);
	else
		outb(eth_nic_base+D8390_P0_COMMAND, D8390_COMMAND_PS1 |
			D8390_COMMAND_RD2 | D8390_COMMAND_STP);
	for (i=0; i<ETHER_ADDR_SIZE; i++)
		outb(eth_nic_base+D8390_P1_PAR0+i, nic->node_addr[i]);
	for (i=0; i<ETHER_ADDR_SIZE; i++)
		outb(eth_nic_base+D8390_P1_MAR0+i, 0xFF);
	outb(eth_nic_base+D8390_P1_CURR, eth_rx_start);
	if (eth_flags & FLAG_790)
		outb(eth_nic_base+D8390_P0_COMMAND, D8390_COMMAND_PS0 |
			D8390_COMMAND_STA);
	else
		outb(eth_nic_base+D8390_P0_COMMAND, D8390_COMMAND_PS0 |
			D8390_COMMAND_RD2 | D8390_COMMAND_STA);
	outb(eth_nic_base+D8390_P0_ISR, 0xFF);
	outb(eth_nic_base+D8390_P0_TCR, 0);
	outb(eth_nic_base+D8390_P0_RCR, 4);	/* allow broadcast frames */
#ifdef INCLUDE_T503
        if (eth_vendor == VENDOR_3COM) {
        /*
         * No way to tell whether or not we're supposed to use
         * the 3Com's transceiver unless the user tells us.
         * 'aui' should have some compile time default value
         * which can be changed from the command menu.
         */
                if (nic->aui)
                        outb(eth_asic_base + _3COM_CR, 0);
                else
                        outb(eth_asic_base + _3COM_CR, _3COM_CR_XSEL);
        }
#endif
}

/**************************************************************************
ETH_TRANSMIT - Transmit a frame
**************************************************************************/
static void eth_transmit(
	struct nic *nic,
	char *d,			/* Destination */
	unsigned int t,			/* Type */
	unsigned int s,			/* size */
	char *p)			/* Packet */
{
	int c;				/* used in NETBOOT16 */


#ifdef INCLUDE_T503
        if (eth_vendor == VENDOR_3COM) {
#ifdef	NETBOOT32
                net_bcopy(d, (void *)eth_bmem, ETHER_ADDR_SIZE);	/* dst */
                net_bcopy(nic->node_addr, (void *)eth_bmem+ETHER_ADDR_SIZE, ETHER_ADDR_SIZE); /* src */
                *((char *)eth_bmem+12) = t>>8;		/* type */
                *((char *)eth_bmem+13) = t;
                net_bcopy(p, (void *)eth_bmem+ETHER_HDR_SIZE, s);
                s += ETHER_HDR_SIZE;
                while (s < ETH_MIN_PACKET) *((char *)eth_bmem+(s++)) = 0;
#endif
#ifdef	NETBOOT16
		bcopyf(d, eth_bmem, ETHER_ADDR_SIZE);
		bcopyf(nic->node_addr, eth_bmem+ETHER_ADDR_SIZE, ETHER_ADDR_SIZE);
		c = t >> 8;
		bcopyf(&c, eth_bmem+12, 1);
		c = t;
		bcopyf(&c, eth_bmem+13, 1);
		bcopyf(p, (Address)(eth_bmem+ETHER_HDR_SIZE), s);
		s += ETHER_HDR_SIZE;
		if (s < ETH_MIN_PACKET)
			bzerof(eth_bmem+s, ETH_MIN_PACKET-s), s = ETH_MIN_PACKET;
#endif
        }
#endif
#ifdef INCLUDE_WD
	if (eth_vendor == VENDOR_WD) {		/* Memory interface */
		if (eth_flags & FLAG_16BIT) {
			outb(eth_asic_base + WD_LAAR, eth_laar | WD_LAAR_M16EN);
			inb(0x84);
		}
		if (eth_flags & FLAG_790) {
			outb(eth_asic_base + WD_MSR, WD_MSR_MENB);
			inb(0x84);
		}
		inb(0x84);
#ifdef	NETBOOT32
		net_bcopy(d, (void *)eth_bmem, ETHER_ADDR_SIZE);	/* dst */

		net_bcopy(nic->node_addr, (void *)eth_bmem+ETHER_ADDR_SIZE, ETHER_ADDR_SIZE); /* src */
		*((char *)eth_bmem+12) = t>>8;		/* type */
		*((char *)eth_bmem+13) = t;
		net_bcopy(p, (void *)eth_bmem+ETHER_HDR_SIZE, s);
		s += ETHER_HDR_SIZE;
		while (s < ETH_MIN_PACKET) *((char *)eth_bmem+(s++)) = 0;
#endif
#ifdef	NETBOOT16
		bcopyf(d, eth_bmem, ETHER_ADDR_SIZE);
		bcopyf(nic->node_addr, eth_bmem+ETHER_ADDR_SIZE, ETHER_ADDR_SIZE);
		c = t >> 8;
		/* bcc generates worse code without (const+const) below */
		bcopyf(&c, eth_bmem+(ETHER_ADDR_SIZE+ETHER_ADDR_SIZE), 1);
		c = t;
		bcopyf(&c, eth_bmem+(ETHER_ADDR_SIZE+ETHER_ADDR_SIZE+1), 1);
		bcopyf(p, (Address)(eth_bmem+ETHER_HDR_SIZE), s);
		s += ETHER_HDR_SIZE;
		if (s < ETH_MIN_PACKET)
			bzerof(eth_bmem+s, ETH_MIN_PACKET-s), s = ETH_MIN_PACKET;
#endif
		if (eth_flags & FLAG_790) {
			outb(eth_asic_base + WD_MSR, 0);
			inb(0x84);
		}
		if (eth_flags & FLAG_16BIT) {
			outb(eth_asic_base + WD_LAAR, eth_laar & ~WD_LAAR_M16EN);
			inb(0x84);
		}
	}
#endif
#if	defined(INCLUDE_NE) || defined(INCLUDE_NEPCI)
	if (eth_vendor == VENDOR_NOVELL) {	/* Programmed I/O */
		unsigned short type;
		type = (t >> 8) | (t << 8);
		eth_pio_write(d, eth_tx_start<<8, ETHER_ADDR_SIZE);
		eth_pio_write(nic->node_addr, (eth_tx_start<<8)+ETHER_ADDR_SIZE, ETHER_ADDR_SIZE);
		/* bcc generates worse code without (const+const) below */
		eth_pio_write((unsigned char *)&type, (eth_tx_start<<8)+(ETHER_ADDR_SIZE+ETHER_ADDR_SIZE), 2);
		eth_pio_write(p, (eth_tx_start<<8)+ETHER_HDR_SIZE, s);
		s += ETHER_HDR_SIZE;
		if (s < ETH_MIN_PACKET) s = ETH_MIN_PACKET;
	}
#endif
	if (eth_flags & FLAG_790)
		outb(eth_nic_base+D8390_P0_COMMAND, D8390_COMMAND_PS0 |
			D8390_COMMAND_STA);
	else
		outb(eth_nic_base+D8390_P0_COMMAND, D8390_COMMAND_PS0 |
			D8390_COMMAND_RD2 | D8390_COMMAND_STA);
	outb(eth_nic_base+D8390_P0_TPSR, eth_tx_start);
	outb(eth_nic_base+D8390_P0_TBCR0, s);
	outb(eth_nic_base+D8390_P0_TBCR1, s>>8);
	if (eth_flags & FLAG_790)
		outb(eth_nic_base+D8390_P0_COMMAND, D8390_COMMAND_PS0 |
			D8390_COMMAND_TXP | D8390_COMMAND_STA);
	else
		outb(eth_nic_base+D8390_P0_COMMAND, D8390_COMMAND_PS0 |
			D8390_COMMAND_TXP | D8390_COMMAND_RD2 |
			D8390_COMMAND_STA);
}

/**************************************************************************
ETH_POLL - Wait for a frame
**************************************************************************/
static int eth_poll(struct nic *nic)
{
	int ret = 0;
	unsigned char rstat, curr, next;
	unsigned short len, frag;
	unsigned short pktoff;
	unsigned char *p;
	struct ringbuffer pkthdr;
	rstat = inb(eth_nic_base+D8390_P0_RSR);
	if (rstat & D8390_RSTAT_OVER) {
		eth_reset(nic);
		return(0);
	}
	if (!(rstat & D8390_RSTAT_PRX)) return(0);
	next = inb(eth_nic_base+D8390_P0_BOUND)+1;
	if (next >= eth_memsize) next = eth_rx_start;
	outb(eth_nic_base+D8390_P0_COMMAND, D8390_COMMAND_PS1);
	curr = inb(eth_nic_base+D8390_P1_CURR);
	outb(eth_nic_base+D8390_P0_COMMAND, D8390_COMMAND_PS0);
	if (curr >= eth_memsize) curr=eth_rx_start;
	if (curr == next) return(0);
	if (eth_vendor == VENDOR_WD) {
		if (eth_flags & FLAG_16BIT) {
			outb(eth_asic_base + WD_LAAR, eth_laar | WD_LAAR_M16EN);
			inb(0x84);
		}
		if (eth_flags & FLAG_790) {
			outb(eth_asic_base + WD_MSR, WD_MSR_MENB);
			inb(0x84);
		}
		inb(0x84);
	}
	pktoff = next << 8;
	if (eth_flags & FLAG_PIO)
		eth_pio_read(pktoff, (char *)&pkthdr, 4);
	else
#ifdef	NETBOOT32
		net_bcopy((void *)eth_rmem + pktoff, &pkthdr, 4);
#endif
#ifdef	NETBOOT16
		fnet_bcopy(eth_rmem + pktoff, &pkthdr, 4);
#endif
	pktoff += sizeof(pkthdr);
	len = pkthdr.len - 4; /* sub CRC */
	if ((pkthdr.status & D8390_RSTAT_PRX) == 0 || pkthdr.len < ETH_MIN_PACKET || pkthdr.len > ETH_MAX_PACKET) {
		printf("B");
		return (0);
	}
	else {
		p = nic->packet;
		nic->packetlen = len;		/* available to caller */
		frag = (eth_memsize << 8) - pktoff;
		if (len > frag) {		/* We have a wrap-around */
			/* read first part */
			if (eth_flags & FLAG_PIO)
				eth_pio_read(pktoff, p, frag);
			else
#ifdef	NETBOOT32
				net_bcopy((void *)eth_rmem + pktoff, p, frag);
#endif
#ifdef	NETBOOT16
				fnet_bcopy(eth_rmem + pktoff, p, frag);
#endif
			pktoff = eth_rx_start << 8;
			p += frag;
			len -= frag;
		}
		/* read second part */
		if (eth_flags & FLAG_PIO)
			eth_pio_read(pktoff, p, len);
		else
#ifdef	NETBOOT32
			net_bcopy((void *)eth_rmem + pktoff, p, len);
#endif
#ifdef	NETBOOT16
			fnet_bcopy(eth_rmem + pktoff, p, len);
#endif
		ret = 1;
	}
	if (eth_vendor == VENDOR_WD) {
		if (eth_flags & FLAG_790) {
			outb(eth_asic_base + WD_MSR, 0);
			inb(0x84);
		}
		if (eth_flags & FLAG_16BIT) {
			outb(eth_asic_base + WD_LAAR, eth_laar &
				~WD_LAAR_M16EN);
			inb(0x84);
		}
		inb(0x84);
	}
	next = pkthdr.next;		/* frame number of next packet */
	if (next == eth_rx_start)
		next = eth_memsize;
	outb(eth_nic_base+D8390_P0_BOUND, next-1);
	return(ret);
}

/**************************************************************************
ETH_DISABLE - Turn off adapter
**************************************************************************/
static void eth_disable(struct nic *nic)
{
}

/**************************************************************************
ETH_PROBE - Look for an adapter
**************************************************************************/
struct nic *eth_probe(struct nic *nic, unsigned short *probe_addrs)
{
	int i;
	struct wd_board *brd;
	unsigned short chksum;
	unsigned char c;
#if	defined(INCLUDE_WD) && defined(NETBOOT16)
	unsigned char	bmem13, bmem11;
#endif

	eth_vendor = VENDOR_NONE;

#ifdef INCLUDE_WD
	/******************************************************************
	Search for WD/SMC cards
	******************************************************************/
	for (eth_asic_base = WD_LOW_BASE; eth_asic_base <= WD_HIGH_BASE;
		eth_asic_base += 0x20) {
		chksum = 0;
		for (i=8; i<16; i++)
			chksum += inb(i+eth_asic_base);
		if ((chksum & 0x00FF) == 0x00FF)
			break;
	}
	if (eth_asic_base <= WD_HIGH_BASE) { /* We've found a board */
		eth_vendor = VENDOR_WD;
		eth_nic_base = eth_asic_base + WD_NIC_ADDR;
		c = inb(eth_asic_base+WD_BID);	/* Get board id */
		for (brd = wd_boards; brd->name; brd++)
			if (brd->id == c) break;
		if (!brd->name) {
			printf("\r\nUnknown Ethernet type %x\r\n", c);
			return(0);	/* Unknown type */
		}
		eth_flags = brd->flags;
		eth_memsize = brd->memsize;
		eth_tx_start = 0;
		eth_rx_start = D8390_TXBUF_SIZE;
		if ((c == TYPE_WD8013EP) &&
			(inb(eth_asic_base + WD_ICR) & WD_ICR_16BIT)) {
				eth_flags = FLAG_16BIT;
				eth_memsize = MEM_16384;
		}
		if ((c & WD_SOFTCONFIG) && (!(eth_flags & FLAG_790))) {
#ifdef	NETBOOT32
			eth_bmem = (0x80000 |
			 ((inb(eth_asic_base + WD_MSR) & 0x3F) << 13));
#endif
#ifdef	NETBOOT16
			eth_bmem = inb(eth_asic_base + WD_MSR) & 0x3F;
			eth_bmem <<= 13;
			eth_bmem |= 0x80000;
#endif
		} else
			eth_bmem = WD_DEFAULT_MEM;
#ifdef	NETBOOT16
		/* cast is to force evaluation in long precision */
		bmem13 = (Address)eth_bmem >> 13;
		bmem11 = (Address)eth_bmem >> 11;
#endif
#ifdef	NETBOOT32
		if (brd->id == TYPE_SMC8216T || brd->id == TYPE_SMC8216C) {
			*((unsigned int *)(eth_bmem + 8192)) = (unsigned int)0;
			if (*((unsigned int *)(eth_bmem + 8192))) {
				brd += 2;
				eth_memsize = brd->memsize;
			}
		}
#endif
#ifdef	NETBOOT16
		if (brd->id == TYPE_SMC8216T || brd->id == TYPE_SMC8216C) {
			i = 0;
			bcopyf(&i, eth_bmem + 8192, sizeof(i));
			if (!fbsame(eth_bmem + 8192, 0, sizeof(i))) {
				brd += 2;
				eth_memsize = brd->memsize;
			}
		}
#endif
		outb(eth_asic_base + WD_MSR, 0x80);	/* Reset */
		printf("\r\n%s base 0x%x, memory 0x%x, addr ",
			brd->name, eth_asic_base, eth_bmem);
		for (i=0; i<ETHER_ADDR_SIZE; i++) {
			printf("%x",((int)(nic->node_addr[i] =
				inb(i+eth_asic_base+WD_LAR)) & 0xff));
				if (i < ETHER_ADDR_SIZE-1) printf (":");
		}
		if (eth_flags & FLAG_790) {
			outb(eth_asic_base+WD_MSR, WD_MSR_MENB);
			outb(eth_asic_base+0x04, (inb(eth_asic_base+0x04) |
				0x80));
#ifdef	NETBOOT32
			outb(eth_asic_base+0x0B,
				(((unsigned)eth_bmem >> 13) & 0x0F) |
				(((unsigned)eth_bmem >> 11) & 0x40) |
				(inb(eth_asic_base+0x0B) & 0xB0));
#endif
#ifdef	NETBOOT16
			outb(eth_asic_base+0x0B,
				(bmem13 & 0x0F) |
				(bmem11 & 0x40) |
				(inb(eth_asic_base+0x0B) & 0xB0));
#endif
			outb(eth_asic_base+0x04, (inb(eth_asic_base+0x04) &
				~0x80));
		} else {
#ifdef	NETBOOT32
			outb(eth_asic_base+WD_MSR,
				(((unsigned)eth_bmem >> 13) & 0x3F) | 0x40);
#endif
#ifdef	NETBOOT16
			outb(eth_asic_base+WD_MSR,
				(bmem13 & 0x3F) | 0x40);
#endif
		}
		if (eth_flags & FLAG_16BIT) {
			if (eth_flags & FLAG_790) {
				eth_laar = inb(eth_asic_base + WD_LAAR);
				outb(eth_asic_base + WD_LAAR, WD_LAAR_M16EN);
				inb(0x84);
			} else {
				outb(eth_asic_base + WD_LAAR, (eth_laar =
					WD_LAAR_M16EN | WD_LAAR_L16EN | 1));
			}
		}
		printf("\r\n");

	}
#endif
#ifdef INCLUDE_T503
        /******************************************************************
        Search for 3Com 3c503 if no WD/SMC cards
        ******************************************************************/
	if (eth_vendor == VENDOR_NONE) {
		int	idx;
		static unsigned short	base[] = {
			0x300, 0x310, 0x330, 0x350,
			0x250, 0x280, 0x2A0, 0x2E0, 0 };
		/* this must parallel the table above */
		static unsigned char	bcfr[] = {
                        _3COM_BCFR_300, _3COM_BCFR_310,
                        _3COM_BCFR_330, _3COM_BCFR_350,
                        _3COM_BCFR_250, _3COM_BCFR_280,
                        _3COM_BCFR_2A0, _3COM_BCFR_2E0, 0 };

		/* Loop through possible addresses checking each one */

		for (idx = 0; (eth_nic_base = base[idx]) != 0; ++idx) {

			eth_asic_base = eth_nic_base + _3COM_ASIC_OFFSET;
/*
 * Note that we use the same settings for both 8 and 16 bit cards:
 * both have an 8K bank of memory at page 1 while only the 16 bit
 * cards have a bank at page 0.
 */
			eth_memsize = MEM_16384;
			eth_tx_start = 32;
			eth_rx_start = 32 + D8390_TXBUF_SIZE;

		/* Check our base address */

			if (inb(eth_asic_base + _3COM_BCFR) != bcfr[idx])
				continue;		/* nope */

		/* Now get the shared memory address */

			switch (inb(eth_asic_base + _3COM_PCFR)) {
				case _3COM_PCFR_DC000:
					eth_bmem = 0xdc000;
					break;
				case _3COM_PCFR_D8000:
					eth_bmem = 0xd8000;
					break;
				case _3COM_PCFR_CC000:
					eth_bmem = 0xcc000;
					break;
				case _3COM_PCFR_C8000:
					eth_bmem = 0xc8000;
					break;
				default:
					continue;	/* nope */
				}
			break;
		}

		if (base[idx] == 0)		/* not found */
			return (0);
		eth_vendor = VENDOR_3COM;


        /* Need this to make eth_poll() happy. */

                eth_rmem = eth_bmem - 0x2000;

        /* Reset NIC and ASIC */

                outb (eth_asic_base + _3COM_CR , _3COM_CR_RST | _3COM_CR_XSEL);
                outb (eth_asic_base + _3COM_CR , _3COM_CR_XSEL);

        /* Get our ethernet address */

                outb(eth_asic_base + _3COM_CR, _3COM_CR_EALO | _3COM_CR_XSEL);
                printf("\r\n3Com 3c503 base 0x%x, memory 0x%X addr ",
                                 eth_nic_base, eth_bmem);
                for (i=0; i<ETHER_ADDR_SIZE; i++) {
                        printf("%b",(int)(nic->node_addr[i] =
			inb(eth_nic_base+i)));
                        if (i < ETHER_ADDR_SIZE-1) printf (":");
                }
                outb(eth_asic_base + _3COM_CR, _3COM_CR_XSEL);
        /*
         * Initialize GA configuration register. Set bank and enable shared
         * mem. We always use bank 1.
         */
                outb(eth_asic_base + _3COM_GACFR, _3COM_GACFR_RSEL |
                                _3COM_GACFR_MBS0);

                outb(eth_asic_base + _3COM_VPTR2, 0xff);
                outb(eth_asic_base + _3COM_VPTR1, 0xff);
                outb(eth_asic_base + _3COM_VPTR0, 0x00);
        /*
         * Clear memory and verify that it worked (we use only 8K)
         */
#ifdef	NETBOOT32
                bzero((char *)eth_bmem, 0x2000);
                for(i = 0; i < 0x2000; ++i)
                        if (*(((char *)eth_bmem)+i)) {
                                printf ("Failed to clear 3c503 shared mem.\r\n");
                                return (0);
                        }
#endif
#ifdef	NETBOOT16
                bzerof(eth_bmem, 0x2000);
		if (!fbsame(eth_bmem, 0, 0x2000)) {
			printf ("Failed to clear 3c503 shared mem.\r\n");
			return (0);
		}
#endif
        /*
         * Initialize GA page/start/stop registers.
         */
                outb(eth_asic_base + _3COM_PSTR, eth_tx_start);
                outb(eth_asic_base + _3COM_PSPR, eth_memsize);

                printf ("\r\n");

        }
#endif
#if	defined(INCLUDE_NE) || defined(INCLUDE_NEPCI)
	/******************************************************************
	Search for NE1000/2000 if no WD/SMC or 3com cards
	******************************************************************/
	if (eth_vendor == VENDOR_NONE) {
		char romdata[16], testbuf[32];
		int idx;
		static char test[] = "NE1000/2000 memory";
#ifndef	NE_SCAN
#define	NE_SCAN	0
#endif
		static unsigned short base[] = {NE_SCAN, 0};
		/* if no addresses supplied, fall back on defaults */
		if (probe_addrs == 0 || probe_addrs[0] == 0)
			probe_addrs = base;
		eth_bmem = 0;		/* No shared memory */
		for (idx = 0; (eth_nic_base = probe_addrs[idx]) != 0; ++idx) {
			eth_flags = FLAG_PIO;
			eth_asic_base = eth_nic_base + NE_ASIC_OFFSET;
			eth_memsize = MEM_16384;
			eth_tx_start = 32;
			eth_rx_start = 32 + D8390_TXBUF_SIZE;
			c = inb(eth_asic_base + NE_RESET);
			outb(eth_asic_base + NE_RESET, c);
			inb(0x84);
			outb(eth_nic_base + D8390_P0_COMMAND, D8390_COMMAND_STP |
				D8390_COMMAND_RD2);
			outb(eth_nic_base + D8390_P0_RCR, D8390_RCR_MON);
			outb(eth_nic_base + D8390_P0_DCR, D8390_DCR_FT1 | D8390_DCR_LS);
			outb(eth_nic_base + D8390_P0_PSTART, MEM_8192);
			outb(eth_nic_base + D8390_P0_PSTOP, MEM_16384);
			eth_pio_write(test, 8192, sizeof(test));
			eth_pio_read(8192, testbuf, sizeof(test));
			if (!bcmp(test, testbuf, sizeof(test)))
				break;
			eth_flags |= FLAG_16BIT;
			eth_memsize = MEM_32768;
			eth_tx_start = 64;
			eth_rx_start = 64 + D8390_TXBUF_SIZE;
			outb(eth_nic_base + D8390_P0_DCR, D8390_DCR_WTS |
				D8390_DCR_FT1 | D8390_DCR_LS);
			outb(eth_nic_base + D8390_P0_PSTART, MEM_16384);
			outb(eth_nic_base + D8390_P0_PSTOP, MEM_32768);
			eth_pio_write(test, 16384, sizeof(test));
			eth_pio_read(16384, testbuf, sizeof(test));
			if (!bcmp(testbuf, test, sizeof(test)))
				break;
		}
		if (eth_nic_base == 0)
			return (0);
		if (eth_nic_base > ISA_MAX_ADDR)	/* PCI probably */
			eth_flags |= FLAG_16BIT;
		eth_vendor = VENDOR_NOVELL;
		eth_pio_read(0, romdata, 16);
		printf("\r\nNE*000 base 0x%x, addr ", eth_nic_base);
		for (i=0; i<ETHER_ADDR_SIZE; i++) {
			printf("%x",(int)(nic->node_addr[i] = romdata[i
				+ ((eth_flags & FLAG_16BIT) ? i : 0)]));
			if (i < ETHER_ADDR_SIZE-1) printf (":");
		}
		printf("\r\n");
	}
#endif
	if (eth_vendor == VENDOR_NONE) return(0);

        if (eth_vendor != VENDOR_3COM) eth_rmem = eth_bmem;
	eth_reset(nic);
	nic->reset = eth_reset;
	nic->poll = eth_poll;
	nic->transmit = eth_transmit;
	nic->disable = eth_disable;
	return(nic);
}

#endif

/*
 * Local variables:
 *  c-basic-offset: 8
 * End:
 */
