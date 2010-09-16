struct grub_net_card
{
  struct grub_net_card *next;
  char *name;
  struct grub_net_card_driver *driver;
  void *data;
};
