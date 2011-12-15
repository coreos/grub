/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2010,2011  Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <grub/net.h>
#include <grub/net/udp.h>
#include <grub/command.h>
#include <grub/i18n.h>
#include <grub/err.h>

struct dns_cache_element
{
  char *name;
  grub_size_t naddresses;
  struct grub_net_network_level_address *addresses;
  struct grub_net_network_level_address *source;
  grub_uint64_t limit_time;
};

struct dns_header
{
  grub_uint16_t id;
  grub_uint8_t flags;
  grub_uint8_t ra_z_r_code;
  grub_uint16_t qdcount;
  grub_uint16_t ancount;
  grub_uint16_t nscount;
  grub_uint16_t arcount;
} __attribute__ ((packed));

enum
  {
    FLAGS_RESPONSE = 0x80,
    FLAGS_OPCODE = 0x78,
    FLAGS_RD = 0x01
  };

enum
  {
    ERRCODE_MASK = 0x0f
  };

enum
  {
    DNS_PORT = 53
  };

struct recv_data
{
  grub_size_t *naddresses;
  struct grub_net_network_level_address **addresses;
  int cache;
  grub_uint16_t id;
  int dns_err;
  const char *name;
};

static int
check_name (const grub_uint8_t *name_at, const grub_uint8_t *head,
	    const grub_uint8_t *tail, const char *check_with)
{
  const char *readable_ptr = check_with;
  const grub_uint8_t *ptr;
  int bytes_processed = 0;
  for (ptr = name_at; ptr < tail && bytes_processed < tail - head + 2; )
    {
      /* End marker.  */
      if (!*ptr)
	return (*readable_ptr == 0);
      if (*ptr & 0xc0)
	{
	  bytes_processed += 2;
	  if (ptr + 1 >= tail)
	    return 0;
	  ptr = head + (((ptr[0] & 0x3f) << 8) | ptr[1]);
	  continue;
	}
      if (grub_memcmp (ptr + 1, readable_ptr, *ptr) != 0)
	return 0;
      if (grub_memchr (ptr + 1, 0, *ptr) 
	  || grub_memchr (ptr + 1, '.', *ptr))
	return 0;
      if (readable_ptr[*ptr] != '.' && readable_ptr[*ptr] != 0)
	return 0;
      bytes_processed += *ptr + 1;
      readable_ptr += *ptr;
      if (*readable_ptr)
	readable_ptr++;
      ptr += *ptr + 1;
    }
  return 0;
}

static grub_err_t 
recv_hook (grub_net_udp_socket_t sock __attribute__ ((unused)),
	   struct grub_net_buff *nb,
	   void *data_)
{
  struct dns_header *head;
  struct recv_data *data = data_;
  int i, j;
  grub_uint8_t *ptr;

  head = (struct dns_header *) nb->data;
  ptr = (grub_uint8_t *) (head + 1);
  if (ptr >= nb->tail)
    {
      grub_netbuff_free (nb);
      return GRUB_ERR_NONE;
    }
  
  if (head->id != data->id)
    {
      grub_netbuff_free (nb);
      return GRUB_ERR_NONE;
    }
  if (!(head->flags & FLAGS_RESPONSE) || (head->flags & FLAGS_OPCODE))
    {
      grub_netbuff_free (nb);
      return GRUB_ERR_NONE;
    }
  if (head->ra_z_r_code & ERRCODE_MASK)
    {
      data->dns_err = 1;
      grub_netbuff_free (nb);
      return GRUB_ERR_NONE;
    }
  for (i = 0; i < grub_cpu_to_be16 (head->qdcount); i++)
    {
      if (ptr >= nb->tail)
	{
	  grub_netbuff_free (nb);
	  return GRUB_ERR_NONE;
	}
      while (ptr < nb->tail && !((*ptr & 0xc0) || *ptr == 0))
	ptr += *ptr + 1;
      if (ptr < nb->tail && (*ptr & 0xc0))
	ptr++;
      ptr++;
      ptr += 4;
    }
  *data->addresses = grub_malloc (sizeof ((*data->addresses)[0])
				 * grub_cpu_to_be16 (head->ancount));
  if (!*data->addresses)
    {
      grub_errno = GRUB_ERR_NONE;
      grub_netbuff_free (nb);
      return GRUB_ERR_NONE;
    }
  for (i = 0; i < grub_cpu_to_be16 (head->ancount); i++)
    {
      int ignored = 0;
      int is_ipv6 = 0;
      grub_uint32_t ttl = 0;
      grub_uint16_t length;
      if (ptr >= nb->tail)
	{
	  if (!*data->naddresses)
	    grub_free (*data->addresses);
	  return GRUB_ERR_NONE;
	}
      ignored = !check_name (ptr, nb->data, nb->tail, data->name);
      while (ptr < nb->tail && !((*ptr & 0xc0) || *ptr == 0))
	ptr += *ptr + 1;
      if (ptr < nb->tail && (*ptr & 0xc0))
	ptr++;
      ptr++;
      if (ptr + 10 >= nb->tail)
	{
	  if (!*data->naddresses)
	    grub_free (*data->addresses);
	  grub_netbuff_free (nb);
	  return GRUB_ERR_NONE;
	}
      if (*ptr++ != 0)
	ignored = 1;
      if (*ptr != 1 && *ptr != 28)
	ignored = 1;
      if (*ptr == 28)
	is_ipv6 = 1;
      ptr++;
      if (*ptr++ != 0)
	ignored = 1;
      if (*ptr++ != 1)
	ignored = 1;
      for (j = 0; j < 4; j++)
	{
	  ttl >>= 8;
	  ttl = *ptr++ << 28;
	}
      length = *ptr++ << 8;
      length |= *ptr++;
      if (ptr + length > nb->tail)
	{
	  if (!*data->naddresses)
	    grub_free (*data->addresses);
	  grub_netbuff_free (nb);
	  return GRUB_ERR_NONE;
	}
      if (!ignored && !is_ipv6 && length == 4)
	{
	  (*data->addresses)[*data->naddresses].type
	    = GRUB_NET_NETWORK_LEVEL_PROTOCOL_IPV4;
	  grub_memcpy (&(*data->addresses)[*data->naddresses].ipv4,
		       ptr, 4);
	  (*data->naddresses)++;
	}
      if (!ignored && is_ipv6 && length == 16)
	{
	  (*data->addresses)[*data->naddresses].type
	    = GRUB_NET_NETWORK_LEVEL_PROTOCOL_IPV6;
	  grub_memcpy (&(*data->addresses)[*data->naddresses].ipv6,
		       ptr, 16);
	  (*data->naddresses)++;
	}
      ptr += length;
    }
  grub_netbuff_free (nb);
  return GRUB_ERR_NONE;
}

grub_err_t
grub_net_dns_lookup (const char *name,
		     const struct grub_net_network_level_address *servers,
		     grub_size_t n_servers,
		     grub_size_t *naddresses,
		     struct grub_net_network_level_address **addresses,
		     int cache)
{
  grub_size_t send_servers = 0;
  grub_size_t i, j;
  struct grub_net_buff *nb;
  grub_net_udp_socket_t sockets[n_servers];
  grub_uint8_t *optr;
  const char *iptr;
  struct dns_header *head;
  static grub_uint16_t id = 1;
  grub_err_t err = GRUB_ERR_NONE;
  struct recv_data data = {naddresses, addresses, cache,
			   grub_cpu_to_be16 (id++), 0, name};
  grub_uint8_t *nbd;
  int have_server = 0;

  *naddresses = 0;
  /*  if (cache && cache_lookup (name, servers, n_servers, addresses))
      return GRUB_ERR_NONE;*/

  nb = grub_netbuff_alloc (GRUB_NET_OUR_MAX_IP_HEADER_SIZE
			   + GRUB_NET_MAX_LINK_HEADER_SIZE
			   + GRUB_NET_UDP_HEADER_SIZE
			   + sizeof (struct dns_header)
			   + grub_strlen (name) + 2 + 4
			   + 2 + 4);
  if (!nb)
    return grub_errno;
  grub_netbuff_reserve (nb, GRUB_NET_OUR_MAX_IP_HEADER_SIZE
			+ GRUB_NET_MAX_LINK_HEADER_SIZE
			+ GRUB_NET_UDP_HEADER_SIZE);
  grub_netbuff_put (nb, sizeof (struct dns_header)
		    + grub_strlen (name) + 2 + 4 + 2 + 4);
  head = (struct dns_header *) nb->data;
  optr = (grub_uint8_t *) (head + 1);
  for (iptr = name; *iptr; )
    {
      const char *dot;
      dot = grub_strchr (iptr, '.');
      if (!dot)
	dot = iptr + grub_strlen (iptr);
      if ((dot - iptr) >= 64)
	return grub_error (GRUB_ERR_BAD_ARGUMENT,
			   "domain component is too long");
      *optr = (dot - iptr);
      optr++;
      grub_memcpy (optr, iptr, dot - iptr);
      optr += dot - iptr;
      iptr = dot;
      if (*iptr)
	iptr++;
    }
  *optr++ = 0;

  /* Type: A.  */
  *optr++ = 0;
  *optr++ = 1;

  /* Class.  */
  *optr++ = 0;
  *optr++ = 1;

  /* Compressed name.  */
  *optr++ = 0xc0;
  *optr++ = 0x0c;
  /* Type: AAAA.  */
  *optr++ = 0;
  *optr++ = 28;

  /* Class.  */
  *optr++ = 0;
  *optr++ = 1;

  head->id = data.id;
  head->flags = FLAGS_RD;
  head->ra_z_r_code = 0;
  head->qdcount = grub_cpu_to_be16_compile_time (2);
  head->ancount = grub_cpu_to_be16_compile_time (0);
  head->nscount = grub_cpu_to_be16_compile_time (0);
  head->arcount = grub_cpu_to_be16_compile_time (0);

  nbd = nb->data;

  for (i = 0; i < n_servers * 4; i++)
    {
      /* Connect to a next server.  */
      while (!(i & 1) && send_servers < n_servers)
	{
	  sockets[send_servers] = grub_net_udp_open (servers[send_servers],
						     DNS_PORT,
						     recv_hook,
						     &data);
	  send_servers++;
	  if (!sockets[send_servers - 1])
	    {
	      err = grub_errno;
	      grub_errno = GRUB_ERR_NONE;
	    }
	  else
	    {
	      have_server = 1;
	      break;
	    }
	}
      if (!have_server)
	goto out;
      if (*data.naddresses)
	goto out;
      for (j = 0; j < send_servers; j++)
	{
	  grub_err_t err2;
	  if (!sockets[j])
	    continue;
	  nb->data = nbd;
	  err2 = grub_net_send_udp_packet (sockets[j], nb);
	  if (err2)
	    {
	      grub_errno = GRUB_ERR_NONE;
	      err = err2;
	    }
	  if (*data.naddresses)
	    goto out;
	}
      grub_net_poll_cards (200);
    }
 out:
  grub_netbuff_free (nb);
  for (j = 0; j < send_servers; j++)
    grub_net_udp_close (sockets[j]);
  if (*data.naddresses)
    return GRUB_ERR_NONE;
  if (data.dns_err)
    return grub_error (GRUB_ERR_NET_NO_DOMAIN, "no DNS domain found");
    
  if (err)
    {
      grub_errno = err;
      return err;
    }
  return grub_error (GRUB_ERR_TIMEOUT, "no DNS reply received");
}

static grub_err_t
grub_cmd_nslookup (struct grub_command *cmd __attribute__ ((unused)),
		   int argc, char **args)
{
  grub_err_t err;
  struct grub_net_network_level_address server;
  grub_size_t naddresses, i;
  struct grub_net_network_level_address *addresses;
  if (argc != 2)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "2 arguments expected");
  err = grub_net_resolve_address (args[1], &server);
  if (err)
    return err;

  err = grub_net_dns_lookup (args[0], &server, 1, &naddresses, &addresses, 0);
  if (err)
    return err;
  for (i = 0; i < naddresses; i++)
    {
      char buf[GRUB_NET_MAX_STR_ADDR_LEN];
      grub_net_addr_to_str (&addresses[i], buf);
      grub_printf ("%s\n", buf);
    }
  return GRUB_ERR_NONE;
}

static grub_command_t cmd;

void
grub_dns_init (void)
{
  cmd = grub_register_command ("net_nslookup", grub_cmd_nslookup,
			       "ADDRESS DNSSERVER",
			       N_("perform a DNS lookup"));
}

void
grub_dns_fini (void)
{
  grub_unregister_command (cmd);
}
