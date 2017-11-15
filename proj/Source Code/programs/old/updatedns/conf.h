/*
 *
 *	RADIUS
 *	Remote Authentication Dial In User Service
 *
 * ASCEND: @(#)conf.h	1.2 (95/07/25 00:55:28)
 *
 *
 *	Livingston Enterprises, Inc.
 *	6920 Koll Center Parkway
 *	Pleasanton, CA   94566
 *
 *	Copyright 1992 Livingston Enterprises, Inc.
 *
 *	Permission to use, copy, modify, and distribute this software for any
 *	purpose and without fee is hereby granted, provided that this
 *	copyright and permission notice appear on all copies and supporting
 *	documentation, the name of Livingston Enterprises, Inc. not be used
 *	in advertising or publicity pertaining to distribution of the
 *	program without specific prior permission, and notice be given
 *	in supporting documentation that copying and distribution is by
 *	permission of Livingston Enterprises, Inc.   
 *
 *	Livingston Enterprises, Inc. makes no representations about
 *	the suitability of this software for any purpose.  It is
 *	provided "as is" without express or implied warranty.
 *
 */

/* $Id: conf.h,v 1.2 1996/12/12 00:04:42 baskar Exp $ */

#ifndef _conf_h_
#define _conf_h_

#if defined(__STDC__) && (__STDC__ == 1)
#	define	P__(s)	s
#	define	CONST	const
#else
#	define	P__(s)	(/* s */)
#	define	CONST	/* const */
#endif

#if defined(__alpha) && defined(__osf__)
#define UINT4_IS_UINT
typedef unsigned int	UINT4;
#else
#define UINT4_IS_ULONG
typedef unsigned long	UINT4;
#endif

#if defined(unixware) || defined(sys5) || defined(M_UNIX)
#include        <string.h>
#else   /* unixware */
#if defined(SOLARIS)
#include	<string.h>
#else
#include        <strings.h>
#endif
#endif  /* unixware */

#if defined(BSDI) || defined(FreeBSD)
# include        <sys/types.h>
# include        <stdlib.h>
# if defined(BSDI)
#  include        <machine/inline.h>
#  include        <machine/endian.h>
# endif
#else
# include        <malloc.h>
#endif

#if defined(aix)
#include	<sys/select.h>
#endif	/* aix 	*/

#endif /* _conf_h_ */
