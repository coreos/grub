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

#include <grub/misc.h>
#include <grub/net/tcp.h>
#include <grub/net/ip.h>
#include <grub/net/ethernet.h>
#include <grub/net/netbuff.h>
#include <grub/net.h>
#include <grub/mm.h>
#include <grub/dl.h>
#include <grub/file.h>

GRUB_MOD_LICENSE ("GPLv3+");

enum
  {
    HTTP_PORT = 80
  };


typedef struct http_data
{
  grub_uint64_t file_size;
  grub_uint64_t position;
  char *current_line;
  grub_size_t current_line_len;
  int headers_recv;
  int first_line_recv;
  grub_net_tcp_socket_t sock;
} *http_data_t;

static grub_err_t
parse_line (http_data_t data, char *ptr, grub_size_t len)
{
  char *end = ptr + len;
  while (end > ptr && *(end - 1) == '\r')
    end--;
  *end = 0;
  if (ptr == end)
    {
      data->headers_recv = 1;
      return GRUB_ERR_NONE;
    }

  if (!data->first_line_recv)
    {
      int code;
      if (grub_memcmp (ptr, "HTTP/1.1 ", sizeof ("HTTP/1.1 ") - 1) != 0)
	return grub_error (GRUB_ERR_NET_INVALID_RESPONSE,
			   "unsupported HTTP response");
      ptr += sizeof ("HTTP/1.1 ") - 1;
      code = grub_strtoul (ptr, &ptr, 10);
      if (grub_errno)
	return grub_errno;
      switch (code)
	{
	case 200:
	  break;
	default:
	  return grub_error (GRUB_ERR_NET_UNKNOWN_ERROR,
			     "unsupported HTTP error %d: %s",
			     code, ptr);
	}
      data->first_line_recv = 1;
      return GRUB_ERR_NONE;
    }
  if (grub_memcmp (ptr, "Content-Length: ", sizeof ("Content-Length: ") - 1)
      == 0)
    {
      ptr += sizeof ("Content-Length: ") - 1;
      data->file_size = grub_strtoull (ptr, &ptr, 10);
      return GRUB_ERR_NONE;
    }
  return GRUB_ERR_NONE;  
}

static void
http_err (grub_net_tcp_socket_t sock __attribute__ ((unused)),
	  void *f)
{
  grub_file_t file = f;
  http_data_t data = file->data;

  if (data->sock)
    grub_net_tcp_close (data->sock);
  grub_free (data);
  if (data->current_line)
    grub_free (data->current_line);
  file->device->net->eof = 1;
}

static grub_err_t
http_receive (grub_net_tcp_socket_t sock __attribute__ ((unused)),
	      struct grub_net_buff *nb,
	      void *f)
{
  grub_file_t file = f;
  http_data_t data = file->data;
  char *ptr = (char *) nb->data;
  grub_err_t err;

  if (!data->headers_recv && data->current_line)
    {
      int have_line = 1;
      char *t;
      ptr = grub_memchr (nb->data, '\n', nb->tail - nb->data);
      if (ptr)
	ptr++;
      else
	{
	  have_line = 0;
	  ptr = (char *) nb->tail;
	}
      t = grub_realloc (data->current_line,
			data->current_line_len + (ptr - (char *) nb->data));
      if (!t)
	{
	  grub_netbuff_free (nb);
	  grub_net_tcp_close (data->sock);
	  return grub_errno;
	}
	      
      data->current_line = t;
      grub_memcpy (data->current_line + data->current_line_len,
		   nb->data, ptr - (char *) nb->data);
      data->current_line_len += ptr - (char *) nb->data;
      if (!have_line)
	{
	  grub_netbuff_free (nb);
	  return GRUB_ERR_NONE;
	}
      err = parse_line (data, data->current_line, data->current_line_len);
      grub_free (data->current_line);
      data->current_line = 0;
      data->current_line_len = 0;
      if (err)
	{
	  grub_net_tcp_close (data->sock);
	  grub_netbuff_free (nb);
	  return err;
	}
    }

  while (ptr < (char *) nb->tail && !data->headers_recv)
    {
      char *ptr2;
      ptr2 = grub_memchr (ptr, '\n', (char *) nb->tail - ptr);
      if (!ptr2)
	{
	  data->current_line = grub_malloc ((char *) nb->tail - ptr);
	  if (!data->current_line)
	    {
	      grub_netbuff_free (nb);
	      grub_net_tcp_close (data->sock);
	      return grub_errno;
	    }
	  data->current_line_len = (char *) nb->tail - ptr;
	  grub_memcpy (data->current_line, ptr, data->current_line_len);
	  grub_netbuff_free (nb);
	  return GRUB_ERR_NONE;
	}
      err = parse_line (data, ptr, ptr2 - ptr);
      if (err)
	{
	  grub_net_tcp_close (data->sock);
	  grub_netbuff_free (nb);
	  return err;
	}
      ptr = ptr2 + 1;
    }
	  
  if (((char *) nb->tail - ptr) > 0)
    {
      data->position += ((char *) nb->tail - ptr);
      err = grub_netbuff_pull (nb, ptr - (char *) nb->data);
      if (err)
	{
	  grub_net_tcp_close (data->sock);
	  grub_netbuff_free (nb);
	  return err;
	}      
      grub_net_put_packet (&file->device->net->packs, nb);
    }
  else
    grub_netbuff_free (nb);
  return GRUB_ERR_NONE;
}

static grub_err_t
http_open (struct grub_file *file, const char *filename)
{
  struct grub_net_buff *nb;
  http_data_t data;
  grub_err_t err;
  grub_uint8_t *ptr;
  int i;

  data = grub_zalloc (sizeof (*data));
  if (!data)
    return grub_errno;

  nb = grub_netbuff_alloc (GRUB_NET_TCP_RESERVE_SIZE
			   + sizeof ("GET ") - 1
			   + grub_strlen (filename)
			   + sizeof (" HTTP/1.1\r\nHost: ") - 1
			   + grub_strlen (file->device->net->server)
			   + sizeof ("\r\nUser-Agent: " PACKAGE_STRING
				     "\r\n\r\n") - 1);
  if (!nb)
    {
      grub_free (data);
      return grub_errno;
    }

  grub_netbuff_reserve (nb, GRUB_NET_TCP_RESERVE_SIZE);
  ptr = nb->tail;
  err = grub_netbuff_put (nb, sizeof ("GET ") - 1);
  if (err)
    {
      grub_free (data);
      grub_netbuff_free (nb);
      return err;
    }
  grub_memcpy (ptr, "GET ", sizeof ("GET ") - 1);

  ptr = nb->tail;
  err = grub_netbuff_put (nb, grub_strlen (filename));
  if (err)
    {
      grub_free (data);
      grub_netbuff_free (nb);
      return err;
    }
  grub_memcpy (ptr, filename, grub_strlen (filename));

  ptr = nb->tail;
  err = grub_netbuff_put (nb, sizeof (" HTTP/1.1\r\nHost: ") - 1);
  if (err)
    {
      grub_free (data);
      grub_netbuff_free (nb);
      return err;
    }
  grub_memcpy (ptr, " HTTP/1.1\r\nHost: ",
	       sizeof (" HTTP/1.1\r\nHost: ") - 1);

  ptr = nb->tail;
  err = grub_netbuff_put (nb, grub_strlen (file->device->net->server));
  if (err)
    {
      grub_free (data);
      grub_netbuff_free (nb);
      return err;
    }
  grub_memcpy (ptr, file->device->net->server,
	       grub_strlen (file->device->net->server));

  ptr = nb->tail;
  err = grub_netbuff_put (nb, 
			  sizeof ("\r\nUser-Agent: " PACKAGE_STRING "\r\n\r\n")
			  - 1);
  if (err)
    {
      grub_free (data);
      grub_netbuff_free (nb);
      return err;
    }
  grub_memcpy (ptr, "\r\nUser-Agent: " PACKAGE_STRING "\r\n\r\n",
	       sizeof ("\r\nUser-Agent: " PACKAGE_STRING "\r\n\r\n") - 1);

  file->not_easily_seekable = 1;
  file->data = data;

  data->sock = grub_net_tcp_open (file->device->net->server,
				  HTTP_PORT, http_receive,
				  http_err,
				  file);
  if (!data->sock)
    {
      grub_free (data);
      grub_netbuff_free (nb);
      return grub_errno;
    }

  //  grub_net_poll_cards (5000);

  err = grub_net_send_tcp_packet (data->sock, nb, 1);
  if (err)
    {
      grub_free (data);
      grub_net_tcp_close (data->sock);
      return err;
    }

  for (i = 0; !data->headers_recv && i < 100; i++)
    {
      grub_net_tcp_retransmit ();
      grub_net_poll_cards (300);
    }

  if (!data->headers_recv)
    {
      grub_net_tcp_close (data->sock);
      grub_free (data);
      return grub_error (GRUB_ERR_TIMEOUT, "Time out opening http.");
    }
  file->size = data->file_size;

  return GRUB_ERR_NONE;
}

static grub_err_t
http_close (struct grub_file *file)
{
  http_data_t data = file->data;

  if (data->sock)
    grub_net_tcp_close (data->sock);
  grub_free (data);
  if (data->current_line)
    grub_free (data->current_line);
  return GRUB_ERR_NONE;
}

static struct grub_net_app_protocol grub_http_protocol = 
  {
    .name = "http",
    .open = http_open,
    .close = http_close
  };

GRUB_MOD_INIT (http)
{
  grub_net_app_level_register (&grub_http_protocol);
}

GRUB_MOD_FINI (http)
{
  grub_net_app_level_unregister (&grub_http_protocol);
}
