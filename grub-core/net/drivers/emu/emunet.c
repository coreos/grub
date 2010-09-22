#include <grub/net/netbuff.h>
#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h> /* the L2 protocols */
#include <grub/net.h>

static grub_err_t 
card_open (struct grub_net_card *dev)
{
  dev->data_num = socket (AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
  if (dev->data_num < 0)
    return grub_error (GRUB_ERR_IO, "couldn't open packet interface");
  return GRUB_ERR_NONE;
}

static grub_err_t
card_close (struct grub_net_card *dev)
{
  close (dev->data_num);
  return GRUB_ERR_NONE;
}

static grub_err_t 
send_card_buffer (struct grub_net_card *dev, struct grub_net_buff *pack)
{
  ssize_t actual;

  actual = write (dev->data_num, pack->data, pack->tail - pack->data);
  if (actual < 0)
    return grub_error (GRUB_ERR_IO, "couldn't send packets");
  
  return GRUB_ERR_NONE;
}

static grub_err_t 
get_card_packet (struct grub_net_card *dev, struct grub_net_buff *pack)
{
  ssize_t actual;

  grub_netbuff_clear(pack); 
  actual = read (dev->data_num, pack->data, 1500);
  if (actual < 0)
    return grub_error (GRUB_ERR_IO, "couldn't receive packets");
  grub_netbuff_put (pack, actual);

  return GRUB_ERR_NONE; 
}

static struct grub_net_card_driver emudriver = 
{
  .name = "emu",
  .init = card_open,
  .fini = card_close,  
  .send = send_card_buffer, 
  .recv = get_card_packet
};

GRUB_MOD_INIT(emunet)
{
  grub_net_card_driver_register (&emudriver);
}

GRUB_MODE_FINI(emunet)
{
  grub_net_card_driver_unregister (&emudriver);
}



