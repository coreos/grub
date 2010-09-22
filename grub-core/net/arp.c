#include <grub/net/arp.h>
#include <grub/net/netbuff.h>
#include <grub/mm.h>
#include <grub/net.h>
#include <grub/net/ethernet.h>
#include <grub/net/ip.h>
#include <grub/time.h>

static struct arp_entry arp_table[10];
static grub_int8_t new_table_entry = -1;

static
void arp_init_table(void)
{
  grub_memset (arp_table, 0, sizeof (arp_table)); 
  new_table_entry = 0;
}

static struct arp_entry *
arp_find_entry (const grub_net_network_level_address_t *proto)
{
  unsigned i;
  for(i = 0; i < ARRAY_SIZE (arp_table); i++)
    {
      if(arp_table[i].avail == 1 &&
	 arp_table[i].nl_address.ipv4 == proto->ipv4)
        return &(arp_table[i]);
    }
  return NULL;
}

grub_err_t
grub_net_arp_resolve(struct grub_net_network_level_interface *inf,
		     const grub_net_network_level_address_t *proto_addr, 
		     grub_net_link_level_address_t *hw_addr)
{
  struct arp_entry *entry;
  struct grub_net_buff *nb;
  struct arphdr *arp_header;
  grub_net_link_level_address_t target_hw_addr;
  grub_uint8_t *aux, i;

  /* Check cache table */
  entry = arp_find_entry (proto_addr);
  if (entry)
    {
      *hw_addr = entry->ll_address;
      return GRUB_ERR_NONE;
    }
  /* Build a request packet */
  nb = grub_netbuff_alloc (2048);
  if (!nb)
    return grub_errno;
  grub_netbuff_reserve(nb, 2048);
  grub_netbuff_push(nb, sizeof(*arp_header) + 2 * (6 + 6));
  arp_header = (struct arphdr *)nb->data;
  arp_header->hrd = grub_cpu_to_be16 (GRUB_NET_ARPHRD_ETHERNET);
  arp_header->pro = grub_cpu_to_be16 (GRUB_NET_ETHERTYPE_IP);
  arp_header->hln = 6;
  arp_header->pln = 4;
  arp_header->op = grub_cpu_to_be16 (ARP_REQUEST);
  aux = (grub_uint8_t *)arp_header + sizeof(*arp_header);
  /* Sender hardware address */
  grub_memcpy(aux, &inf->hwaddress.mac, 6);
  aux += 6;
  /* Sender protocol address */
  grub_memcpy(aux, &inf->address.ipv4, 4);
  aux += 4;
  /* Target hardware address */
  for(i = 0; i < 6; i++)
    aux[i] = 0x00;
  aux += 6;
  /* Target protocol address */
  grub_memcpy(aux, &proto_addr->ipv4, 4);

  grub_memset (&target_hw_addr.mac, 0xff, 6);

  send_ethernet_packet (inf, nb, target_hw_addr, GRUB_NET_ETHERTYPE_ARP);
  grub_netbuff_clear(nb); 
  grub_netbuff_reserve(nb, 2048);

  grub_uint64_t start_time, current_time;
  start_time = grub_get_time_ms();
  do
    {
      grub_net_recv_ethernet_packet (inf, nb, GRUB_NET_ETHERTYPE_ARP);
      /* Now check cache table again */
      entry = arp_find_entry(proto_addr);
      if (entry)
        {
          grub_memcpy(hw_addr, &entry->ll_address, sizeof (*hw_addr));
          grub_netbuff_clear(nb); 
          return GRUB_ERR_NONE;
        }
      current_time = grub_get_time_ms();
      if (current_time -  start_time > 3000)
        break;
    } while (! entry);
  grub_netbuff_clear(nb); 
  return grub_error (GRUB_ERR_TIMEOUT, "Timeout: could not resolve hardware address.");
}

grub_err_t
grub_net_arp_receive (struct grub_net_network_level_interface *inf,
		      struct grub_net_buff *nb)
{
  struct arphdr *arp_header = (struct arphdr *)nb->data;
  struct arp_entry *entry;
  grub_uint8_t *sender_hardware_address, *sender_protocol_address;
  grub_uint8_t *target_hardware_address, *target_protocol_address;
  grub_net_network_level_address_t hwaddress;

  sender_hardware_address = (grub_uint8_t *) arp_header + sizeof(*arp_header);
  sender_protocol_address = sender_hardware_address + arp_header->hln;
  target_hardware_address = sender_protocol_address + arp_header->pln;
  target_protocol_address = target_hardware_address + arp_header->hln;
  grub_memcpy (&hwaddress.ipv4, sender_protocol_address, 4);
  /* Check if the sender is in the cache table */
  entry = arp_find_entry(&hwaddress);
  /* Update sender hardware address */
  if (entry)
    grub_memcpy(entry->ll_address.mac, sender_hardware_address, 6);
  else
    {
      /* Add sender to cache table */
      if (new_table_entry == -1)
	arp_init_table();
      entry = &(arp_table[new_table_entry]);
      entry->avail = 1;
      grub_memcpy(&entry->nl_address.ipv4, sender_protocol_address, 4);
      grub_memcpy(entry->ll_address.mac, sender_hardware_address, 6);
      new_table_entry++;
      if (new_table_entry == ARRAY_SIZE (arp_table))
	new_table_entry = 0;
    }

  /* Am I the protocol address target? */
  if (grub_memcmp(target_protocol_address, inf->hwaddress.mac, 6) == 0
      && grub_be_to_cpu16 (arp_header->op) == ARP_REQUEST)
    {
      grub_net_link_level_address_t aux;
      /* Swap hardware fields */
      grub_memcpy(target_hardware_address, sender_hardware_address, arp_header->hln);
      grub_memcpy(sender_hardware_address, inf->hwaddress.mac, 6);
      grub_memcpy(aux.mac, sender_protocol_address, 6);
      grub_memcpy(sender_protocol_address, target_protocol_address, arp_header->pln);
      grub_memcpy(target_protocol_address, aux.mac, arp_header->pln);
      /* Change operation to REPLY and send packet */
      arp_header->op = grub_be_to_cpu16 (ARP_REPLY);
      grub_memcpy (aux.mac, target_hardware_address, 6);
      send_ethernet_packet (inf, nb, aux, GRUB_NET_ETHERTYPE_ARP);
    }
  return GRUB_ERR_NONE;
}
