#include "netboot.h"
#include "config.h"
#include "nic.h"

#include "ip.h"


struct arptable_t arptable[MAX_ARP];
int _pending_key;

static unsigned long	netmask;
static char rfc1533_cookie[5] = { RFC1533_COOKIE, RFC1533_END };
static char broadcast[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

static char	*kernel;
static char	kernel_buf[128];
static struct bootpd_t bootp_data;
static int     vendorext_isvalid;
static unsigned char vendorext_magic[] = {0xE4,0x45,0x74,0x68}; /* äEth */
static unsigned char   *end_of_rfc1533 = NULL;

int ip_abort = 0;

static int bootp(void);
static void default_netmask(void);
static void convert_ipaddr(register char *d, register char *s);
static int decode_rfc1533(register unsigned char *p, int block, int len, 
			  int eof);


int ip_init(void)
{
  static int bootp_completed = 0,
    probe_completed = 0;

  if (!probe_completed && !eth_probe())
    { 
      printf("No ethernet adapter found\n");
      return 0;
    }
  probe_completed = 1;

#if 1
  if ((!bootp_completed  ||
       !arptable[ARP_CLIENT].ipaddr || !arptable[ARP_SERVER].ipaddr))
    {
#endif
      if (!bootp())
	{
	  printf("No BOOTP server found\n");
	  return 0;
	}

      printf("My IP 0x%x, Server IP 0x%x, GW IP 0x%x\n",
	     arptable[ARP_CLIENT].ipaddr,
	     arptable[ARP_SERVER].ipaddr,
	     arptable[ARP_GATEWAY].ipaddr);
#ifdef CONFIG_FILE_EXT
      {
	char *ext = &config_file;
	
	/* seek to end of config file name */
	while(*ext)
	  ext++;

	sprintf(ext, ".%d", arptable[ARP_CLIENT].ipaddr & 0xff);
      }
#endif
#if 1
    }
  bootp_completed = 1;
#endif

  return 1;
}

/**************************************************************************
UDP_TRANSMIT - Send a UDP datagram
**************************************************************************/
int udp_transmit(destip, srcsock, destsock, len, buf)
	unsigned long destip;
	unsigned int srcsock, destsock;
	int len;
	char *buf;
{
	struct iphdr *ip;
	struct udphdr *udp;
	struct arprequest arpreq;
	int arpentry, i;
	int retry;

	ip = (struct iphdr *)buf;
	udp = (struct udphdr *)(buf + sizeof(struct iphdr));
	ip->verhdrlen = 0x45;
	ip->service = 0;
	ip->len = htons(len);
	ip->ident = 0;
	ip->frags = 0;
	ip->ttl = 60;
	ip->protocol = IP_UDP;
	ip->chksum = 0;
	convert_ipaddr(ip->src, (char *)&arptable[ARP_CLIENT].ipaddr);
	convert_ipaddr(ip->dest, (char *)&destip);
	ip->chksum = ipchksum((unsigned short *)buf, sizeof(struct iphdr));
	udp->src = htons(srcsock);
	udp->dest = htons(destsock);
	udp->len = htons(len - sizeof(struct iphdr));
	udp->chksum = 0;
	if (destip == IP_BROADCAST) {
		eth_transmit(broadcast, IP, len, buf);
	} else {
		long h_netmask = ntohl(netmask);
				/* Check to see if we need gateway */
		if (((destip & h_netmask) !=
			(arptable[ARP_CLIENT].ipaddr & h_netmask)) &&
			arptable[ARP_GATEWAY].ipaddr)
				destip = arptable[ARP_GATEWAY].ipaddr;
		for(arpentry = 0; arpentry<MAX_ARP; arpentry++)
			if (arptable[arpentry].ipaddr == destip) break;
		if (arpentry == MAX_ARP) {
			printf("%I is not in my arp table!\r\n", destip);
			return(0);
		}
		for (i = 0; i<ETHER_ADDR_SIZE; i++)
			if (arptable[arpentry].node[i]) break;
		if (i == ETHER_ADDR_SIZE) {	/* Need to do arp request */
			arpreq.hwtype = htons(1);
			arpreq.protocol = htons(IP);
			arpreq.hwlen = ETHER_ADDR_SIZE;
			arpreq.protolen = 4;
			arpreq.opcode = htons(ARP_REQUEST);
			bcopy(arptable[ARP_CLIENT].node, arpreq.shwaddr,
				ETHER_ADDR_SIZE);
			convert_ipaddr(arpreq.sipaddr,
				(char *)&arptable[ARP_CLIENT].ipaddr);
			bzero(arpreq.thwaddr, ETHER_ADDR_SIZE);
			convert_ipaddr(arpreq.tipaddr, (char *)&destip);
			for (retry = 0; retry < MAX_ARP_RETRIES;
			     rfc951_sleep(++retry)) {
				eth_transmit(broadcast, ARP, sizeof(arpreq),
					(char *)&arpreq);
				if (await_reply(AWAIT_ARP, arpentry,
					arpreq.tipaddr)) goto xmit;
				if (ip_abort)
				  return 0;
			}
			return(0);
		}
xmit:		eth_transmit(arptable[arpentry].node, IP, len, buf);
	}
	return(1);
}

/**************************************************************************
AWAIT_REPLY - Wait until we get a response for our request
**************************************************************************/
int await_reply(type, ival, ptr)
	int type, ival;
	char *ptr;
{
	unsigned long time;
	struct	iphdr *ip;
	struct	udphdr *udp;
	struct	arprequest *arpreply;
	struct	bootp_t *bootpreply;
	struct	rpc_t *rpc;

	int	protohdrlen = ETHER_HDR_SIZE + sizeof(struct iphdr) +
				sizeof(struct udphdr);

	ip_abort = 0;

	time = currticks() + TIMEOUT;
	while(time > currticks()) {
#if 0
		if (iskey() && (getchar() == ESC)) longjmp(jmp_bootmenu,1);
#else
		if (iskey() && (getchar() == ESC)) 
		  {
		    ip_abort = 1;
		    return 0;
		  }
#endif
		if (eth_poll()) 
		  {	/* We have something! */
		    /* Check for ARP - No IP hdr */
		    if ((nic.packetlen >= ETHER_HDR_SIZE +
			 sizeof(struct arprequest)) &&
			(((nic.packet[12] << 8) | nic.packet[13]) == ARP)) 
		      {
			unsigned long target_ip;

			arpreply = (struct arprequest *)
			  &nic.packet[ETHER_HDR_SIZE];

			if (arpreply->opcode == ntohs(ARP_REQUEST)
			    && (convert_ipaddr(&target_ip, 
					       &arpreply->tipaddr),
				arptable[ARP_CLIENT].ipaddr == target_ip))
			  {
			    /* request for our ether address -- send a reply */

			    unsigned long my_ip_net 
			      = *(unsigned long*)(arpreply->tipaddr);

			    bcopy(arpreply->shwaddr, arpreply->thwaddr, 10);
			    bcopy(arptable[ARP_CLIENT].node,
				  arpreply->shwaddr, ETHER_ADDR_SIZE);
			    *(unsigned long*)(arpreply->sipaddr) = my_ip_net;
			    arpreply->opcode = htons(ARP_REPLY);
			    eth_transmit(arpreply->thwaddr, ARP, 
					 sizeof(struct arprequest), 
					 (char *) arpreply);

			    continue;
			  }
			  
			else if (type == AWAIT_ARP &&
				 (arpreply->opcode == ntohs(ARP_REPLY)) &&
				 !bcmp(arpreply->sipaddr, ptr, 4)) {
					bcopy(arpreply->shwaddr,
						arptable[ival].node,
						ETHER_ADDR_SIZE);
					return(1);
				}
			continue;
		      }

					/* Anything else has IP header */
			if ((nic.packetlen < protohdrlen) ||
			   (((nic.packet[12] << 8) | nic.packet[13]) != IP)) continue;
			ip = (struct iphdr *)&nic.packet[ETHER_HDR_SIZE];
			if ((ip->verhdrlen != 0x45) ||
				ipchksum((unsigned short *)ip, sizeof(struct iphdr)) ||
				(ip->protocol != IP_UDP)) continue;
			udp = (struct udphdr *)&nic.packet[ETHER_HDR_SIZE +
				sizeof(struct iphdr)];

					/* BOOTP ? */
			bootpreply = (struct bootp_t *)&nic.packet[ETHER_HDR_SIZE];
			if ((type == AWAIT_BOOTP) &&
			   (nic.packetlen >= (ETHER_HDR_SIZE +
			     sizeof(struct bootp_t))) &&
			   (ntohs(udp->dest) == BOOTP_CLIENT) &&
			   (bootpreply->bp_op == BOOTP_REPLY)) {
				convert_ipaddr((char *)&arptable[ARP_CLIENT].ipaddr,
					bootpreply->bp_yiaddr);
				default_netmask();
				convert_ipaddr((char *)&arptable[ARP_SERVER].ipaddr,
					bootpreply->bp_siaddr);
				bzero(arptable[ARP_SERVER].node,
					ETHER_ADDR_SIZE);  /* Kill arp */
				convert_ipaddr((char *)&arptable[ARP_GATEWAY].ipaddr,
					bootpreply->bp_giaddr);
				bzero(arptable[ARP_GATEWAY].node,
					ETHER_ADDR_SIZE);  /* Kill arp */
				if (bootpreply->bp_file[0]) {
					bcopy(bootpreply->bp_file,
						kernel_buf, 128);
					kernel = kernel_buf;
				}
				bcopy((char *)bootpreply,
				      (char *)&bootp_data,
				      sizeof(struct bootp_t));
				decode_rfc1533(bootp_data.bootp_reply.bp_vend,
					       0, BOOTP_VENDOR_LEN, 1);
				return(1);
			}

					/* TFTP ? */
			if ((type == AWAIT_TFTP) &&
				(ntohs(udp->dest) == ival)) return(1);

					/* RPC */
			rpc = (struct rpc_t *)&nic.packet[ETHER_HDR_SIZE];
#ifdef	NFS_BOOT
			if ((type == AWAIT_RPC) &&
			   (ntohs(udp->dest) == RPC_SOCKET) &&
			   (ntohl(rpc->u.reply.id) == ival) &&
			   (ntohl(rpc->u.reply.type) == MSG_REPLY)) {
				rpc_id++;
				return(1);
			}
#endif	/* NFS_BOOT */
		}
	}
	return(0);
}

/**************************************************************************
IPCHKSUM - Checksum IP Header
**************************************************************************/
unsigned short ipchksum(ip, len)
	register unsigned short *ip;
	register int len;
{
	unsigned long sum = 0;
	len >>= 1;
	while (len--) {
		sum += *(ip++);
		if (sum > 0xFFFF)
			sum -= 0xFFFF;
	}
	return((~sum) & 0x0000FFFF);
}

/**************************************************************************
TFTP - Download extended BOOTP data, configuration file or kernel image
**************************************************************************/
int tftp(name,fnc)
	char           *name;
	int	       (*fnc)(unsigned char *, int, int, int);
{
	int             retry = 0;
	static unsigned short isocket = 2000;
	unsigned short  osocket;
	unsigned short  len, block = 0, prevblock = 0;
	struct tftp_t  *tr;
	struct tftp_t   tp;
	int		rc;
	int		packetsize = TFTP_DEFAULTSIZE_PACKET;

	tp.opcode = htons(TFTP_RRQ);
	len = 30 + sprintf((char *)tp.u.rrq, "%s%coctet%cblksize%c%d",
			   name, 0, 0, 0, TFTP_MAX_PACKET) + 1;
	if (!udp_transmit(arptable[ARP_SERVER].ipaddr, ++isocket, TFTP,
		len, (char *)&tp))
		return (0);
	for (;;)
	{
		if (!await_reply(AWAIT_TFTP, isocket, NULL))
		{
			if (ip_abort)
				return 0;
			if (prevblock == 0 && retry++ < MAX_TFTP_RETRIES)
			{	/* maybe initial request was lost */
				rfc951_sleep(retry);
				if (!udp_transmit(arptable[ARP_SERVER].ipaddr,
					++isocket, TFTP, len, (char *)&tp))
					return (0);
				continue;
			}
			break;	/* timeout on other blocks */
		}
		tr = (struct tftp_t *)&nic.packet[ETHER_HDR_SIZE];
		if (tr->opcode == ntohs(TFTP_ERROR))
		{
			printf("TFTP error %d (%s)\r\n",
			       ntohs(tr->u.err.errcode),
			       tr->u.err.errmsg);
			break;
		}

		if (tr->opcode == ntohs(TFTP_OACK)) {
#if 0
			char *p = tr->u.oack.data, *e;

			retry = MAX_TFTP_RETRIES;/*no more retries! */
			if (prevblock)		/* shouldn't happen */
				continue;	/* ignore it */
			len = ntohs(tr->udp.len) - sizeof(struct udphdr) - 2;
			if (len > TFTP_MAX_PACKET)
				goto noak;
			e = p + len;
			while (*p != '\000' && p < e) {
				if (!strcasecmp("blksize", p)) {
					p += 8;
					if ((packetsize = getdec(&p)) <
					    TFTP_DEFAULTSIZE_PACKET)
						goto noak;
					while (p < e && *p) p++;
					if (p < e)
						p++;
				}
				else {
				noak:
					tp.opcode = htons(TFTP_ERROR);
					tp.u.err.errcode = 8;
					len = 30 + sprintf((char *)tp.u.err.errmsg,
							   "RFC1782 error")+ 1;
					udp_transmit(arptable[ARP_SERVER].
						     ipaddr, isocket,
						     ntohs(tr->udp.src),
						     len, (char *)&tp);
					return (0);
				}
			}
			if (p > e)
				goto noak;
			block = tp.u.ack.block = 0; /* this ensures, that */
						/* the packet does not get */
						/* processed as data! */
#else
			continue;
#endif
		}
		else if (tr->opcode == ntohs(TFTP_DATA)) {
			retry = MAX_TFTP_RETRIES;/*no more retries! */
			len = ntohs(tr->udp.len) - sizeof(struct udphdr) - 4;
			if (len > packetsize)	/* shouldn't happen */
				continue;	/* ignore it */
			block = ntohs(tp.u.ack.block = tr->u.data.block); }
		else /* neither TFTP_OACK nor TFTP_DATA */
			break;
		
		tp.opcode = htons(TFTP_ACK);
		osocket = ntohs(tr->udp.src);
		udp_transmit(arptable[ARP_SERVER].ipaddr, isocket,
			osocket, TFTP_MIN_PACKET, (char *)&tp);	/* ack */
		if (block <= prevblock)		/* retransmission or OACK */
			continue;		/* don't process */
		prevblock = block;
		if ((rc = fnc(tr->u.data.download,
			      block, len, len < packetsize)) >= 0)
			return(rc);
		if (len < packetsize)		/* End of data */
			return (1);
	}
	return (0);
}

/**************************************************************************
CONVERT_IPADDR - Convert IP address from net to machine order
**************************************************************************/
static void convert_ipaddr(d, s)
	register char *d,*s;
{
	d[3] = s[0];
	d[2] = s[1];
	d[1] = s[2];
	d[0] = s[3];
}

/**************************************************************************
RFC951_SLEEP - sleep for expotentially longer times
**************************************************************************/
void rfc951_sleep(exp)
	int exp;
{
	static long seed = 0;
	long q,z,tmo;

	if (!seed) /* Initialize linear congruential generator */
		seed = currticks() + *(long *)&arptable[ARP_CLIENT].node
		       + ((short *)arptable[ARP_CLIENT].node)[2];
	/* simplified version of the LCG given in Bruce Scheier's
	   "Applied Cryptography" */
	q = seed/53668;
	if ((seed = 40014*(seed-53668*q) - 12211*q) < 0) seed += 2147483563l;
	/* compute mask */
	for (tmo = 63; tmo <= 60*18 && --exp > 0; tmo = 2*tmo+1);
	/* sleep */
	printf("<sleep>\r\n");
	for (tmo = (tmo&seed)+currticks(); currticks() < tmo; )
/* 	  if (iskey() && (getchar() == ESC)) longjmp(jmp_bootmenu,1) */ ;
	eth_reset();
	return;
}

/**************************************************************************
TWIDDLE
**************************************************************************/
void twiddle()
{
        static unsigned long lastticks = 0;
        static int count=0;
        static char tiddles[]="-\\|/";
        unsigned long ticks;
        if ((ticks = currticks()) < lastticks)
                return;
        lastticks = ticks+1;
        putchar(tiddles[(count++)&3]);
        putchar('\b');
}

/**************************************************************************
BOOTP - Get my IP address and load information
**************************************************************************/
static int bootp()
{
	int retry;
	struct bootp_t bp;
	unsigned long  starttime;
#ifdef T509HACK
	int flag;

	flag = 1;
#endif
	bzero(&bp, sizeof(struct bootp_t));
	bp.bp_op = BOOTP_REQUEST;
	bp.bp_htype = 1;
	bp.bp_hlen = ETHER_ADDR_SIZE;
	bp.bp_xid = starttime = currticks();
	bcopy(arptable[ARP_CLIENT].node, bp.bp_hwaddr, ETHER_ADDR_SIZE);
	bcopy(rfc1533_cookie, bp.bp_vend, 5); /* request RFC-style options */
	
	for (retry = 0; retry < MAX_BOOTP_RETRIES; ) {
		udp_transmit(IP_BROADCAST, 0, BOOTP_SERVER,
			sizeof(struct bootp_t), (char *)&bp);
#ifdef T509HACK
		if (flag) { 
			flag--;
		} else {
			if (await_reply(AWAIT_BOOTP, 0, NULL))
				return(1);
			if (ip_abort) /* hohmuth */
			    return 0;
			rfc951_sleep(++retry);

		}
#else
		if (await_reply(AWAIT_BOOTP, 0, NULL))
			return(1);
		if (ip_abort)	/* hohmuth */
		    return 0;
		rfc951_sleep(++retry);
#endif
		bp.bp_secs = htons((currticks()-starttime)/20);
	}
	return(0);
}

/**************************************************************************
DEFAULT_NETMASK - Set a default netmask for IP address
**************************************************************************/
static void default_netmask()
{
	int net = arptable[ARP_CLIENT].ipaddr >> 24;
	if (net <= 127)
		netmask = htonl(0xff000000);
	else if (net < 192)
		netmask = htonl(0xffff0000);
	else
		netmask = htonl(0xffffff00);
}

/**************************************************************************
DECODE_RFC1533 - Decodes RFC1533 header
**************************************************************************/
static int decode_rfc1533(p, block, len, eof)
	register unsigned char *p;
	int block, len, eof;
{
	static unsigned char *extdata = NULL, *extend = NULL;
	unsigned char        *extpath = NULL;
	unsigned char        *end, *q;

	if (block == 0) {
#ifdef IMAGE_MENU
		bzero(imagelist, sizeof(imagelist));
		menudefault = useimagemenu = 0;
		menutmo = -1;
#endif
#ifdef MOTD
		bzero(motd, sizeof(motd));
#endif
		end_of_rfc1533 = NULL;
		vendorext_isvalid = 0;
		if (bcmp(p, rfc1533_cookie, 4))
			return(0); /* no RFC 1533 header found */
		p += 4;
		end = p + len; }
	else {
		if (block == 1) {
			if (bcmp(p, rfc1533_cookie, 4))
				return(0); /* no RFC 1533 header found */
			p += 4;
			len -= 4; }
		if (extend + len <= (unsigned char *)(&bootp_data+1)) {
			bcopy(p, extend, len);
			extend += len;
		} else {
			printf("Overflow in vendor data buffer! Aborting...\r\n");
			*extdata = RFC1533_END;
			return(0);
		}
		p = extdata; end = extend;
	}
	if (eof) {
		while(p < end) {
			unsigned char c = *p;
			if (c == RFC1533_PAD) {p++; continue;}
			else if (c == RFC1533_END) {
				end_of_rfc1533 = end = p; continue; }
			else if (c == RFC1533_NETMASK) {bcopy(p+2,&netmask,4);}
#ifdef	NFS_BOOT
			else if (c == RFC1533_HOSTNAME) {
				if (TAG_LEN(p) < (MNAMELEN&~3)-1) {
				bcopy(p+2,&nfsdiskless.my_hostnam,TAG_LEN(p));
				nfsdiskless.my_hostnam[TAG_LEN(p)] = '\000';
				hostnamelen = (TAG_LEN(p)+3)&~3;
				}
			}
#endif
			else if (c == RFC1533_GATEWAY) {
				/* This is a little simplistic, but it will
				   usually be sufficient; look into the gate-
				   way list, only if there has been no BOOTP
				   gateway. Take only the first entry */
				if (TAG_LEN(p) >= 4 &&
				    arptable[ARP_GATEWAY].ipaddr == 0)
					convert_ipaddr((char *)&arptable[ARP_GATEWAY].ipaddr,
					               p+2);
			}
			else if (c == RFC1533_EXTENSIONPATH)
				extpath = p;
			else if (c == RFC1533_VENDOR_MAGIC &&
				 TAG_LEN(p) >= 6 &&
				 !bcmp(p+2,vendorext_magic,4) &&
				 p[6] == RFC1533_VENDOR_MAJOR)
				vendorext_isvalid++;
#ifdef IMAGE_MENU
			else if (c == RFC1533_VENDOR_MNUOPTS) {
				parse_menuopts(p+2, TAG_LEN(p));
			}
			else if (c >= RFC1533_VENDOR_IMG &&
				 c<RFC1533_VENDOR_IMG+RFC1533_VENDOR_NUMOFIMG){
				imagelist[c - RFC1533_VENDOR_IMG] = p;
				useimagemenu++;
			}
#endif
#ifdef MOTD
			else if (c >= RFC1533_VENDOR_MOTD &&
				 c < RFC1533_VENDOR_MOTD +
				 RFC1533_VENDOR_NUMOFMOTD)
				motd[c - RFC1533_VENDOR_MOTD] = p;
#endif
			else {
				/* printf("Unknown RFC1533-tag ");
				for(q=p;q<p+2+TAG_LEN(p);q++)
					printf("%x ",*q);
				printf("\n\r"); */ }
			p += TAG_LEN(p) + 2;
		}
		extdata = extend = end;
		if (block == 0 && extpath != NULL) {
			char fname[64];
			bcopy(extpath+2,fname,TAG_LEN(extpath));
			fname[(int)TAG_LEN(extpath)] = '\000';
			printf("Loading BOOTP-extension file: %s\r\n",fname);
			tftp(fname,decode_rfc1533);
		}
	}
	return(-1); /* proceed with next block */
}
