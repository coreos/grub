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
  int i;
  int rrqlen;
  int hdrlen;
  char open_data[1500]; 
  struct grub_net_buff nb;
  tftp_data_t data = grub_malloc (sizeof *data);
  grub_err_t err;
 
  file->device->net->socket->data = (void *) data;
  nb.head = open_data;
  nb.end = open_data + sizeof (open_data);
  grub_netbuff_clear (&nb);  

  grub_netbuff_reserve (&nb,1500);  
  grub_netbuff_push (&nb,sizeof (*tftph));

  tftph = (struct tftphdr *) nb.data; 
  
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
 
  grub_netbuff_unput (&nb, nb.tail - (nb.data + hdrlen)); 

  file->device->net->socket->out_port = TFTP_SERVER_PORT;

  err = grub_net_send_udp_packet (file->device->net->socket, &nb);
  if (err)
    return err;

  /* Receive OACK packet.  */
  for ( i = 0; i < 3; i++)
    {
      grub_net_pool_cards (100);
      if (grub_errno)
	return grub_errno;
      if (file->device->net->socket->status != 0)
	break;
      /* Retry.  */
      //err = grub_net_send_udp_packet (file->device->net->socket, &nb);
     // if (err)
     //   return err;
    }

  if (file->device->net->socket->status == 0)
    return grub_error (GRUB_ERR_TIMEOUT,"Time out opening tftp.");
  file->size = data->file_size;

  return GRUB_ERR_NONE;
}

static grub_err_t 
tftp_receive (grub_net_socket_t sock, struct grub_net_buff *nb)
{
  struct tftphdr *tftph;
  char  nbdata[128];
  tftp_data_t data = sock->data;
  grub_err_t err;
  char *ptr;
  struct grub_net_buff nb_ack;

  nb_ack.head = nbdata;
  nb_ack.end = nbdata + sizeof (nbdata);


  tftph = (struct tftphdr *) nb->data;
  switch (grub_be_to_cpu16 (tftph->opcode))
  {
    case TFTP_OACK:
      for (ptr = nb->data; ptr < nb->tail; )
	{
	  if (grub_memcmp (ptr, "tsize\0", sizeof ("tsize\0") - 1) == 0)
	    data->file_size = grub_strtoul (ptr + sizeof ("tsize\0") - 1, 0, 0);
	  while (ptr < nb->tail && *ptr)
	    ptr++;
	  ptr++;
	}
      sock->status = 1; 
      data->block = 0;
      grub_netbuff_clear(nb);
    break;
    case TFTP_DATA:
      grub_netbuff_pull (nb,sizeof (tftph->opcode) + sizeof (tftph->u.data.block));
      
      if (grub_be_to_cpu16 (tftph->u.data.block) == data->block + 1)
	{
	  data->block++;
	  unsigned size = nb->tail - nb->data;
	  if (size < 1024)
	    sock->status = 2;
	  /* Prevent garbage in broken cards.  */
	  if (size > 1024)
	    grub_netbuff_unput (nb, size - 1024); 
	}
      else
	grub_netbuff_clear(nb);
      
    break; 
    case TFTP_ERROR:
      grub_netbuff_clear (nb);
      return grub_error (GRUB_ERR_IO, (char *)tftph->u.err.errmsg);
    break;
  }
  grub_netbuff_clear (&nb_ack);
  grub_netbuff_reserve (&nb_ack,128);
  grub_netbuff_push (&nb_ack,sizeof (tftph->opcode) + sizeof (tftph->u.ack.block));

  tftph = (struct tftphdr *) nb_ack.data; 
  tftph->opcode = grub_cpu_to_be16 (TFTP_ACK);
  tftph->u.ack.block = data->block;

  err = grub_net_send_udp_packet (sock, &nb_ack);
  return err;
}

static grub_err_t 
tftp_close (struct grub_file *file __attribute__ ((unused)))
{
  return 0;
}

static struct grub_net_app_protocol grub_tftp_protocol = 
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
