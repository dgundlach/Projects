#ifndef _conf_h_
#define _conf_h_

#if defined(__STDC__) && (__STDC__ == 1)
#       define  P__(s)  s
#       define  CONST   const
#else
#       define  P__(s)  (/* s */)
#       define  CONST   /* const */
#endif

#if defined(__alpha) && defined(__osf__)
#define UINT4_IS_UINT
typedef unsigned int    UINT4;
#else
#define UINT4_IS_ULONG
typedef unsigned long   UINT4;
#endif

#if defined(unixware) || defined(sys5) || defined(M_UNIX)
#include        <string.h>
#else   /* unixware */
#if defined(SOLARIS)
#include        <string.h>
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
#include        <sys/select.h>
#endif  /* aix  */

#endif /* _conf_h_ */
