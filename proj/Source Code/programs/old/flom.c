/*
   flom.c - Filter Local Outgoing Mail

   This program filters mail that look like they are local, but have to
   be sent out to the main mailhub because the user is actually on
   another host.  It is passed one argument - the string prepended to the
   recipient name.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFERSIZE	256

int main (int argc, char **argv) {

   char *ln, *p1, *p2, line[BUFFERSIZE], newline[BUFFERSIZE];
   int lineno = 0;

   if (argc != 2) {
      printf("Usage: %s <prepend string>\n", argv[0]);
      exit(1);
   }
   ln = fgets(line, BUFFERSIZE, stdin);
   while (ln) {
      if (lineno == 3) {
         /* Edit the Delivered-To: line */
         bzero(newline, BUFFERSIZE);
         p1 = line;
         p2 = newline;
         while (*p1 != ' ') {
            *p2 = *p1;
            p1++;
            p2++;
         }
         *p2 = *p1;
         p1++;
         strncat(newline, argv[1], BUFFERSIZE);
         strncat(newline, p1, BUFFERSIZE);
         fputs(newline, stdout);
     } 
      else if ((lineno != 1) && (lineno != 2))
         fputs(line, stdout);
      lineno++;
      ln = fgets(line, BUFFERSIZE, stdin);
   }
}
