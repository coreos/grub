#include <grub/misc.h>
#include <grub/net/tftp.h>
#include <grub/net/udp.h>
#include <grub/net/ip.h>
#include <grub/net/ethernet.h>
#include <grub/net/netbuff.h>
#include <grub/net/interface.h>
#include <grub/net/type_net.h>
#include <grub/net.h>
#include <grub/mm.h>
#include <grub/dl.h>
#include <grub/file.h>

struct {
  int block_size;
  int size;
} tftp_file;
static  int block;


static char *get_tok_val(char **tok, char **val, char **str_opt,char *end);
static void process_option(char *tok, char *val);

static char *
get_tok_val(char **tok, char **val,char **str_opt,char *end)
{
  char *p = *str_opt;
  *tok = p;
  p += grub_strlen(p) + 1;

  if(p > end)
    return NULL;

  *val = p;
  p += grub_strlen(p) + 1;
  *str_opt = p;
  return *tok;
}

static void
process_option(char *tok, char *val)
{
  if (!grub_strcmp(tok,"blksize"))
  {
    tftp_file.block_size = grub_strtoul (val,NULL,0);
    return;
  }

  if (!grub_strcmp(tok,"tsize"))
  {
    tftp_file.size = grub_strtoul (val,NULL,0);
    return;
  }

}

//void tftp_open (char *options);

/*send read request*/
static grub_err_t 
tftp_open (struct grub_file *file, const char *filename)
{
  struct tftphdr *tftph; 
  char *rrq;
  int rrqlen;
  int hdrlen; 
  struct grub_net_buff nb;
  grub_net_network_level_address_t addr;
  grub_err_t err;

  err = grub_net_resolve_address (file->device->net->name
				  + sizeof ("tftp,") - 1, &addr);
  if (err)
    return err;
  
  grub_memset (&nb, 0, sizeof (nb));
  grub_netbuff_push (&nb,sizeof (*tftph));

  tftph = (struct tftphdr *) nb.data; 
  
  rrq = (char *) tftph->u.rrq;
  rrqlen = 0;
  
  tftph->opcode = TFTP_RRQ;
  grub_strcpy (rrq, filename);
  rrqlen += grub_strlen (filename) + 1;
  rrq +=  grub_strlen (filename) + 1;
  /*passar opcoes como parametro ou usar default?*/
  
  grub_strcpy (rrq,"octet");
  rrqlen += grub_strlen ("octet") + 1;
  rrq +=  grub_strlen ("octet") + 1;

  //grub_strcpy (rrq,"netascii");
  //rrqlen += grub_strlen ("netascii") + 1;
  //rrq +=  grub_strlen ("netascii") + 1;

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
  rrq +=  grub_strlen ("0") + 1;
  hdrlen = sizeof (tftph->opcode) + rrqlen;
 
  grub_netbuff_unput (&nb,nb.tail - (nb.data+hdrlen)); 

  grub_net_send_udp_packet (&addr,
			    &nb, TFTP_CLIENT_PORT, TFTP_SERVER_PORT);

  grub_net_send_udp_packet (&addr, &nb, TFTP_CLIENT_PORT, TFTP_SERVER_PORT);
  /*Receive OACK*/
  grub_netbuff_clear (&nb); 
  grub_netbuff_reserve (&nb,2048);
  file->size = tftp_file.size;

  return grub_net_recv_udp_packet (&addr, &nb,
				   TFTP_CLIENT_PORT, TFTP_SERVER_PORT);
}

static grub_ssize_t
tftp_receive (struct grub_file *file, char *buf, grub_size_t len)
{
  struct tftphdr *tftph;
  char *token,*value,*temp; 
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
  switch (tftph->opcode)
  {
    case TFTP_OACK:
      /*process oack packet*/
      temp = (char *) tftph->u.oack.data;
      while(get_tok_val(&token,&value,&temp,nb.tail))
      {
        process_option(token,value);
      }
       
      //buff_clean
      grub_netbuff_clear(&nb); 
    //  grub_printf("OACK---------------------------------------------------------\n");
      //grub_printf("block_size=%d\n",tftp_file.block_size);
     // grub_printf("file_size=%d\n",tftp_file.size);
     // grub_printf("OACK---------------------------------------------------------\n");
      block = 0;
    break; 
    case TFTP_DATA:
      grub_netbuff_pull (&nb,sizeof (tftph->opcode) + sizeof (tftph->u.data.block));
      if (tftph->u.data.block == block + 1)
	{
	  block  = tftph->u.data.block;
	  grub_memcpy (buf, nb.data, len);
	}
      else
        grub_netbuff_clear(&nb); 
    break; 
    case TFTP_ERROR:
      grub_netbuff_clear (&nb);
      return grub_error (GRUB_ERR_ACCESS_DENIED, (char *)tftph->u.err.errmsg);
    break;
  }

  nb.data = nb.tail = nb.end;
 
  grub_netbuff_push (&nb,sizeof (tftph->opcode) + sizeof (tftph->u.ack.block));

  tftph = (struct tftphdr *) nb.data; 
  tftph->opcode = TFTP_ACK;
  tftph->u.ack.block = block;

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
