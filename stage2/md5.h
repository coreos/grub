/* md5.h - an implementation of the MD5 algorithm and MD5 crypt */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2000  Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* Initialize the buffers to compute a new md5 checksum.  This will
   destroy any previously calculated md5 sum. */
extern void md5_init(void);

/* Digestify the given input.  This may be called multiple times. */
extern void md5_update(const char *input, int inputlen);

/* Calculate the 16 byte md5 check sum.  The result will be valid until
   the next md5_init(). */
extern unsigned char* md5_final(void);

/* Check a md5 password for validity.  Returns 0 if password was
   correct, and a value != 0 for error, similarly to strcmp. */
extern int check_md5_password (const char* key, const char* crypted);
