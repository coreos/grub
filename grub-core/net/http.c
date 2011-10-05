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
  int size_recv;
  grub_net_tcp_socket_t sock;
  char *filename;
  grub_err_t err;
  char *errmsg;
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
	case 404:
	  data->err = GRUB_ERR_FILE_NOT_FOUND;
	  data->errmsg = grub_xasprintf ("file `%s' not found", data->filename);
	  return GRUB_ERR_NONE;
	default:
	  data->err = GRUB_ERR_NET_UNKNOWN_ERROR;
	  data->errmsg = grub_xasprintf ("unsupported HTTP error %d: %s",
					 code, ptr);
	  return GRUB_ERR_NONE;
	}
      data->first_line_recv = 1;
      return GRUB_ERR_NONE;
    }
  if (grub_memcmp (ptr, "Content-Length: ", sizeof ("Content-Length: ") - 1)
      == 0 && !data->size_recv)
    {
      ptr += sizeof ("Content-Length: ") - 1;
      data->file_size = grub_strtoull (ptr, &ptr, 10);
      data->size_recv = 1;
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
    grub_net_tcp_close (data->sock, GRUB_NET_TCP_ABORT);
  if (data->current_line)
    grub_free (data->current_line);
  grub_free (data);
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
	  grub_net_tcp_close (data->sock, GRUB_NET_TCP_ABORT);
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
	  grub_net_tcp_close (data->sock, GRUB_NET_TCP_ABORT);
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
	      grub_net_tcp_close (data->sock, GRUB_NET_TCP_ABORT);
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
	  grub_net_tcp_close (data->sock, GRUB_NET_TCP_ABORT);
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
	  grub_net_tcp_close (data->sock, GRUB_NET_TCP_ABORT);
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
http_establish (struct grub_file *file, grub_off_t offset, int initial)
{
  http_data_t data = file->data;
  grub_uint8_t *ptr;
  int i;
  struct grub_net_buff *nb;
  grub_err_t err;

  nb = grub_netbuff_alloc (GRUB_NET_TCP_RESERVE_SIZE
			   + sizeof ("GET ") - 1
			   + grub_strlen (data->filename)
			   + sizeof (" HTTP/1.1\r\nHost: ") - 1
			   + grub_strlen (file->device->net->server)
			   + sizeof ("\r\nUser-Agent: " PACKAGE_STRING
				     "\r\n") - 1
			   + sizeof ("Content-Range: bytes XXXXXXXXXXXXXXXXXXXX"
				     "-XXXXXXXXXXXXXXXXXXXX/"
				     "XXXXXXXXXXXXXXXXXXXX\r\n\r\n"));
  if (!nb)
    return grub_errno;

  grub_netbuff_reserve (nb, GRUB_NET_TCP_RESERVE_SIZE);
  ptr = nb->tail;
  err = grub_netbuff_put (nb, sizeof ("GET ") - 1);
  if (err)
    {
      grub_netbuff_free (nb);
      return err;
    }
  grub_memcpy (ptr, "GET ", sizeof ("GET ") - 1);

  ptr = nb->tail;

  err = grub_netbuff_put (nb, grub_strlen (data->filename));
  if (err)
    {
      grub_netbuff_free (nb);
      return err;
    }
  grub_memcpy (ptr, data->filename, grub_strlen (data->filename));

  ptr = nb->tail;
  err = grub_netbuff_put (nb, sizeof (" HTTP/1.1\r\nHost: ") - 1);
  if (err)
    {
      grub_netbuff_free (nb);
      return err;
    }
  grub_memcpy (ptr, " HTTP/1.1\r\nHost: ",
	       sizeof (" HTTP/1.1\r\nHost: ") - 1);

  ptr = nb->tail;
  err = grub_netbuff_put (nb, grub_strlen (file->device->net->server));
  if (err)
    {
      grub_netbuff_free (nb);
      return err;
    }
  grub_memcpy (ptr, file->device->net->server,
	       grub_strlen (file->device->net->server));

  ptr = nb->tail;
  err = grub_netbuff_put (nb, 
			  sizeof ("\r\nUser-Agent: " PACKAGE_STRING "\r\n")
			  - 1);
  if (err)
    {
      grub_netbuff_free (nb);
      return err;
    }
  grub_memcpy (ptr, "\r\nUser-Agent: " PACKAGE_STRING "\r\n",
	       sizeof ("\r\nUser-Agent: " PACKAGE_STRING "\r\n") - 1);
  if (!initial)
    {
      ptr = nb->tail;
      grub_snprintf ((char *) ptr,
		     sizeof ("Content-Range: bytes XXXXXXXXXXXXXXXXXXXX-"
			     "XXXXXXXXXXXXXXXXXXXX/XXXXXXXXXXXXXXXXXXXX\r\n"
			     "\r\n"),
		     "Content-Range: bytes %" PRIuGRUB_UINT64_T "-%"
		     PRIuGRUB_UINT64_T "/%" PRIuGRUB_UINT64_T "\r\n\r\n",
		     offset, data->file_size - 1, data->file_size);
      grub_netbuff_put (nb, grub_strlen ((char *) ptr));
    }
  ptr = nb->tail;
  grub_netbuff_put (nb, 2);
  grub_memcpy (ptr, "\r\n", 2);

  data->sock = grub_net_tcp_open (file->device->net->server,
				  HTTP_PORT, http_receive,
				  http_err,
				  file);
  if (!data->sock)
    {
      grub_netbuff_free (nb);
      return grub_errno;
    }

  //  grub_net_poll_cards (5000);

  err = grub_net_send_tcp_packet (data->sock, nb, 1);
  if (err)
    {
      grub_net_tcp_close (data->sock, GRUB_NET_TCP_ABORT);
      return err;
    }

  for (i = 0; !data->headers_recv && i < 100; i++)
    {
      grub_net_tcp_retransmit ();
      grub_net_poll_cards (300);
    }

  if (!data->headers_recv)
    {
      grub_net_tcp_close (data->sock, GRUB_NET_TCP_ABORT);
      if (data->err)
	{
	  char *str = data->errmsg;
	  err = grub_error (data->err, "%s", str);
	  grub_free (str);
	  return data->err;
	}
      return grub_error (GRUB_ERR_TIMEOUT, "timeout opening http");
    }
  return GRUB_ERR_NONE;
}

static grub_err_t
http_seek (struct grub_file *file, grub_off_t off)
{
  struct http_data *old_data, *data;
  grub_err_t err;
  old_data = file->data;
  /* FIXME: Reuse socket?  */
  grub_net_tcp_close (old_data->sock, GRUB_NET_TCP_ABORT);

  while (file->device->net->packs.first)
    grub_net_remove_packet (file->device->net->packs.first);

  data = grub_zalloc (sizeof (*data));
  if (!data)
    return grub_errno;

  data->file_size = old_data->file_size;
  data->size_recv = 1;
  data->filename = old_data->filename;
  if (!data->filename)
    {
      grub_free (data);
      return grub_errno;
    }
  grub_free (old_data);

  err = http_establish (file, off, 0);
  if (err)
    {
      grub_free (data->filename);
      grub_free (data);
      return err;
    }
  return GRUB_ERR_NONE;
}

static grub_err_t
http_open (struct grub_file *file, const char *filename)
{
  grub_err_t err;
  struct http_data *data;

  data = grub_zalloc (sizeof (*data));
  if (!data)
    return grub_errno;

  data->filename = grub_strdup (filename);
  if (!data->filename)
    {
      grub_free (data);
      return grub_errno;
    }

  file->not_easily_seekable = 0;
  file->data = data;

  err = http_establish (file, 0, 1);
  if (err)
    {
      grub_free (data->filename);
      grub_free (data);
      return err;
    }
  file->size = data->file_size;

  return GRUB_ERR_NONE;
}

static grub_err_t
http_close (struct grub_file *file)
{
  http_data_t data = file->data;

  if (data->sock)
    grub_net_tcp_close (data->sock, GRUB_NET_TCP_ABORT);
  if (data->current_line)
    grub_free (data->current_line);
  grub_free (data);
  return GRUB_ERR_NONE;
}

static struct grub_net_app_protocol grub_http_protocol = 
  {
    .name = "http",
    .open = http_open,
    .close = http_close,
    .seek = http_seek
  };

GRUB_MOD_INIT (http)
{
  grub_net_app_level_register (&grub_http_protocol);
}

GRUB_MOD_FINI (http)
{
  grub_net_app_level_unregister (&grub_http_protocol);
}
