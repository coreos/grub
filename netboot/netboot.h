/**************************************************************************
NETBOOT -  BOOTP/TFTP Bootstrap Program

Author: Martin Renters
  Date: Dec/93

**************************************************************************/

#define NETBOOT32

#include "../stage2/shared.h"

#include "byteorder.h"
#include "io.h"

typedef      unsigned long Address;

extern int _pending_key;
extern inline int getchar(void);
extern inline int getchar(void) {
	int foo;

	if (_pending_key == -1)
		return getkey();
	foo = _pending_key;
	_pending_key = -1;
	return foo;
}
extern inline int iskey(void);
extern inline int iskey(void) {
	return _pending_key != -1 ? 1 : ((_pending_key = checkkey()) != -1);
}

#if 0 /*Edmund*/
#define bcmp __builtin_memcmp
#define memcpy(d,s,sz) bcopy((s),(d),(sz))
#else
#define bcopy(src, dest, n) memcpy(dest, src, n)
#define bzero(s, n) memset(s, 0, n)
#define bcmp(s1, s2, n) memcmp(s1, s2, n)
#endif

/* ANSI prototyping macro */
#ifdef  __STDC__
#define P(x)    x
#else
#define P(x)    ()



#endif

#define ESC		0x1B

#ifndef DEFAULT_BOOTFILE
#define DEFAULT_BOOTFILE	"/kernel"
#endif

#ifndef MAX_TFTP_RETRIES
#define MAX_TFTP_RETRIES	20
#endif

#ifndef MAX_BOOTP_RETRIES
#define MAX_BOOTP_RETRIES	20
#endif

#ifndef MAX_BOOTP_EXTLEN
#define MAX_BOOTP_EXTLEN	1024
#endif

#ifndef MAX_ARP_RETRIES
#define MAX_ARP_RETRIES		20
#endif

#ifndef MAX_RPC_RETRIES
#define MAX_RPC_RETRIES		20
#endif

#ifndef TIMEOUT			/* Inter-packet retry in ticks 18/sec */
#define TIMEOUT			180
#endif

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

#define IP		0x0800
#define ARP		0x0806

#define BOOTP_SERVER	67
#define BOOTP_CLIENT	68
#define TFTP		69
#define SUNRPC		111

#define RPC_SOCKET	620			/* Arbitrary */

#define IP_UDP		17
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

#define RFC1533_VENDOR_MAJOR	0
#define RFC1533_VENDOR_MINOR	0

#define RFC1533_VENDOR_MAGIC	128
#define RFC1533_VENDOR_ADDPARM	129
#define RFC1533_VENDOR_MNUOPTS	160
#define RFC1533_VENDOR_SELECTION 176
#define RFC1533_VENDOR_MOTD	184
#define RFC1533_VENDOR_NUMOFMOTD 8
#define RFC1533_VENDOR_IMG	192
#define RFC1533_VENDOR_NUMOFIMG	16

#define RFC1533_END		255
#define BOOTP_VENDOR_LEN	64

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

#define PROG_PORTMAP	100000
#define PROG_NFS	100003
#define PROG_MOUNT	100005

#define MSG_CALL	0
#define MSG_REPLY	1

#define PORTMAP_LOOKUP	3

#define MOUNT_ADDENTRY	1
#define MOUNT_UMNTALL   4
#define NFS_LOOKUP	4
#define NFS_READ	6

#define NFS_READ_SIZE	1024


#define AWAIT_ARP	0
#define AWAIT_BOOTP	1
#define AWAIT_TFTP	2
#define AWAIT_RPC	3

struct arptable_t {
	unsigned long ipaddr;
	unsigned char node[6];
};

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
	char src[4];
	char dest[4];
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
	char bp_ciaddr[4];
	char bp_yiaddr[4];
	char bp_siaddr[4];
	char bp_giaddr[4];
	char bp_hwaddr[16];
	char bp_sname[64];
	char bp_file[128];
	char bp_vend[BOOTP_VENDOR_LEN];
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

struct rpc_t {
	struct iphdr	ip;
	struct udphdr	udp;
	union {
		char data[1400];
		struct {
			long id;
			long type;
			long rstatus;
			long verifier;
			long v2;
			long astatus;
			long data[1];
		} reply;
	} u;
};

#define TFTP_MIN_PACKET	(sizeof(struct iphdr) + sizeof(struct udphdr) + 4)

/***************************************************************************
RPC Functions
***************************************************************************/
#define PUTLONG(val) {\
	register int lval = val; \
	*(rpcptr++) = ((lval) >> 24); \
	*(rpcptr++) = ((lval) >> 16); \
	*(rpcptr++) = ((lval) >> 8); \
	*(rpcptr++) = (lval); \
	rpclen+=4; }

#if 0				/* XXX */

/***************************************************************************
External prototypes
***************************************************************************/
/* main.c */
extern void load P((void));
extern int load_linux P((int root_mount_port,int swap_mount_port,
	int root_nfs_port,char *kernel_handle));
extern int linux_tftp P((unsigned int block,unsigned char *data,int len));
extern int tftpkernel P((unsigned char *, int, int, int));
extern int tftp P((char *name, int (*)(unsigned char *, int, int, int)));
extern int bootp P((void));
extern int udp_transmit P((unsigned long destip, unsigned int srcsock,
	unsigned int destsock, int len, char *buf));
extern int await_reply P((int type, int ival, char *ptr));
extern void default_netmask P((void));
extern int decode_rfc1533 P((unsigned char *, int, int, int));
extern unsigned short ipchksum P((unsigned short *, int len));
extern void convert_ipaddr P((char *, char *));
extern void rfc951_sleep P((int));

/* bootmenu.c */
extern int execute P((char *string));
extern void bootmenu P((void));
extern void show_motd P((void));
extern void parse_menuopts P((char *,int));
extern int  getoptvalue P((char **, int *, int *));
extern void selectImage P((char **, char *, struct arptable_t *));

/* linuxloader.c */
extern char *linux_add_cmdline P((char *string));

/* rpc.c */
extern int rpclookup P((int addr, int prog, int ver));
extern int nfs_mount P((int server, int port, char *path, char *fh));
extern int nfs_umountall P((int server, int port));
extern int nfs_lookup P((int server, int port, char *fh, char *path, char *file_fh));
extern int nfs_read P((int server, int port, char *fh, int offset, int len, char *buffer));
extern void rpc_err P((struct rpc_t *rpc));
extern void nfs_err P((int err));

/* misc.c */
#if 0 /*Edmund*/
extern void bcopy P((void *src, void *dst, int cnt));
extern void bzero P((void *dst, int cnt));
extern int bcmp P((void *src, void *dst, int cnt));
#endif
extern int strcasecmp P((char *a, char *b));
extern char *substr P((char *a, char *b));
extern int getdec P((char **));
extern void twiddle P((void));
extern void printf();		/* old style to avoid varargs */
extern char *sprintf();
extern int setip P((char *p, unsigned long *i));
extern void gateA20 P((void));

/* start*.S */
extern int getchar P((void));
extern void putchar P((int));
extern int iskey P((void));
extern int getshift P((void));
extern unsigned short memsize P((void));
extern void disk_init P((void));
extern short disk_read P((int drv,int c,int h,int s,char *buf));
extern void start_linux P((void));
extern void xstart P((unsigned long, unsigned long, unsigned long));
extern long currticks P((void));
extern int setjmp P((void *jmpbuf));
extern void longjmp P((void *jmpbuf, int where));
extern void exit P((int status));

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
extern char *kernel;
extern char kernel_buf[];
extern struct nfs_diskless nfsdiskless;
extern int hostnamelen;
extern unsigned long netmask;
extern int    jmp_bootmenu[10];
extern char   kernel_buf[128];
extern struct bootpd_t bootp_data;
extern struct arptable_t arptable[MAX_ARP];
extern char   *imagelist[RFC1533_VENDOR_NUMOFIMG];
extern char   *motd[RFC1533_VENDOR_NUMOFMOTD];
extern int    menutmo,menudefault;
extern unsigned char *end_of_rfc1533;

/* bootmenu.c */

/* linuxloader.c */

/* rpc.c */
extern int rpc_id;

/* created by linker */
extern char _edata[], _end[];

/*
 * Local variables:
 *  c-basic-offset: 8
 * End:
 */
