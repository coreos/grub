#include <grub/net/arp.h>
#include <grub/net/netbuff.h>
#include <grub/net/interface.h>
#include <grub/mm.h>
#include <grub/net.h>
#include <grub/net/ethernet.h>
#include <grub/net/ip.h>
#include <grub/time.h>
#include <grub/net/ieee1275/interface.h>

static struct arp_entry arp_table[SIZE_ARP_TABLE];
static grub_int8_t new_table_entry = -1;

static void arp_init_table(void)
{
  struct arp_entry *entry = &(arp_table[0]);
  for(;entry<=&(arp_table[SIZE_ARP_TABLE-1]);entry++)
    {
      entry->avail = 0;
      entry->nl_protocol = NULL;
      entry->ll_protocol = NULL;
      entry->nl_address.addr = NULL;
      entry->nl_address.len = 0;
      entry->ll_address.addr = NULL;
      entry->ll_address.len = 0;
    }
  new_table_entry = 0;
}

static struct arp_entry *
arp_find_entry(struct grub_net_network_link_interface *net_link_inf, const void *proto_address, grub_uint8_t address_len)
{
  grub_uint8_t i;
  for(i=0;i < SIZE_ARP_TABLE; i++)
    {
      if(arp_table[i].avail == 1 && 
         arp_table[i].nl_protocol->type == net_link_inf->net_prot->type && 
         arp_table[i].ll_protocol->type == net_link_inf->link_prot->type &&
         (!grub_memcmp(arp_table[i].nl_address.addr, proto_address, address_len)))
        return &(arp_table[i]);
    }
  return NULL;
}

grub_err_t arp_resolve(struct grub_net_network_layer_interface *inf __attribute__ ((unused)), 
struct grub_net_network_link_interface *net_link_inf, struct grub_net_addr *proto_addr, 
struct grub_net_addr *hw_addr)
{
  struct arp_entry *entry;
  struct grub_net_buff *nb;
  struct arphdr *arp_header;
  struct grub_net_addr target_hw_addr;
  grub_uint8_t *aux, i;

  /* Check cache table */
  entry = arp_find_entry(net_link_inf, proto_addr->addr, proto_addr->len);
  if (entry)
    {
      hw_addr->addr = grub_malloc(entry->ll_address.len);
      if (! hw_addr->addr)
        return grub_error (GRUB_ERR_OUT_OF_MEMORY, "fail to alloc memory");
      grub_memcpy(hw_addr->addr, entry->ll_address.addr, entry->ll_address.len);
      hw_addr->len = entry->ll_address.len;
      return GRUB_ERR_NONE;
    }
  /* Build a request packet */
  nb = grub_netbuff_alloc (2048);
  grub_netbuff_reserve(nb, 2048);
  grub_netbuff_push(nb, sizeof(*arp_header) + 2*(inf->card->lla->len + inf->card->ila->len));
  arp_header = (struct arphdr *)nb->data;
  arp_header->hrd = net_link_inf->link_prot->type;
  arp_header->pro = net_link_inf->net_prot->type;
  arp_header->hln = inf->card->lla->len;
  arp_header->pln = inf->card->ila->len;
  arp_header->op = ARP_REQUEST;
  aux = (grub_uint8_t *)arp_header + sizeof(*arp_header);
  /* Sender hardware address */
  grub_memcpy(aux, inf->card->lla->addr, inf->card->lla->len);
  aux += inf->card->lla->len;
  /* Sender protocol address */
  grub_memcpy(aux, inf->card->ila->addr, inf->card->ila->len);
  aux += inf->card->ila->len;
  /* Target hardware address */
  for(i=0; i < inf->card->lla->len; i++)
    aux[i] = 0x00;
  aux += inf->card->lla->len;
  /* Target protocol address */
  grub_memcpy(aux, proto_addr->addr, inf->card->ila->len);

  target_hw_addr.addr = grub_malloc(inf->card->lla->len);
  target_hw_addr.len = inf->card->lla->len;
  for(i=0; i < target_hw_addr.len; i++)
    (target_hw_addr.addr)[i] = 0xFF;
  net_link_inf->link_prot->send(inf, net_link_inf, nb, target_hw_addr, ARP_ETHERTYPE);
  grub_free(target_hw_addr.addr);
  grub_netbuff_clear(nb); 
  grub_netbuff_reserve(nb, 2048);

  grub_uint64_t start_time, current_time;
  start_time = grub_get_time_ms();
  do
    {
      net_link_inf->link_prot->recv(inf, net_link_inf, nb, ARP_ETHERTYPE);
      /* Now check cache table again */
      entry = arp_find_entry(net_link_inf, proto_addr->addr, proto_addr->len);
      if (entry)
        {
          hw_addr->addr = grub_malloc(entry->ll_address.len);
          if (! hw_addr->addr)
            return grub_error (GRUB_ERR_OUT_OF_MEMORY, "fail to alloc memory");
          grub_memcpy(hw_addr->addr, entry->ll_address.addr, entry->ll_address.len);
          hw_addr->len = entry->ll_address.len;
          grub_netbuff_clear(nb); 
          return GRUB_ERR_NONE;
        }
      current_time =  grub_get_time_ms();
      if (current_time -  start_time > TIMEOUT_TIME_MS)
        break;
    } while (! entry);
  grub_netbuff_clear(nb); 
  return grub_error (GRUB_ERR_TIMEOUT, "Timeout: could not resolve hardware address.");
}

grub_err_t arp_receive(struct grub_net_network_layer_interface *inf __attribute__ ((unused)),
struct grub_net_network_link_interface *net_link_inf, struct grub_net_buff *nb)
{
  struct arphdr *arp_header = (struct arphdr *)nb->data;
  struct arp_entry *entry;
  grub_uint8_t merge = 0;

  /* Verify hardware type, protocol type, hardware address length, protocol address length */
  if (arp_header->hrd != net_link_inf->link_prot->type || arp_header->pro != net_link_inf->net_prot->type ||
      arp_header->hln != inf->card->lla->len || arp_header->pln != inf->card->ila->len)
    return GRUB_ERR_NONE;

  grub_uint8_t *sender_hardware_address, *sender_protocol_address, *target_hardware_address, *target_protocol_address;
  sender_hardware_address = (grub_uint8_t *)arp_header + sizeof(*arp_header);
  sender_protocol_address = sender_hardware_address + arp_header->hln;
  target_hardware_address = sender_protocol_address + arp_header->pln;
  target_protocol_address = target_hardware_address + arp_header->hln;
  /* Check if the sender is in the cache table */
  entry = arp_find_entry(net_link_inf, sender_protocol_address, arp_header->pln);
  /* Update sender hardware address */
  if (entry)
    {
      if (entry->ll_address.addr)
        grub_free(entry->ll_address.addr);
      entry->ll_address.addr = grub_malloc(arp_header->hln);
      if (! entry->ll_address.addr)
        return grub_error (GRUB_ERR_OUT_OF_MEMORY, "fail to alloc memory");
      grub_memcpy(entry->ll_address.addr, sender_hardware_address, arp_header->hln);
      entry->ll_address.len = arp_header->hln;
      merge = 1;
    }
  /* Am I the protocol address target? */
  if (! grub_memcmp(target_protocol_address, inf->card->ila->addr, arp_header->pln))
    {
    /* Add sender to cache table */
    if (! merge)
      {
      if (new_table_entry == -1)
        arp_init_table();
      entry = &(arp_table[new_table_entry]);
      entry->avail = 1;
      entry->ll_protocol = net_link_inf->link_prot;
      entry->nl_protocol = net_link_inf->net_prot;
      if (entry->nl_address.addr)
        grub_free(entry->nl_address.addr);
      entry->nl_address.addr = grub_malloc(arp_header->pln);
      if (! entry->nl_address.addr)
        return grub_error (GRUB_ERR_OUT_OF_MEMORY, "fail to alloc memory");
      grub_memcpy(entry->nl_address.addr, sender_protocol_address, arp_header->pln);
      entry->nl_address.len = arp_header->pln;
      if (entry->ll_address.addr)
        grub_free(entry->ll_address.addr);
      entry->ll_address.addr = grub_malloc(arp_header->hln);
      if (! entry->ll_address.addr)
        return grub_error (GRUB_ERR_OUT_OF_MEMORY, "fail to alloc memory");
      grub_memcpy(entry->ll_address.addr, sender_hardware_address, arp_header->hln);
      entry->ll_address.len = arp_header->hln;
      new_table_entry++;
      if (new_table_entry == SIZE_ARP_TABLE)
        new_table_entry = 0;
      }
    if (arp_header->op == ARP_REQUEST)
      {
        struct grub_net_addr aux;
        /* Swap hardware fields */
        grub_memcpy(target_hardware_address, sender_hardware_address, arp_header->hln);
        grub_memcpy(sender_hardware_address, inf->card->lla->addr, arp_header->hln);
        /* Swap protocol fields */
        aux.addr = grub_malloc(arp_header->pln);
        if (! aux.addr)
          return grub_error (GRUB_ERR_OUT_OF_MEMORY, "fail to alloc memory");
        grub_memcpy(aux.addr, sender_protocol_address, arp_header->pln);
        grub_memcpy(sender_protocol_address, target_protocol_address, arp_header->pln);
        grub_memcpy(target_protocol_address, aux.addr, arp_header->pln);
        grub_free(aux.addr);
        /* Change operation to REPLY and send packet */
        arp_header->op = ARP_REPLY;
        aux.addr = target_hardware_address;
        aux.len = arp_header->hln;
        net_link_inf->link_prot->send(inf, net_link_inf, nb, aux, ARP_ETHERTYPE);
      }
    }
  return GRUB_ERR_NONE;
}
