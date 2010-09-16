#ifndef GRUB_NETBUFF_HEADER
#define GRUB_NETBUFF_HEADER

#include <grub/misc.h>

#define NETBUFF_ALIGN 2048
#define NETBUFFMINLEN 64

struct grub_net_buff
{
  /*Pointer to the start of the buffer*/
  char *head;
  /*Pointer to the data */
  char *data;
  /*Pointer to the tail */
  char *tail;
  /*Pointer to the end of the buffer*/
  char *end;
};

grub_err_t grub_netbuff_put (struct grub_net_buff *net_buff ,grub_size_t len);
grub_err_t grub_netbuff_unput (struct grub_net_buff *net_buff ,grub_size_t len);
grub_err_t grub_netbuff_push (struct grub_net_buff *net_buff ,grub_size_t len);
grub_err_t grub_netbuff_pull (struct grub_net_buff *net_buff ,grub_size_t len);
grub_err_t grub_netbuff_reserve (struct grub_net_buff *net_buff ,grub_size_t len);
grub_err_t grub_netbuff_clear (struct grub_net_buff *net_buff);
struct grub_net_buff * grub_netbuff_alloc ( grub_size_t len );
grub_err_t grub_netbuff_free (struct grub_net_buff *net_buff);
grub_err_t grub_netbuff_clear (struct grub_net_buff *net_buff);
#endif
