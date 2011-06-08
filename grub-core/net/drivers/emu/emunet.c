
#include <grub/dl.h>
#include <grub/net/netbuff.h>
#include <sys/socket.h>
#include <grub/net.h>
#include <sys/types.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <grub/term.h>

static int fd;

static grub_err_t
send_card_buffer (struct grub_net_card *dev __attribute__ ((unused)),
		  struct grub_net_buff *pack)
{
  ssize_t actual;

  actual = write (fd, pack->data, pack->tail - pack->data);
  if (actual < 0)
    return grub_error (GRUB_ERR_IO, "couldn't send packets");

  return GRUB_ERR_NONE;
}

static grub_ssize_t
get_card_packet (struct grub_net_card *dev __attribute__ ((unused)),
		 struct grub_net_buff *pack)
{
  ssize_t actual;

  grub_netbuff_clear (pack);
  actual = read (fd, pack->data, 1500);
  if (actual < 0)
    return -1;
  grub_netbuff_put (pack, actual);

  return actual;
}

static struct grub_net_card_driver emudriver = 
  {
    .name = "emu",
    .send = send_card_buffer,
    .recv = get_card_packet
  };

static struct grub_net_card emucard = 
  {
    .name = "emu0",
    .driver = &emudriver,
    .default_address = {
			 .type = GRUB_NET_LINK_LEVEL_PROTOCOL_ETHERNET,
			 {.mac = {0, 1, 2, 3, 4, 5}}
		       },
    .flags = 0
  };

GRUB_MOD_INIT(emunet)
{
  struct ifreq ifr;
  fd = open ("/dev/net/tun", O_RDWR | O_NONBLOCK);
  if (fd < 0)
    return;
  grub_memset (&ifr, 0, sizeof (ifr));
  ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
  if (ioctl (fd, TUNSETIFF, &ifr) < 0)
    {
      close (fd);
      fd = -1;
      return;
    }
  grub_net_card_register (&emucard);
}

GRUB_MOD_FINI(emunet)
{
  if (fd >= 0)
    {
      close (fd);
      grub_net_card_unregister (&emucard);
    }
}
