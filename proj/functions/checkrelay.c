/***************************************************************************
 *
 * checkrelay.c
 *
 * This program is usually called by inetd or xinetd and replaces the
 * the functionality of tcpd and tcp-env.  It checks the address of the 
 * calling host with a control list in /var/qmail/control/relayclients.
 * The list is laid out as follows:
 *
 * address/netmask bits
 *
 * Exmaple:
 *
 * 127.0.0.1/8
 * 216.89.164.128/25
 * 209.176.168.0/24
 *
 * Author: Dan Gundlach <dan@msl.net>
 * License: GPL
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define CONFFILE "/var/qmail/control/relayclients"

typedef struct network {
  unsigned int base;
  unsigned int netmask;
} network;

unsigned int masks[33];

network *scan_line(char * line)
{
  char *slash, *next;
  network *net;
  struct in_addr addr;
  int mindex;

  if (!line) {
    return NULL;
  }

 /*
  * Allocate the network structure and initialize.
  */

  net = (struct network *)malloc(sizeof(struct network));
  net->base = 0;
  net->netmask = masks[32];

 /*
  * If there's a slash, look up the netmask
  */

  if ((slash = strchr(line,'/'))) {
    *slash++ = '\0';
    mindex = strtoul(slash,&next,10);
    if (mindex > 32) {
      free(net);
      return NULL;
    }
    net->netmask = masks[mindex];
  }

 /*
  * Convert the ip addresss from ascii to network byte order.
  */

  if (!inet_aton(line,&addr)) {
    free(net);
    return NULL;
  }
  net->base = addr.s_addr;

 /*
  * Compute the network address.
  */

  net->base = net->base & net->netmask;
  return net;
}

int main(int argc, char **argv)
{
  unsigned int i = 0x80000000U;
  unsigned int j,k;
  char *buff;
  char *line, *next;
  struct network *net;
  unsigned int remotenet;
  struct stat st;
  int f,len;
  struct sockaddr si;
  struct sockaddr_in *sp;
  int dummy;

 /*
  * Build the table of netmasks.
  */

  if (argc < 2) {
    exit(1);
  }
  k = i;
  masks[0] = 0;
  for(j=1; j<33; j++) {
    masks[j] = htonl(i);
    k = k >> 1;
    i = i + k;
  }

 /*
  * Get the address of our peer.
  */

  dummy = sizeof(si);
  sp = (struct sockaddr_in *) &si;
  if ((getpeername(0,&si,&dummy)) == -1) {
    exit(1);
  }
  remotenet = sp->sin_addr.s_addr;

  if (stat(CONFFILE,&st) != -1) {

   /*
    * Allocate a buffer to hold the entire file.
    */

    buff = malloc(st.st_size + 2);
    *buff = '\0';

   /*
    * Open the configuration file, and read it all into one big buffer.
    */

    if ((f = open(CONFFILE,O_RDONLY))) {
      len = read(f,buff,st.st_size);
      close(f);
      line = buff;

     /*
      * Loop for each line.
      */

      while (*line) {

       /*
        * Find the end of the line.
        */

        next = strchr(line,'\n');
        if (next) {
          *next++ = '\0';
        } else {
          next = line;
          while (*next) {
            next++;
          }
        }

       /*
        * If we have a valid network address, compare it with the remote
        * ip address.
        */

        if ((net = scan_line(line))) {
          if ((remotenet & net->netmask) == net->base) {

           /*
            * They match.  Set the environment up and escape out of the loop.
            */

            free(net);
            setenv("RELAYCLIENT","",1);
            break;
          }
          free(net);
        }

       /*
        * Get ready to check the next line.
        */

        line = next;
      }
      free(buff);
    }
  }

 /*
  * Execute the argument(s) passed on the command line.
  */

  execvp(argv[1],argv + 1);
  exit(111);
}
