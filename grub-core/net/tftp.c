#include <grub/misc.h>
#include <grub/net/tftp.h>
#include <grub/net/udp.h>
#include <grub/net/ip.h>
#include <grub/net/ethernet.h>
#include <grub/net/netbuff.h>
#include <grub/net.h>
#include <grub/mm.h>
#include <grub/dl.h>
#include <grub/file.h>

static grub_err_t 
tftp_open (struct grub_file *file, const char *filename)
{
  struct tftphdr *tftph; 
  char *rrq;
  char *ptr;
  int rrqlen;
  int hdrlen; 
  struct grub_net_buff *nb;
  grub_net_network_level_address_t addr;
  grub_err_t err;

  err = grub_net_resolve_address (file->device->net->name
				  + sizeof ("tftp,") - 1, &addr);
  if (err)
    return err;

  nb = grub_netbuff_alloc (2048);
  if (!nb)
    return grub_errno;

  grub_netbuff_reserve (nb,2048);  
  grub_netbuff_push (nb,sizeof (*tftph));

  tftph = (struct tftphdr *) nb->data; 
  
  rrq = (char *) tftph->u.rrq;
  rrqlen = 0;
  
  tftph->opcode = grub_cpu_to_be16 (TFTP_RRQ);
  grub_strcpy (rrq, filename);
  rrqlen += grub_strlen (filename) + 1;
  rrq +=  grub_strlen (filename) + 1;
  
  grub_strcpy (rrq,"octet");
  rrqlen += grub_strlen ("octet") + 1;
  rrq +=  grub_strlen ("octet") + 1;

  grub_strcpy (rrq,"blksize");
  rrqlen += grub_strlen("blksize") + 1;
  rrq +=  grub_strlen ("blksize") + 1;

  grub_strcpy (rrq,"1024");
  rrqlen += grub_strlen ("1024") + 1;
  rrq +=  grub_strlen ("1024") + 1;
  
  grub_strcpy (rrq,"tsize");
  rrqlen += grub_strlen ("tsize") + 1;
  rrq +=  grub_strlen ("tsize") + 1;

  grub_strcpy (rrq,"0");
  rrqlen += grub_strlen ("0") + 1;
  rrq += grub_strlen ("0") + 1;
  hdrlen = sizeof (tftph->opcode) + rrqlen;
 
  grub_netbuff_unput (nb,nb->tail - (nb->data+hdrlen)); 

  err = grub_net_send_udp_packet (&addr,
				  nb, TFTP_CLIENT_PORT, TFTP_SERVER_PORT);
  if (err)
    return err;

  /* Receive OACK.  */
  grub_netbuff_clear (nb); 
  grub_netbuff_reserve (nb,2048);

  do
    {
      err = grub_net_recv_udp_packet (&addr, nb,
				      TFTP_CLIENT_PORT, TFTP_SERVER_PORT);
      if (err)
	return err;
    }
  while (nb->tail == nb->data);

  file->size = 0;

  for (ptr = nb->data; ptr < nb->tail; )
    grub_printf ("%02x ", *ptr);

  for (ptr = nb->data; ptr < nb->tail; )
    {
      if (grub_memcmp (ptr, "tsize\0=", sizeof ("tsize\0=") - 1) == 0)
	{
	  file->size = grub_strtoul (ptr + sizeof ("tsize\0=") - 1, 0, 0);
	  grub_errno = GRUB_ERR_NONE;
	}
      while (ptr < nb->tail && *ptr)
	ptr++;
      ptr++;
    }
  return GRUB_ERR_NONE;
}

static grub_ssize_t
tftp_receive (struct grub_file *file, char *buf, grub_size_t len)
{
  struct tftphdr *tftph;
  //  char *token,*value,*temp; 
  grub_err_t err;
  grub_net_network_level_address_t addr;
  struct grub_net_buff nb;

  err = grub_net_resolve_address (file->device->net->name
				  + sizeof ("tftp,") - 1, &addr);
  if (err)
    return err;

  grub_net_recv_udp_packet (&addr, &nb,
			    TFTP_CLIENT_PORT, TFTP_SERVER_PORT);
   
  tftph = (struct tftphdr *) nb.data;
  switch (grub_be_to_cpu16 (tftph->opcode))
  {
    case TFTP_DATA:
      grub_netbuff_pull (&nb,sizeof (tftph->opcode) + sizeof (tftph->u.data.block));
      //      if (tftph->u.data.block == block + 1)
      //{
      //  block  = tftph->u.data.block;
	  grub_memcpy (buf, nb.data, len);
	  //}
	  //else
	  //grub_netbuff_clear(&nb); 
      break; 
    case TFTP_ERROR:
      grub_netbuff_clear (&nb);
      return grub_error (GRUB_ERR_IO, (char *)tftph->u.err.errmsg);
  }

  nb.data = nb.tail = nb.end;
 
  grub_netbuff_push (&nb,sizeof (tftph->opcode) + sizeof (tftph->u.ack.block));

  tftph = (struct tftphdr *) nb.data; 
  tftph->opcode = grub_cpu_to_be16 (TFTP_ACK);
  //  tftph->u.ack.block = block;

  return grub_net_send_udp_packet (&addr, &nb, TFTP_CLIENT_PORT, TFTP_SERVER_PORT);
}

static grub_err_t 
tftp_close (struct grub_file *file __attribute__ ((unused)))
{
  return 0;
}

static struct grub_fs grub_tftp_protocol = 
{
  .name = "tftp",
  .open = tftp_open,
  .read = tftp_receive, 
  .close = tftp_close 
};

GRUB_MOD_INIT (tftp)
{
  grub_net_app_level_register (&grub_tftp_protocol);
}

GRUB_MOD_FINI (tftp)
{
  grub_net_app_level_unregister (&grub_tftp_protocol);
}
