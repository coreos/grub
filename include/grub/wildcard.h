/* wildcard.h  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2010  Free Software Foundation, Inc.
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

#ifndef GRUB_WILDCARD_HEADER
#define GRUB_WILDCARD_HEADER

/* Pluggable wildcard expansion engine.  */
struct grub_wildcard_translator
{
  char *(*escape) (const char *str);
  char *(*unescape) (const char *str);

  grub_err_t (*expand) (const char *str, char ***expansions);

  struct grub_wildcard_translator *next;
};

#endif /* GRUB_WILDCARD_HEADER */
