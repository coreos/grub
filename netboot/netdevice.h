/*
 * INET		An implementation of the TCP/IP protocol suite for the LINUX
 *		operating system.  INET is implemented using the  BSD Socket
 *		interface as the means of communication with the user level.
 *
 *		Definitions for the Interfaces handler.
 *
 * Version:	@(#)dev.h	1.0.10	08/12/93
 *
 * Authors:	Ross Biro, <bir7@leland.Stanford.Edu>
 *		Fred N. van Kempen, <waltje@uWalt.NL.Mugnet.ORG>
 *		Corey Minyard <wf-rch!minyard@relay.EU.net>
 *		Donald J. Becker, <becker@super.org>
 *		Alan Cox, <A.Cox@swansea.ac.uk>
 *		Bjorn Ekwall. <bj0rn@blox.se>
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 *		Moved to /usr/include/linux for NET3
 */
/* for future expansion when we will have different priorities. */
#define DEV_NUMBUFFS	3
#define MAX_ADDR_LEN	7
#ifndef CONFIG_AX25
#ifndef CONFIG_TR
#ifndef CONFIG_NET_IPIP
#define MAX_HEADER	32		/* We really need about 18 worst case .. so 32 is aligned */
#else
#define MAX_HEADER	80		/* We need to allow for having tunnel headers */
#endif  /* IPIP */
#else
#define MAX_HEADER	48		/* Token Ring header needs 40 bytes ... 48 is aligned */ 
#endif /* TR */
#else
#define MAX_HEADER	96		/* AX.25 + NetROM */
#endif /* AX25 */

#define IS_MYADDR	1		/* address is (one of) our own	*/
#define IS_LOOPBACK	2		/* address is for LOOPBACK	*/
#define IS_BROADCAST	3		/* address is a valid broadcast	*/
#define IS_INVBCAST	4		/* Wrong netmask bcast not for us (unused)*/
#define IS_MULTICAST	5		/* Multicast IP address */

/*
 * The DEVICE structure.
 * Actually, this whole structure is a big mistake.  It mixes I/O
 * data with strictly "high-level" data, and it has to know about
 * almost every data structure used in the INET module.  
 */
struct device 
{

  /*
   * This is the first field of the "visible" part of this structure
   * (i.e. as seen by users in the "Space.c" file).  It is the name
   * the interface.
   */
  char			  *name;

  /* I/O specific fields - FIXME: Merge these and struct ifmap into one */
  unsigned long		  rmem_end;		/* shmem "recv" end	*/
  unsigned long		  rmem_start;		/* shmem "recv" start	*/
  unsigned long		  mem_end;		/* shared mem end	*/
  unsigned long		  mem_start;		/* shared mem start	*/
  unsigned long		  base_addr;		/* device I/O address	*/
  unsigned char		  irq;			/* device IRQ number	*/

  /* Low-level status flags. */
  volatile unsigned char  start,		/* start an operation	*/
                          interrupt;		/* interrupt arrived	*/
  unsigned long		  tbusy;		/* transmitter busy must be long for bitops */

  struct device		  *next;

  /* The device initialization function. Called only once. */
  int			  (*init)(struct device *dev);

  /* Some hardware also needs these fields, but they are not part of the
     usual set specified in Space.c. */
  unsigned char		  if_port;		/* Selectable AUI, TP,..*/
  unsigned char		  dma;			/* DMA channel		*/

/*   struct enet_statistics* (*get_stats)(struct device *dev); */

  /*
   * This marks the end of the "visible" part of the structure. All
   * fields hereafter are internal to the system, and may change at
   * will (read: may be cleaned up at will).
   */

  /* These may be needed for future network-power-down code. */
  unsigned long		  trans_start;	/* Time (in jiffies) of last Tx	*/
  unsigned long		  last_rx;	/* Time of last Rx		*/

  unsigned short	  flags;	/* interface flags (a la BSD)	*/
  unsigned short	  family;	/* address family ID (AF_INET)	*/
  unsigned short	  metric;	/* routing metric (not used)	*/
  unsigned short	  mtu;		/* interface MTU value		*/
  unsigned short	  type;		/* interface hardware type	*/
  unsigned short	  hard_header_len;	/* hardware hdr length	*/
  void			  *priv;	/* pointer to private data	*/

  /* Interface address info. */
  unsigned char		  broadcast[MAX_ADDR_LEN];	/* hw bcast add	*/
  unsigned char		  pad;				/* make dev_addr aligned to 8 bytes */
  unsigned char		  dev_addr[MAX_ADDR_LEN];	/* hw address	*/
  unsigned char		  addr_len;	/* hardware address length	*/
  unsigned long		  pa_addr;	/* protocol address		*/
  unsigned long		  pa_brdaddr;	/* protocol broadcast addr	*/
  unsigned long		  pa_dstaddr;	/* protocol P-P other side addr	*/
  unsigned long		  pa_mask;	/* protocol netmask		*/
  unsigned short	  pa_alen;	/* protocol address length	*/

#if 0
  struct dev_mc_list	 *mc_list;	/* Multicast mac addresses	*/ 
#endif
  int			 mc_count;	/* Number of installed mcasts	*/
  
#if 0
  struct ip_mc_list	 *ip_mc_list;	/* IP multicast filter chain    */ 
#endif
  unsigned int 		tx_queue_len;	/* Max frames per queue allowed */
    
  /* For load balancing driver pair support */
  
  unsigned long		   pkt_queue;	/* Packets queued */
#if 0
  struct device		  *slave;	/* Slave device */

  struct net_alias_info		*alias_info;	/* main dev alias info */ 
  struct net_alias		*my_alias;	/* alias devs */ 
  /* Pointer to the interface buffers. */
  struct sk_buff_head	  buffs[DEV_NUMBUFFS];
#endif  

  int			  (*change_mtu)(struct device *dev, int new_mtu);            
};

#if 0
struct packet_type {
  unsigned short	type;	/* This is really htons(ether_type). */
  struct device *	dev;
  int			(*func) (struct sk_buff *, struct device *,
				 struct packet_type *);
  void			*data;
  struct packet_type	*next;
};


/* Used by dev_rint */
#define IN_SKBUFF	1

extern int		dev_lockct;

/*
 *	These two don't currently need to be interrupt-safe
 *	but they may do soon. Do it properly anyway.
 */

extern __inline__ void  dev_lock_list(void)
{
	unsigned long flags;
	save_flags(flags);
	cli();
	dev_lockct++;
	restore_flags(flags);
}

extern __inline__ void  dev_unlock_list(void)
{
	unsigned long flags;
	save_flags(flags);
	cli();
	dev_lockct--;
	restore_flags(flags);
}

/*
 *	This almost never occurs, isn't in performance critical paths
 *	and we can thus be relaxed about it
 */
 
extern __inline__ void dev_lock_wait(void)
{
	while(dev_lockct)
		schedule();
}




#endif	/* _LINUX_DEV_H */
