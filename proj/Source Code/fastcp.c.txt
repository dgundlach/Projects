/*

Simple file copy program using sendfile(2).

usage: fastcp <source> <destination>

Copyright (C) 2003 Jeff Tranter.


This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/


#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/sendfile.h>
#include <sys/stat.h>

int main (int argc, char** argv)
{
  int src;               /* file descriptor for source file */
  int dest;              /* file descriptor for destination file */
  struct stat stat_buf;  /* hold information about input file */
  off_t offset = 0;      /* byte offset used by sendfile */
  int rc;                /* return code from sendfile */

  /* check for two command line arguments */
  if (argc != 3) {
    fprintf(stderr, "usage: %s <source> <destination>\n", argv[0]);
    exit(1);
  }

  /* check that source file exists and can be opened */
  src = open(argv[1], O_RDONLY);
  if (src == -1) {
    fprintf(stderr, "unable to open '%s': %s\n", argv[1], strerror(errno));
    exit(1);
  }

 /* get size and permissions of the source file */
  fstat(src, &stat_buf);

 /* open destination file */
  dest = open(argv[2], O_WRONLY|O_CREAT, stat_buf.st_mode);
  if (dest == -1) {
    fprintf(stderr, "unable to open '%s': %s\n", argv[2], strerror(errno));
    exit(1);
  }

 /* copy file using sendfile */
 rc = sendfile (dest, src, &offset, stat_buf.st_size);
 if (rc == -1) {
    fprintf(stderr, "error from sendfile: %s\n", strerror(errno));
    exit(1);
 }
 if (rc != stat_buf.st_size) {
   fprintf(stderr, "incomplete transfer from sendfile: %d of %d bytes\n",
           rc,
           (int)stat_buf.st_size);
   exit(1);
 }

 /* clean up and exit */
 close(dest);
 close(src);

 return 0;
}
