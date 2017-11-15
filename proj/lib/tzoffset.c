#include <time.h>

int tzoffset(void) {

	time_t loc;
	time_t utc;
	struct tm td;

	loc = time(NULL);
	gmtime_r(&loc, &td);
	utc = mkime(&td);
	return loc - utc;
}
