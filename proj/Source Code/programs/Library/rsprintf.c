/*
 * $Id: rsprintf.c v1.0 2005-04-08 00:39:02 djg
 *
 * rsprintf, vrsprintf
 *
 * Print to a string according to a format.  If the string is NULL, or
 * is not large enough, (re)allocate it.
 *
 * rsprintf is called with a variable number of arguments following the 
 * format string.
 *
 * vrsprintf is called with a va_list instead of a variable number of 
 * arguments.  The application should call the va_end macro after calling
 * vrsprintf.
 *
 * Parameters:
 *
 *      **str       A pointer to the string variable.
 *      *size       A pointer to the variable containing the string size.
 *      offset      The point in the string to begin writing.
 *      *format     The format in which to print the rest of the arguments.
 *
 * Return value:
 *
 *      The number of characters printed (not including the trailing \0
 *      used to end output strings).  If the string could not be (re)alloced,
 *      -1 is returned.
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

/*
 * Value of SHIFT determines the divisor for allocating the buffer size.
 * Value should be 5 at a minimum, which would allocate 32 byte chunks.
 * Increasing SHIFT by 1 doubles the allocation size.
 */

#define SHIFT     8           // 8 == 256 characters.
#define PAD      32           // Leave a pad of at least 32 characters.

int vrsprintf(char **str, size_t *size, size_t offset,
                                        const char *format, va_list args) {

    char x;
    char *buff = NULL;
    int ns, off;

    if (!str || !size) {
        return -1;
    }

//  Calculate the amount of space we have left in the string.  If the value
//  is zero, there's no more room.  If the value is negative, we're going 
//  to have some undefined characters preceding the output.  This is not
//  necessarily a bad thing, but the programmer has to be careful with what
//  he is doing.

    ns = *size - offset;
    if (*str && ns > 0) {
        buff = *str;        
        off = offset;                        
    } else {            //  No space left.  Point to a temporary buffer.
        ns = 1;
        buff = &x;
        off = 0;
    }

//  Print the string, or at least attempt to.

    ns = offset + vsnprintf(buff + off, ns, format, args);
    if (ns >= *size || buff != *str) {

//      If there wasn't any room in the string, we need to allocate more room
//      and reprint the string.  Pad the value returned by our (now) trial
//      attempt and calculate the chunk of memory to allocate.  The amount of
//      memory allocated is multiple of the allocation size.

        ns = (((ns + PAD) >> SHIFT) + 1) << SHIFT;
        if ((buff = realloc(*str, ns))) {
            *size = ns;
            *str = buff;
        } else {
            return -1;
        }
        ns = vsnprintf(buff + offset, ns, format, args);
    } else {
        ns -= offset;
    }
    return ns;
}


int rsprintf(char **str, size_t *size, size_t offset, 
                                       const char *format, ...) {

    va_list ap;
    int ret;

//  This one is simple.  Just set up a call to the other one.

    va_start(ap, format);
    ret = vrsprintf(str, size, offset, format, ap);
    va_end(ap);
    return ret;
}
