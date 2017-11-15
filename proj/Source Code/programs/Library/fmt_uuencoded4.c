#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define enc(x) (((x) & 077) + ' ')

unsigned long fmt_uuencoded(char* dest,const char* src,unsigned long len) {
  const char* s=(const unsigned char*) src;
  const char* orig=dest;
  unsigned short bits=0,temp=0,i;
  if (!dest) {
    return (((len+2)/3)*4) + (((len+44)/45)*2);
  }
  while (len) {
    i=(len>45) ? 45 : len; *dest++=enc(i); len-=i;
    for (; i; --i) {
      temp<<=8; temp+=*s++; bits+=8;
      while (bits>=6) {
        *dest++=enc(temp>>(bits-6));
        bits-=6;
      }
    }
    if (bits) {
      *dest++=enc(temp<<(6-bits));
      *dest++='*';
      if (bits==2) *dest++='*';
    }
    *dest++='\n';
  }
  return dest-orig;
}

int main(int argc, char **argv) {

    int f;
    char buffer_in[4096], buffer_out[4096];
    unsigned long si, so;

    f = open("fmt_uuencoded2.c", O_RDONLY);
    while ((si = read(f, buffer_in, 3000))) {
        printf("%uli\n", si);
        so = fmt_uuencoded(NULL, buffer_in, si);
        printf("%uli\n", so);
        so = fmt_uuencoded(buffer_out, buffer_in, si);
        printf("%uli\n", so);
        buffer_out[so]='\0';
        printf("%s", buffer_out);
    }
    close(f);
    exit(0);
}
