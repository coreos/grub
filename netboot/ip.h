int ip_init(void);
int tftp(char *name, int (*fnc)(unsigned char *, int, int, int));

extern int ip_abort;

extern int udp_transmit (unsigned long destip, unsigned int srcsock,
	unsigned int destsock, int len, char *buf);

extern int await_reply (int type, int ival, char *ptr);

extern unsigned short ipchksum (unsigned short *, int len);

extern void rfc951_sleep (int);

extern void twiddle (void);
