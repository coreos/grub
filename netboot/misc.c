/**************************************************************************
MISC Support Routines
**************************************************************************/

#include "etherboot.h"

/**************************************************************************
SLEEP
**************************************************************************/
void sleep(int secs)
{
	long tmo;

	for (tmo = currticks()+secs*TICKS_PER_SEC; currticks() < tmo; )
		/* Nothing */;
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

#if 0
#ifdef	ETHERBOOT32
/**************************************************************************
STRCASECMP (not entirely correct, but this will do for our purposes)
**************************************************************************/
int strcasecmp(a,b)
	char *a, *b;
{
	while (*a && *b && (*a & ~0x20) == (*b & ~0x20)) {a++; b++; }
	return((*a & ~0x20) - (*b & ~0x20));
}
#endif	/* ETHERBOOT32 */

/**************************************************************************
PRINTF and friends

	Formats:
		%X	- 4 byte ASCII (8 hex digits)
		%x	- 2 byte ASCII (4 hex digits)
		%b	- 1 byte ASCII (2 hex digits)
		%d	- decimal (also %i)
		%c	- ASCII char
		%s	- ASCII string
		%I	- Internet address in x.x.x.x notation
**************************************************************************/
static char hex[]="0123456789ABCDEF";
char *do_printf(buf, fmt, dp)
	char *buf, *fmt;
	int  *dp;
{
	register char *p;
	char tmp[16];
	while (*fmt) {
		if (*fmt == '%') {	/* switch() uses more space */
			fmt++;

			if (*fmt == 'X') {
				long *lp = (long *)dp;
				register long h = *lp++;
				dp = (int *)lp;
				*(buf++) = hex[(h>>28)& 0x0F];
				*(buf++) = hex[(h>>24)& 0x0F];
				*(buf++) = hex[(h>>20)& 0x0F];
				*(buf++) = hex[(h>>16)& 0x0F];
				*(buf++) = hex[(h>>12)& 0x0F];
				*(buf++) = hex[(h>>8)& 0x0F];
				*(buf++) = hex[(h>>4)& 0x0F];
				*(buf++) = hex[h& 0x0F];
			}
			if (*fmt == 'x') {
				register int h = *(dp++);
				*(buf++) = hex[(h>>12)& 0x0F];
				*(buf++) = hex[(h>>8)& 0x0F];
				*(buf++) = hex[(h>>4)& 0x0F];
				*(buf++) = hex[h& 0x0F];
			}
			if (*fmt == 'b') {
				register int h = *(dp++);
				*(buf++) = hex[(h>>4)& 0x0F];
				*(buf++) = hex[h& 0x0F];
			}
			if ((*fmt == 'd') || (*fmt == 'i')) {
				register int dec = *(dp++);
				p = tmp;
				if (dec < 0) {
					*(buf++) = '-';
					dec = -dec;
				}
				do {
					*(p++) = '0' + (dec%10);
					dec = dec/10;
				} while(dec);
				while ((--p) >= tmp) *(buf++) = *p;
			}
			if (*fmt == 'I') {
				union {
					long		l;
					unsigned char	c[4];
				} u;
				long *lp = (long *)dp;
				u.l = *lp++;
				dp = (int *)lp;
				buf = sprintf(buf,"%d.%d.%d.%d",
					u.c[0], u.c[1], u.c[2], u.c[3]);
			}
			if (*fmt == 'c')
				*(buf++) = *(dp++);
			if (*fmt == 's') {
				p = (char *)*dp++;
				while (*p) *(buf++) = *p++;
			}
		} else *(buf++) = *fmt;
		fmt++;
	}
	*buf = 0;
	return(buf);
}

char *sprintf(buf, fmt, data)
	char *fmt, *buf;
	int data;
{
	return(do_printf(buf,fmt, &data));
}

void printf(fmt,data)
	char *fmt;
	int data;
{
	char buf[120],*p;
	p = buf;
	do_printf(buf,fmt,&data);
	while (*p) putchar(*p++);
}

#ifdef	IMAGE_MENU
/**************************************************************************
INET_NTOA - Convert an ascii x.x.x.x to binary form
**************************************************************************/
int inet_ntoa(p, i)
	char *p;
	in_addr *i;
{
	unsigned long ip = 0;
	int val;
	if (((val = getdec(&p)) < 0) || (val > 255)) return(0);
	if (*p != '.') return(0);
	p++;
	ip = val;
	if (((val = getdec(&p)) < 0) || (val > 255)) return(0);
	if (*p != '.') return(0);
	p++;
	ip = (ip << 8) | val;
	if (((val = getdec(&p)) < 0) || (val > 255)) return(0);
	if (*p != '.') return(0);
	p++;
	ip = (ip << 8) | val;
	if (((val = getdec(&p)) < 0) || (val > 255)) return(0);
	i->s_addr = htonl((ip << 8) | val);
	return(1);
}

#endif	/* IMAGE_MENU */
#endif /* 0 */

int getdec(ptr)
	char **ptr;
{
	char *p = *ptr;
	int ret=0;
	if ((*p < '0') || (*p > '9')) return(-1);
	while ((*p >= '0') && (*p <= '9')) {
		ret = ret*10 + (*p - '0');
		p++;
	}
	*ptr = p;
	return(ret);
}

#if 0
#define K_RDWR 		0x60		/* keyboard data & cmds (read/write) */
#define K_STATUS 	0x64		/* keyboard status */
#define K_CMD	 	0x64		/* keybd ctlr command (write-only) */

#define K_OBUF_FUL 	0x01		/* output buffer full */
#define K_IBUF_FUL 	0x02		/* input buffer full */

#define KC_CMD_WIN	0xd0		/* read  output port */
#define KC_CMD_WOUT	0xd1		/* write output port */
#define KB_A20		0xdf		/* enable A20,
					   enable output buffer full interrupt
					   enable data line
					   disable clock line */
#ifndef	IBM_L40

static void empty_8042(void)
{
	extern void slowdownio();
	unsigned long time;
	char st;
  
  	slowdownio();
	time = currticks() + TICKS_PER_SEC;	/* max wait of 1 second */
  	while ((((st = inb(K_CMD)) & K_OBUF_FUL) ||
	       (st & K_IBUF_FUL)) &&
	       currticks() < time)
		inb(K_RDWR);
}
#endif	IBM_L40

/*
 * Gate A20 for high memory
 */
void gateA20()
{
#ifdef	IBM_L40
	outb(0x2, 0x92);
#else	IBM_L40
	empty_8042();
	outb(KC_CMD_WOUT, K_CMD);
	empty_8042();
	outb(KB_A20, K_RDWR);
	empty_8042();
#endif	IBM_L40
}

#ifdef	ETHERBOOT32
/* Serial console is only implemented in ETHERBOOT32 for now */
void
putchar(int c)
{
#ifdef	ANSIESC
	handleansi(c);
	return;
#endif
	if (c == '\n')
		putchar('\r');
#ifdef	SERIAL_CONSOLE
#if	SERIAL_CONSOLE == DUAL
	putc(c);
#endif	/* SERIAL_CONSOLE == DUAL */
	serial_putc(c);
#else
	putc(c);
#endif	/* SERIAL_CONSOLE */
}

int
getchar(int in_buf)
{
	int c=256;

loop:
#ifdef SERIAL_CONSOLE
#if SERIAL_CONSOLE == DUAL
	if (ischar())
		c = getc();
#endif
	if (serial_ischar())
		c = serial_getc();
#else
	if (ischar())
		c = getc();
#endif
	
	if (c==256){
		goto loop;
	}
	
	if (c == '\r')
		c = '\n';
	if (c == '\b') {
		if (in_buf != 0) {
			putchar('\b');
			putchar(' ');
		} else {
			goto loop;
		}
	}
	putchar(c);
	return(c);
}

int
iskey(void)
{
	int isc;

	/*
	 * Checking the keyboard has the side effect of enabling clock
	 * interrupts so that bios_tick works.  Check the keyboard to
	 * get this side effect even if we only want the serial status.
	 */

#ifdef SERIAL_CONSOLE
#if SERIAL_CONSOLE == DUAL
	if (ischar())
		return 1;
#endif
	if (serial_ischar())
		return 1;
#else
	if (ischar())
		return 1;
#endif
	return 0;
}
#endif	/* ETHERBOOT32 */
#endif

/*
 * Local variables:
 *  c-basic-offset: 8
 * End:
 */
