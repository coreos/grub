#ifndef __OSDEP_H__
#define __OSDEP_H__

/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 */

#if 1
# define ETHERBOOT32
# include <byteorder.h>
# if 0
#  include <linux-asm-string.h>
# endif
# include <linux-asm-io.h>
#else
#ifdef	__linux__
#define	ETHERBOOT32
#include <asm/byteorder.h>
#include "linux-asm-string.h"
#include "linux-asm-io.h"
#define	_edata	edata			/* ELF does not prepend a _ */
#define	_end	end
#endif

#ifdef	__FreeBSD__
#define	ETHERBOOT32
#include <sys/types.h>
#include "linux-asm-string.h"
#include "linux-asm-io.h"
#define	_edata	edata			/* ELF does not prepend a _ */
#define	_end	end
#endif

#ifdef	__BCC__
#define	ETHERBOOT16
#define	inline
#define	const
#define	volatile
#define	setjmp	_setjmp		/* they are that way in libc.a */
#define	longjmp	_longjmp

/* BCC include files are missing these. */
typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;
typedef unsigned long u_long;
#endif
#endif

#if	!defined(ETHERBOOT16) && !defined(ETHERBOOT32)
Error, neither ETHERBOOT16 nor ETHERBOOT32 defined
#endif

#if	defined(ETHERBOOT16) && defined(ETHERBOOT32)
Error, both ETHERBOOT16 and ETHERBOOT32 defined
#endif

typedef	unsigned long Address;

/* ANSI prototyping macro */
#ifdef	__STDC__
#define	P(x)	x
#else
#define	P(x)	()
#endif

#endif

/*
 * Local variables:
 *  c-basic-offset: 8
 * End:
 */
