#include "../stage2/filesys.h"

#include "netboot.h"
#include "netboot_config.h"
#include "nic.h"

#include "ip.h"

#if 0

#define BUFSIZE (20*TFTP_DEFAULTSIZE_PACKET)
static char buf[BUFSIZE];

#else 

/* use GRUB's file system buffer */
#define BUFSIZE (32*1024)
#define buf ((char *)(FSYS_BUF))

#endif

static int buf_read = 0, buf_eof = 0;
static unsigned long buf_fileoff;

static int             retry;
static unsigned short  isocket = 2000;
static unsigned short  osocket;
static unsigned short  len, block, prevblock;
static struct tftp_t  *tr;
static struct tftp_t   tp;
static int	       packetsize = TFTP_DEFAULTSIZE_PACKET;

static int buf_fill(int abort);

int tftp_mount(void)
{
  if (current_drive != 0x20)	/* only mount network drives */
    return 0;

  if (!ip_init())
    return 0;  

  return 1;
}

/* read "size" bytes from file position "filepos" to "addr" */
int tftp_read(char *addr, int size)
{
  int ret = 0;

  if (filepos < buf_fileoff)
    {
      printf("tftp_read: can't read from filepos=%d, buf_fileoff=%d\n",
	     filepos, buf_fileoff);
      errnum = ERR_BOOT_FAILURE;
      return 0;
    }

  while (size > 0)
    {
      if (filepos < buf_fileoff + buf_read)
	{
	  /* can copy stuff from buffer */
	  int tocopy = buf_fileoff + buf_read - filepos;
	  if (tocopy > size) tocopy = size;
	  bcopy(buf + (filepos - buf_fileoff), (void*) addr, tocopy);
	  size -= tocopy;
	  addr += tocopy;
	  filepos += tocopy;
	  ret += tocopy;
	  
	  if (buf_eof && (filepos + size >= buf_fileoff + buf_read))
	    break;		/* end of file */
	      
	  continue;
	}
      else if ((filepos == buf_fileoff + buf_read) && buf_eof)
	break;			/* end of file */
	
      if (buf_eof)		/* filepos beyond end of file */
	{
	  errnum = ERR_READ;
	  return 0;
	}

      /* move buffer contents forward by 1/2 buffer size */
      bcopy(buf + BUFSIZE/2, buf, BUFSIZE/2);
      buf_fileoff += BUFSIZE/2;
      buf_read -= BUFSIZE/2;

      if (! buf_fill(0))		/* read more data */
	{
	  errnum = ERR_READ;
	  return 0;
	}
    }

  return ret;
}

int tftp_dir(char *dirname)
{
  char name[100], *np;

  if (print_possibilities)
    {
      printf(" TFTP doesn't support listing the directory; Sorry!\n");
      return 1;
    }

  /* open the file */
  np = name;
  while (dirname && *dirname && !isspace(*dirname))
    *np++ = *dirname++;
  *np = 0;

  buf_eof = buf_read = buf_fileoff = 0;

  retry = 0;
  block = 0;
  prevblock = 0;
  packetsize = TFTP_DEFAULTSIZE_PACKET;

  /* send tftp read request */
  tp.opcode = htons(TFTP_RRQ);
  len = 30 + sprintf((char *)tp.u.rrq, "%s%coctet%cblksize%c%d",
		     name, 0, 0, 0, TFTP_MAX_PACKET) + 1;

  if (!udp_transmit(arptable[ARP_SERVER].ipaddr, ++isocket, TFTP,
		    len, (char *)&tp))
    {
      printf("tftp_dir: can't transmit TFTP read request\n");
      errnum = ERR_BOOT_FAILURE;
      return (0);
    }
  if (! buf_fill(0))
    {
      printf("tftp_dir: can't read from file\n");
      errnum = ERR_FILE_NOT_FOUND;
      return 0;
    }

/*   filemax = 234620; */
  filemax = 16L*1024*1024;	/* XXX 16MB is max filesize */

  return 1;
}

#if 0
void tftp_close(void)
{
  buf_read = 0;
  buf_fill(1);			/* abort. */
}
#endif

static int buf_fill(int abort)
{
  while (!buf_eof && (buf_read + packetsize <= BUFSIZE))
    {
      /* read a packet from the network */
      if (!await_reply(AWAIT_TFTP, isocket, NULL))
	{
	  if (ip_abort)
	    return 0;

	  if (prevblock == 0 && retry++ < MAX_TFTP_RETRIES)
	    {			/* maybe initial request was lost */
	      rfc951_sleep(retry);
	      if (!udp_transmit(arptable[ARP_SERVER].ipaddr,
				++isocket, TFTP, len, (char *)&tp))
		return (0);
	      continue;
	    }
	  return 0;			/* timeout on other blocks */
	}
      
      /* we got a packet */
      
      tr = (struct tftp_t *)&nic.packet[ETHER_HDR_SIZE];
      
      if (tr->opcode == ntohs(TFTP_ERROR)) /* error? */
	{
	  printf("TFTP error %d (%s)\r\n",
		 ntohs(tr->u.err.errcode),
		 tr->u.err.errmsg);
	  return 0;
	}  
      else if (tr->opcode == ntohs(TFTP_OACK)) 
	{
	  continue;		/* ignore */
	}
      else if (tr->opcode == ntohs(TFTP_DATA)) 
	{
	  retry = MAX_TFTP_RETRIES;	/*no more retries! */
	  len = ntohs(tr->udp.len) - sizeof(struct udphdr) - 4;
	  if (len > packetsize)	/* shouldn't happen */
	    continue;		/* ignore it */
	  block = ntohs(tp.u.ack.block = tr->u.data.block); 
	}
      else /* neither TFTP_OACK nor TFTP_DATA */
	return 0;
		
      tp.opcode = abort ? htons(TFTP_ERROR) : htons(TFTP_ACK);
      osocket = ntohs(tr->udp.src);
      udp_transmit(arptable[ARP_SERVER].ipaddr, isocket,
		   osocket, TFTP_MIN_PACKET, (char *)&tp);	/* ack */

      if (abort)
	{
	  buf_eof = 1;
	  break;
	}

      if (block <= prevblock)	/* retransmission or OACK */
	continue;		/* don't process */
      prevblock = block;

      bcopy(tr->u.data.download, buf + buf_read, len);
      buf_read += len;

      if (len < packetsize)		/* End of data */
	buf_eof = 1;
    }

  return 1;
}
