/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 2002  Free Software Foundation, Inc.
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

#ifndef PUPA_TYPES_HEADER
#define PUPA_TYPES_HEADER	1

#include <config.h>
#include <pupa/cpu/types.h>

#ifdef PUPA_UTIL
# define PUPA_CPU_SIZEOF_VOID_P	SIZEOF_VOID_P
# define PUPA_CPU_SIZEOF_LONG	SIZEOF_LONG
# ifdef WORDS_BIGENDIAN
#  define PUPA_CPU_WORDS_BIGENDIAN	1
# else
#  undef PUPA_CPU_WORDS_BIGENDIAN
# endif
#else /* ! PUPA_UTIL */
# define PUPA_CPU_SIZEOF_VOID_P	PUPA_HOST_SIZEOF_VOID_P
# define PUPA_CPU_SIZEOF_LONG	PUPA_HOST_SIZEOF_LONG
# ifdef PUPA_HOST_WORDS_BIGENDIAN
#  define PUPA_CPU_WORDS_BIGENDIAN	1
# else
#  undef PUPA_CPU_WORDS_BIGENDIAN
# endif
#endif /* ! PUPA_UTIL */

#if PUPA_CPU_SIZEOF_VOID_P != PUPA_CPU_SIZEOF_LONG
# error "This architecture is not supported because sizeof(void *) != sizeof(long)"
#endif

#if PUPA_CPU_SIZEOF_VOID_P != 4 && PUPA_CPU_SIZEOF_VOID_P != 8
# error "This architecture is not supported because sizeof(void *) != 4 and sizeof(void *) != 8"
#endif

/* Define various wide integers.  */
typedef signed char		pupa_int8_t;
typedef short			pupa_int16_t;
typedef int			pupa_int32_t;
#if PUPA_CPU_SIZEOF_VOID_P == 8
typedef long			pupa_int64_t;
#else
typedef long long		pupa_int64_t;
#endif

typedef unsigned char		pupa_uint8_t;
typedef unsigned short		pupa_uint16_t;
typedef unsigned		pupa_uint32_t;
#if PUPA_CPU_SIZEOF_VOID_P == 8
typedef unsigned long		pupa_uint64_t;
#else
typedef unsigned long long	pupa_uint64_t;
#endif

/* Misc types.  */
#if PUPA_HOST_SIZEOF_VOID_P == 8
typedef pupa_uint64_t	pupa_addr_t;
typedef pupa_uint64_t	pupa_off_t;
typedef pupa_uint64_t	pupa_size_t;
typedef pupa_int64_t	pupa_ssize_t;
#else
typedef pupa_uint32_t	pupa_addr_t;
typedef pupa_uint32_t	pupa_off_t;
typedef pupa_uint32_t	pupa_size_t;
typedef pupa_int32_t	pupa_ssize_t;
#endif

/* Byte-orders.  */
#define pupa_swap_bytes16(x)	\
({ \
   pupa_uint16_t _x = (x); \
   (pupa_uint16_t) ((_x << 8) | (_x >> 8)); \
})

#define pupa_swap_bytes32(x)	\
({ \
   pupa_uint32_t _x = (x); \
   (pupa_uint32_t) ((_x << 24) \
                    | ((_x & (pupa_uint32_t) 0xFF00UL) << 8) \
                    | ((_x & (pupa_uint32_t) 0xFF0000UL) >> 8) \
                    | (_x >> 24)); \
})

#define pupa_swap_bytes64(x)	\
({ \
   pupa_uint64_t _x = (x); \
   (pupa_uint64_t) ((_x << 56) \
                    | ((_x & (pupa_uint64_t) 0xFF00ULL) << 40) \
                    | ((_x & (pupa_uint64_t) 0xFF0000ULL) << 24) \
                    | ((_x & (pupa_uint64_t) 0xFF000000ULL) << 8) \
                    | ((_x & (pupa_uint64_t) 0xFF00000000ULL) >> 8) \
                    | ((_x & (pupa_uint64_t) 0xFF0000000000ULL) >> 24) \
                    | ((_x & (pupa_uint64_t) 0xFF000000000000ULL) >> 40) \
                    | (_x >> 56)); \
})

#ifdef PUPA_CPU_WORDS_BIGENDIAN
# define pupa_cpu_to_le16(x)	pupa_swap_bytes16(x)
# define pupa_cpu_to_le32(x)	pupa_swap_bytes32(x)
# define pupa_cpu_to_le64(x)	pupa_swap_bytes64(x)
# define pupa_le_to_cpu16(x)	pupa_swap_bytes16(x)
# define pupa_le_to_cpu32(x)	pupa_swap_bytes32(x)
# define pupa_le_to_cpu64(x)	pupa_swap_bytes64(x)
# define pupa_cpu_to_be16(x)	((pupa_uint16_t) (x))
# define pupa_cpu_to_be32(x)	((pupa_uint32_t) (x))
# define pupa_cpu_to_be64(x)	((pupa_uint64_t) (x))
# define pupa_be_to_cpu16(x)	((pupa_uint16_t) (x))
# define pupa_be_to_cpu32(x)	((pupa_uint32_t) (x))
# define pupa_be_to_cpu64(x)	((pupa_uint64_t) (x))
#else /* ! WORDS_BIGENDIAN */
# define pupa_cpu_to_le16(x)	((pupa_uint16_t) (x))
# define pupa_cpu_to_le32(x)	((pupa_uint32_t) (x))
# define pupa_cpu_to_le64(x)	((pupa_uint64_t) (x))
# define pupa_le_to_cpu16(x)	((pupa_uint16_t) (x))
# define pupa_le_to_cpu32(x)	((pupa_uint32_t) (x))
# define pupa_le_to_cpu64(x)	((pupa_uint64_t) (x))
# define pupa_cpu_to_be16(x)	pupa_swap_bytes16(x)
# define pupa_cpu_to_be32(x)	pupa_swap_bytes32(x)
# define pupa_cpu_to_be64(x)	pupa_swap_bytes64(x)
# define pupa_be_to_cpu16(x)	pupa_swap_bytes16(x)
# define pupa_be_to_cpu32(x)	pupa_swap_bytes32(x)
# define pupa_be_to_cpu64(x)	pupa_swap_bytes64(x)
#endif /* ! WORDS_BIGENDIAN */

#endif /* ! PUPA_TYPES_HEADER */
