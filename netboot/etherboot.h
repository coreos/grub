/**************************************************************************
ETHERBOOT -  BOOTP/TFTP Bootstrap Program

Author: Martin Renters
  Date: Dec/93

**************************************************************************/

/* Include GRUB-specific macros and prototypes here.  */
#include <shared.h>

/* FIXME: For now, disable the DHCP support. Perhaps I should segregate
   the DHCP support from the BOOTP support, and permit both to
   co-exist.  */
#define NO_DHCP_SUPPORT	1

#include "osdep.h"

/* These could be customised for different languages perhaps */
#define	ASK_PROMPT	"Boot from (N)etwork or from (L)ocal? "
#define	ANS_NETWORK	'N'
#define	ANS_LOCAL	'L'
#ifndef	ANS_DEFAULT	/* in case left out in Makefile */
#define	ANS_DEFAULT	ANS_NETWORK
#endif

#define	TAGGED_IMAGE		/* eventually optional */
#if	!defined(TAGGED_IMAGE) && !defined(AOUT_IMAGE) && !defined(ELF_IMAGE)
#define	TAGGED_IMAGE		/* choose at least one */
#endif

#define ESC		0x1B

#ifndef DEFAULT_BOOTFILE
#define DEFAULT_BOOTFILE	"/tftpboot/kernel"
#endif

#ifndef MAX_TFTP_RETRIES
#define MAX_TFTP_RETRIES	20
#endif

#ifndef MAX_BOOTP_RETRIES
#define MAX_BOOTP_RETRIES	20
#endif

#ifndef MAX_BOOTP_EXTLEN
#if	defined(INTERNAL_BOOTP_DATA) || (RELOC >= 0x94200)
#define MAX_BOOTP_EXTLEN	1024
#else
#define MAX_BOOTP_EXTLEN	(1024-sizeof(struct bootp_t))
#endif	/* INTERNAL_BOOTP_DATA || RELOC > 0x94200 */
#endif

#ifndef MAX_ARP_RETRIES
#define MAX_ARP_RETRIES		20
#endif

#ifndef MAX_RPC_RETRIES
#define MAX_RPC_RETRIES		20
#endif

#define	TICKS_PER_SEC		18

/* Inter-packet retry in ticks */
#define TIMEOUT			(10*TICKS_PER_SEC)

/* These settings have sense only if compiled with -DCONGESTED */
/* total retransmission timeout in ticks */
#define TFTP_TIMEOUT		(30*TICKS_PER_SEC)
/* packet retransmission timeout in ticks */
#define TFTP_REXMT		(3*TICKS_PER_SEC)

#ifndef NULL
#define NULL	((void *)0)
#endif

#define TRUE		1
#define FALSE		0

#define ETHER_ADDR_SIZE		6	/* Size of Ethernet address */
#define ETHER_HDR_SIZE		14	/* Size of ethernet header */
#define ETH_MIN_PACKET		64
#define ETH_MAX_PACKET		1518

#define VENDOR_NONE	0
#define VENDOR_WD	1
#define VENDOR_NOVELL	2
#define VENDOR_3COM	3
#define VENDOR_3C509	4
#define VENDOR_CS89x0	5

#define FLAG_PIO	0x01
#define FLAG_16BIT	0x02
#define FLAG_790	0x04

#define ARP_CLIENT	0
#define ARP_SERVER	1
#define ARP_GATEWAY	2
#define ARP_ROOTSERVER	3
#define ARP_SWAPSERVER	4
#define MAX_ARP		ARP_SWAPSERVER+1

#define	RARP_REQUEST	3
#define	RARP_REPLY	4

#define IP		0x0800
#define ARP		0x0806
#define	RARP		0x8035

#define BOOTP_SERVER	67
#define BOOTP_CLIENT	68
#define TFTP		69

#define IP_UDP		17
/* Same after going through htonl */
#define IP_BROADCAST	0xFFFFFFFF

#define ARP_REQUEST	1
#define ARP_REPLY	2

#define BOOTP_REQUEST	1
#define BOOTP_REPLY	2

#define TAG_LEN(p)		(*((p)+1))
#define RFC1533_COOKIE		99, 130, 83, 99
#define RFC1533_PAD		0
#define RFC1533_NETMASK		1
#define RFC1533_TIMEOFFSET	2
#define RFC1533_GATEWAY		3
#define RFC1533_TIMESERVER	4
#define RFC1533_IEN116NS	5
#define RFC1533_DNS		6
#define RFC1533_LOGSERVER	7
#define RFC1533_COOKIESERVER	8
#define RFC1533_LPRSERVER	9
#define RFC1533_IMPRESSSERVER	10
#define RFC1533_RESOURCESERVER	11
#define RFC1533_HOSTNAME	12
#define RFC1533_BOOTFILESIZE	13
#define RFC1533_MERITDUMPFILE	14
#define RFC1533_DOMAINNAME	15
#define RFC1533_SWAPSERVER	16
#define RFC1533_ROOTPATH	17
#define RFC1533_EXTENSIONPATH	18
#define RFC1533_IPFORWARDING	19
#define RFC1533_IPSOURCEROUTING	20
#define RFC1533_IPPOLICYFILTER	21
#define RFC1533_IPMAXREASSEMBLY	22
#define RFC1533_IPTTL		23
#define RFC1533_IPMTU		24
#define RFC1533_IPMTUPLATEAU	25
#define RFC1533_INTMTU		26
#define RFC1533_INTLOCALSUBNETS	27
#define RFC1533_INTBROADCAST	28
#define RFC1533_INTICMPDISCOVER	29
#define RFC1533_INTICMPRESPOND	30
#define RFC1533_INTROUTEDISCOVER 31
#define RFC1533_INTROUTESOLICIT	32
#define RFC1533_INTSTATICROUTES	33
#define RFC1533_LLTRAILERENCAP	34
#define RFC1533_LLARPCACHETMO	35
#define RFC1533_LLETHERNETENCAP	36
#define RFC1533_TCPTTL		37
#define RFC1533_TCPKEEPALIVETMO	38
#define RFC1533_TCPKEEPALIVEGB	39
#define RFC1533_NISDOMAIN	40
#define RFC1533_NISSERVER	41
#define RFC1533_NTPSERVER	42
#define RFC1533_VENDOR		43
#define RFC1533_NBNS		44
#define RFC1533_NBDD		45
#define RFC1533_NBNT		46
#define RFC1533_NBSCOPE		47
#define RFC1533_XFS		48
#define RFC1533_XDM		49
#ifndef	NO_DHCP_SUPPORT
#define RFC2132_REQ_ADDR	50
#define RFC2132_MSG_TYPE	53
#define RFC2132_SRV_ID		54
#define RFC2132_PARAM_LIST	55
#define RFC2132_MAX_SIZE	57

#define DHCPDISCOVER		1
#define DHCPOFFER		2
#define DHCPREQUEST		3
#define DHCPACK			5
#endif	/* NO_DHCP_SUPPORT */

#define RFC1533_VENDOR_MAJOR	0
#define RFC1533_VENDOR_MINOR	0

#define RFC1533_VENDOR_MAGIC	128
#define RFC1533_VENDOR_ADDPARM	129
#define RFC1533_VENDOR_HOWTO    132		/* used by FreeBSD */
#define RFC1533_VENDOR_MNUOPTS	160
#define RFC1533_VENDOR_SELECTION 176
#define RFC1533_VENDOR_MOTD	184
#define RFC1533_VENDOR_NUMOFMOTD 8
#define RFC1533_VENDOR_IMG	192
#define RFC1533_VENDOR_NUMOFIMG	16

#define RFC1533_END		255
#define BOOTP_VENDOR_LEN	64
#ifndef	NO_DHCP_SUPPORT
#define DHCP_OPT_LEN		312
#endif	/* NO_DHCP_SUPPORT */

#define	TFTP_DEFAULTSIZE_PACKET	512
#define	TFTP_MAX_PACKET		1432 /* 512 */

#define TFTP_RRQ	1
#define TFTP_WRQ	2
#define TFTP_DATA	3
#define TFTP_ACK	4
#define TFTP_ERROR	5
#define TFTP_OACK	6

#define TFTP_CODE_EOF	1
#define TFTP_CODE_MORE	2
#define TFTP_CODE_ERROR	3
#define TFTP_CODE_BOOT	4
#define TFTP_CODE_CFG	5

#define AWAIT_ARP	0
#define AWAIT_BOOTP	1
#define AWAIT_TFTP	2
#define AWAIT_RARP   3

typedef struct {
	unsigned long	s_addr;
} in_addr;

struct arptable_t {
	in_addr ipaddr;
	unsigned char node[6];
};

/*
 * A pity sipaddr and tipaddr are not longword aligned or we could use
 * in_addr. No, I don't want to use #pragma packed.
 */
struct arprequest {
	unsigned short hwtype;
	unsigned short protocol;
	char hwlen;
	char protolen;
	unsigned short opcode;
	char shwaddr[6];
	char sipaddr[4];
	char thwaddr[6];
	char tipaddr[4];
};

struct iphdr {
	char verhdrlen;
	char service;
	unsigned short len;
	unsigned short ident;
	unsigned short frags;
	char ttl;
	char protocol;
	unsigned short chksum;
	in_addr src;
	in_addr dest;
};

struct udphdr {
	unsigned short src;
	unsigned short dest;
	unsigned short len;
	unsigned short chksum;
};

struct bootp_t {
	struct iphdr ip;
	struct udphdr udp;
	char bp_op;
	char bp_htype;
	char bp_hlen;
	char bp_hops;
	unsigned long bp_xid;
	unsigned short bp_secs;
	unsigned short unused;
	in_addr bp_ciaddr;
	in_addr bp_yiaddr;
	in_addr bp_siaddr;
	in_addr bp_giaddr;
	char bp_hwaddr[16];
	char bp_sname[64];
	char bp_file[128];
#ifdef	NO_DHCP_SUPPORT
	char bp_vend[BOOTP_VENDOR_LEN];
#else
	char bp_vend[DHCP_OPT_LEN];
#endif /* NO_DHCP_SUPPORT */
};

struct bootpd_t {
	struct bootp_t bootp_reply;
	unsigned char  bootp_extension[MAX_BOOTP_EXTLEN];
};

struct tftp_t {
	struct iphdr ip;
	struct udphdr udp;
	unsigned short opcode;
	union {
		char rrq[TFTP_DEFAULTSIZE_PACKET];
		struct {
			unsigned short block;
			char download[TFTP_MAX_PACKET];
		} data;
		struct {
			unsigned short block;
		} ack;
		struct {
			unsigned short errcode;
			char errmsg[TFTP_DEFAULTSIZE_PACKET];
		} err;
		struct {
			char data[TFTP_DEFAULTSIZE_PACKET+2];
		} oack;
	} u;
};

#define TFTP_MIN_PACKET	(sizeof(struct iphdr) + sizeof(struct udphdr) + 4)

#define	ROM_INFO_LOCATION	0x7dfa
/* at end of floppy boot block */

struct rom_info {
	unsigned short	rom_segment;
	unsigned short	rom_length;
};

/***************************************************************************
extern int rarp P((void));
External prototypes
***************************************************************************/
/* main.c */
#if 0
extern void print_bytes P((unsigned char *bytes, int len));
extern void load P((void));
extern int load_linux P((int root_mount_port,int swap_mount_port,
	int root_nfs_port,char *kernel_handle));
extern int tftpkernel P((unsigned char *, int, int, int));
extern int tftp P((char *name, int (*)(unsigned char *, int, int, int)));
#endif
extern int bootp P((void));
extern int rarp P((void));
extern int udp_transmit P((unsigned long destip, unsigned int srcsock,
	unsigned int destsock, int len, char *buf));
extern int await_reply P((int type, int ival, char *ptr, int timeout));
extern int decode_rfc1533 P((unsigned char *, int, int, int));
extern unsigned short ipchksum P((unsigned short *, int len));
extern void rfc951_sleep P((int));

#if 0
/* bootmenu.c */
extern int execute P((char *string));
extern void bootmenu P((int));
extern void show_motd P((void));
extern void parse_menuopts P((char *,int));
extern int  getoptvalue P((char **, int *, int *));
extern void selectImage P((char **));

/* osloader.c */
#if	defined(AOUT_IMAGE) || defined(ELF_IMAGE)
extern int howto;
#endif
extern int os_tftp P((unsigned int block,unsigned char *data,int len));
#endif

/* misc.c */
extern void twiddle P((void));
extern void sleep P((int secs));
#if 0
extern int strcasecmp P((char *a, char *b));
extern char *substr P((char *a, char *b));
#endif
extern int getdec P((char **));
#if 0
extern void printf();		/* old style to avoid varargs */
extern char *sprintf();
extern int inet_ntoa P((char *p, in_addr *i));
extern void gateA20 P((void));
extern void putchar P((int));
extern int getchar ();  
extern int iskey P((void));     

/* start*.S */
extern int getc P((void));
extern void putc P((int));      
extern int ischar P((void));
extern int getshift P((void));
extern unsigned short memsize P((void));
extern void disk_init P((void));
extern unsigned int disk_read P((int drv,int c,int h,int s,char *buf));
extern void xstart P((unsigned long, unsigned long, char *));
extern long currticks P((void));
extern int setjmp P((void *jmpbuf));
extern void longjmp P((void *jmpbuf, int where));
extern void exit P((int status));

/* serial.S */
extern int serial_getc P((void));
extern void serial_putc P((int));
extern int serial_ischar P((void));
extern void serial_init P((void));

/* ansiesc.c */
extern void ansi_reset P((void));
extern void enable_cursor P((int));

/* md5.c */
extern void md5_put P((unsigned int ch));
extern void md5_done P((unsigned char *buf));

/* floppy.c */
extern int bootdisk P((int dev,int part));
#endif

/***************************************************************************
External variables
***************************************************************************/
/* main.c */
extern int ip_abort;
extern int network_ready;
#if 0
extern char *kernel;
extern char kernel_buf[];
#endif
extern struct rom_info rom;
#if 0
extern int hostnamelen;
extern unsigned long netmask;
extern int    jmp_bootmenu[10];
extern char   kernel_buf[128];
#endif
extern struct arptable_t arptable[MAX_ARP];
#if 0
extern char   *imagelist[RFC1533_VENDOR_NUMOFIMG];
extern char   *motd[RFC1533_VENDOR_NUMOFMOTD];
extern int    menutmo,menudefault;
extern struct bootpd_t bootp_data;
#endif
#ifdef	ETHERBOOT32
#define INTERNAL_BOOTP_DATA	1
#ifdef	INTERNAL_BOOTP_DATA
#define	BOOTP_DATA_ADDR	(&bootp_data)
#else
#define	BOOTP_DATA_ADDR	((struct bootpd_t *)0x93C00)
#endif	/* INTERNAL_BOOTP_DATA */
#endif
#ifdef	ETHERBOOT16
#define	BOOTP_DATA_ADDR	(&bootp_data)
#endif
extern unsigned char *end_of_rfc1533;

/* bootmenu.c */

/* osloader.c */

#if 0
/* created by linker */
extern char _edata[], _end[];
#endif

/*
 * Local variables:
 *  c-basic-offset: 8
 * End:
 */
