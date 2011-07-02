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

GRUB_MOD_LICENSE ("GPLv3+");

typedef struct tftp_data
{
  grub_uint64_t file_size;
  grub_uint64_t block;
  grub_uint32_t block_size;
  int have_oack;
  grub_net_socket_t sock;
} *tftp_data_t;

static grub_err_t
tftp_receive (grub_net_socket_t sock __attribute__ ((unused)),
	      struct grub_net_buff *nb,
	      void *f)
{
  grub_file_t file = f;
  struct tftphdr *tftph = (void *) nb->data;
  char nbdata[512];
  tftp_data_t data = file->data;
  grub_err_t err;
  char *ptr;
  struct grub_net_buff nb_ack;

  nb_ack.head = nbdata;
  nb_ack.end = nbdata + sizeof (nbdata);

  tftph = (struct tftphdr *) nb->data;
  switch (grub_be_to_cpu16 (tftph->opcode))
    {
    case TFTP_OACK:
      data->block_size = 512;
      data->have_oack = 1; 
      for (ptr = nb->data + sizeof (tftph->opcode); ptr < nb->tail;)
	{
	  if (grub_memcmp (ptr, "tsize\0", sizeof ("tsize\0") - 1) == 0)
	    {
	      data->file_size = grub_strtoul (ptr + sizeof ("tsize\0") - 1,
					      0, 0);
	    }
	  if (grub_memcmp (ptr, "blksize\0", sizeof ("blksize\0") - 1) == 0)
	    {
	      data->block_size = grub_strtoul (ptr + sizeof ("blksize\0") - 1,
					       0, 0);
	    }
	  while (ptr < nb->tail && *ptr)
	    ptr++;
	  ptr++;
	}
      data->block = 0;
      grub_netbuff_free (nb);
      break;
    case TFTP_DATA:
      err = grub_netbuff_pull (nb, sizeof (tftph->opcode) +
			       sizeof (tftph->u.data.block));
      if (err)
	return err;
      if (grub_be_to_cpu16 (tftph->u.data.block) == data->block + 1)
	{
	  unsigned size = nb->tail - nb->data;
	  data->block++;
	  if (size < data->block_size)
	    {
	      file->device->net->eof = 1;
	    }
	  /* Prevent garbage in broken cards.  */
	  if (size > data->block_size)
	    {
	      err = grub_netbuff_unput (nb, size - data->block_size);
	      if (err)
		return err;
	    }
	  /* If there is data, puts packet in socket list. */
	  if ((nb->tail - nb->data) > 0)
	    grub_net_put_packet (&file->device->net->packs, nb);
	  else
	    grub_netbuff_free (nb);
	}
      else
	{
	  grub_netbuff_free (nb);
	  return GRUB_ERR_NONE;
	}
      break;
    case TFTP_ERROR:
      grub_netbuff_free (nb);
      return grub_error (GRUB_ERR_IO, (char *) tftph->u.err.errmsg);
    }
  grub_netbuff_clear (&nb_ack);
  grub_netbuff_reserve (&nb_ack, 512);
  err = grub_netbuff_push (&nb_ack, sizeof (tftph->opcode)
			   + sizeof (tftph->u.ack.block));
  if (err)
    return err;

  tftph = (struct tftphdr *) nb_ack.data;
  tftph->opcode = grub_cpu_to_be16 (TFTP_ACK);
  tftph->u.ack.block = grub_cpu_to_be16 (data->block);

  err = grub_net_send_udp_packet (data->sock, &nb_ack);
  if (file->device->net->eof)
    {
      grub_net_udp_close (data->sock);
      data->sock = NULL;
    }
  return err;
}

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
  tftp_data_t data;
  grub_err_t err;

  data = grub_zalloc (sizeof (*data));
  if (!data)
    return grub_errno;

  nb.head = open_data;
  nb.end = open_data + sizeof (open_data);
  grub_netbuff_clear (&nb);

  grub_netbuff_reserve (&nb, 1500);
  err = grub_netbuff_push (&nb, sizeof (*tftph));
  if (err)
    return err;

  tftph = (struct tftphdr *) nb.data;

  rrq = (char *) tftph->u.rrq;
  rrqlen = 0;

  tftph->opcode = grub_cpu_to_be16 (TFTP_RRQ);
  grub_strcpy (rrq, filename);
  rrqlen += grub_strlen (filename) + 1;
  rrq += grub_strlen (filename) + 1;

  grub_strcpy (rrq, "octet");
  rrqlen += grub_strlen ("octet") + 1;
  rrq += grub_strlen ("octet") + 1;

  grub_strcpy (rrq, "blksize");
  rrqlen += grub_strlen ("blksize") + 1;
  rrq += grub_strlen ("blksize") + 1;

  grub_strcpy (rrq, "1024");
  rrqlen += grub_strlen ("1024") + 1;
  rrq += grub_strlen ("1024") + 1;

  grub_strcpy (rrq, "tsize");
  rrqlen += grub_strlen ("tsize") + 1;
  rrq += grub_strlen ("tsize") + 1;

  grub_strcpy (rrq, "0");
  rrqlen += grub_strlen ("0") + 1;
  rrq += grub_strlen ("0") + 1;
  hdrlen = sizeof (tftph->opcode) + rrqlen;

  err = grub_netbuff_unput (&nb, nb.tail - (nb.data + hdrlen));
  if (err)
    return err;

  file->not_easily_seekable = 1;
  file->data = data;
  data->sock = grub_net_udp_open (file->device->net->server,
				  TFTP_SERVER_PORT, tftp_receive,
				  file);
  if (!data->sock)
    return grub_errno;

  err = grub_net_send_udp_packet (data->sock, &nb);
  if (err)
    {
      grub_net_udp_close (data->sock);
      return err;
    }

  /* Receive OACK packet.  */
  for (i = 0; i < 3; i++)
    {
      grub_net_poll_cards (100);
      if (grub_errno)
	return grub_errno;
      if (data->have_oack)
	break;
      /* Retry.  */
      err = grub_net_send_udp_packet (data->sock, &nb);
      if (err)
	{
	  grub_net_udp_close (data->sock);
	  return err;
	}
    }

  if (!data->have_oack)
    {
      grub_net_udp_close (data->sock);
      return grub_error (GRUB_ERR_TIMEOUT, "Time out opening tftp.");
    }
  file->size = data->file_size;

  return GRUB_ERR_NONE;
}

static grub_err_t
tftp_close (struct grub_file *file)
{
  tftp_data_t data = file->data;
  if (data->sock)
    grub_net_udp_close (data->sock);
  grub_free (data);
  return GRUB_ERR_NONE;
}

static struct grub_net_app_protocol grub_tftp_protocol = 
  {
    .name = "tftp",
    .open = tftp_open,
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
