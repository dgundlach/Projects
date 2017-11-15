#include <time.h>
#include <string.h>

#define DAY		86400

char *strptime(const char *, const char *, struct tm *);

static char *date_formats[] = {
	"%Y-%m-%d", NULL
};

long StrToDay(const char *str) {

  struct tm tp;
  char * const *fmt;
  char *cp;
  time_t result;

  bzero(&tp,sizeof(tp));
  for (fmt = date_formats; *fmt; fmt++) {
    cp = strptime(str,*fmt,&tp);
    if (!cp || *cp != '\0') {
      continue;
    }
    if ((result = mktime(&tp)) == (time_t) -1) {
      continue;
    }
    return result / DAY;
  }
  return -1; 
}


