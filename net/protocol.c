#include <grub/net/protocol.h>

static grub_net_protocol_t grub_net_protocols;

void grub_protocol_register (grub_net_protocol_t prot)
{
  prot->next = grub_net_protocols;
  grub_net_protocols = prot;
}

void grub_protocol_unregister (grub_net_protocol_t prot)
{
  grub_net_protocol_t *p, q;

  for (p = &grub_net_protocols, q = *p; q; p = &(q->next), q = q->next)
    if (q == prot)
    {
	*p = q->next;
	break;
    }
}
