/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2007  Free Software Foundation, Inc.
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

#ifndef GRUB_OFNET_HEADER
#define GRUB_OFNET_HEADER	1

#include <grub/symbol.h>
#include <grub/err.h>
#include <grub/types.h>

void grub_ofnet_init(void);
void grub_ofnet_fini(void);

struct grub_ofnet
{
  /* The net name.  */
  const char *name;
  
  /* The OF device string.  */
  char *dev;
  /*server ip*/
  char *sip;
  /*client ip*/
  char *cip;
  /*gateway*/
  char *gat;
  /**/
  int type;
};

typedef struct grub_ofnet *grub_ofnet_t;

struct grub_bootp {                                                                                                                           
  grub_uint8_t  op;          /* 1 = BOOTREQUEST, 2 = BOOTREPLY */
  grub_uint8_t  htype;       /* Hardware address type. */
  grub_uint8_t  hlen;        /* Hardware address length */ 
  grub_uint8_t  hops;        /* Used by gateways in cross-gateway booting. */
  grub_uint32_t xid;         /* Transaction ID */ 
  grub_uint16_t secs;        /* Seconds elapsed. */ 
  grub_uint16_t unused;      /* Unused. */
  grub_uint32_t ciaddr;      /* Client IP address, */
  grub_uint32_t yiaddr;      /* Client IP address filled by server.  */
  grub_uint32_t siaddr;      /* Server IP address.  */ 
  grub_uint32_t giaddr;      /* Gateway IP address.  */
  unsigned char chaddr [16];      /*  Client hardware address */
  char sname [64];    /*  Server name */
  char file [128];      /*  Boot filename */
//  grub_uint32_t filesize ; /*File size (testing)*/
  unsigned char vend [64];
}; 

typedef struct grub_bootp* grub_bootp_t;

char * grub_get_filestr(const char * );
char * grub_ip2str (grub_uint32_t ip);
void grub_get_netinfo (grub_ofnet_t netinfo,grub_bootp_t packet);
grub_bootp_t grub_getbootp (void);
#endif /* ! GRUB_NET_HEADER */
