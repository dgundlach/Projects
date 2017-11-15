#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define SIGNIFICANT_DIGITS 6
#define SHORT_DRAWER 5
#define MEDUIM_DRAWER 7
#define MORTISE_LENGTH 0.25
#define STILE_WIDTH 2.625
#define RAIL_WIDTH_NARROW 1.875
#define RAIL_WIDTH_WIDE 2.625

unsigned int gcd(unsigned long u, unsigned long v) {

  unsigned long shift;

  /* GCD(0,v) == v; GCD(u,0) == u, GCD(0,0) == 0 */

  if (u == 0) return v;
  if (v == 0) return u;

  /* Let shift := lg K, where K is the greatest power of 2
        dividing both u and v. */

  for (shift = 0; ((u | v) & 1) == 0; ++shift) {
    u >>= 1;
    v >>= 1;
  }

  while ((u & 1) == 0)
    u >>= 1;

  /* From here on, u is always odd. */

  do {

    /* remove all factors of 2 in v -- they are not common */
    /*   note: v is not zero, so while will terminate */

    while ((v & 1) == 0)  /* Loop X */
      v >>= 1;

    /* Now u and v are both odd. Swap if necessary so u <= v,
       then set v = v - u (which is even). For bignums, the
       swapping is just pointer movement, and the subtraction
       can be done in-place. */

    if (u > v) {
      unsigned long t = v; v = u; u = t;}  // Swap u and v.
    v = v - u;                       // Here v >= u.
  } while (v != 0);

  /* restore common factors of 2 */

  return u << shift;
}

typedef struct measurement {
  unsigned long inches;
  unsigned long numerator;
  unsigned long denominator;
} measurement;

measurement splitMeasurement(double num) {

  measurement measure;
  int i;
  unsigned long divisor;

  measure.numerator = 0;
  measure.denominator = 1;

  /* we don't want the sign */

  num = fabs(num);

  measure.inches = (long)num;
  measure.numerator = 0;
  measure.denominator = 1;

  /* strip the whole number from the fracion */

  num = num - (double)measure.inches;

  /* strip each digit off and add it to the numerator shifted by 1 significant digit.
         increase the denominator by 1 significant digit. */

  for (i=0; (num>0)&&(i<SIGNIFICANT_DIGITS); i++) {
    num = num * 10;
    measure.numerator = measure.numerator * 10 + (long)num;
    measure.denominator = measure.denominator * 10;
    num = num - (long)num;
  }

  /* factor the fraction down to something useful. */

  divisor = gcd(measure.numerator, measure.denominator);
  measure.numerator = measure.numerator / divisor;
  measure.denominator = measure.denominator / divisor;
  return measure;
}

char *formatMeasurement(double size) {

  static char str[256];
  char *p;
  measurement measure;

  measure = splitMeasurement(size);
  p = str;
  if (measure.inches) {
    p += sprintf(p, "%li", measure.inches);
  }
  if (measure.numerator) {

    if (p != str) {
      p += sprintf(p, " ");
    }
    p += sprintf(p, "%li/%li", measure.numerator, measure.denominator);
  }
  return str;
}

/* Arithmetic sum from 1 to n: n(n-1)/2 */

#define sum1ToN(n) (((n) * ((n) + 1)) / 2)

double *arithmeticProgression(double height, int count, double increment) {

  double *drawers;
  double x;
  int i;

  /* calculate the list of drawers given the count, total height, and increment
         for the additive progression.  The equation for the progression is as
         follows:

             x + (x + inc) + (x + inc * 2) ... + (x + inc * (count - 1)) = height

         which can be simplified to:

             x = height - (inc(count(count+1))/2)/count) */

  /* allocate memory for the drawer list */

  if ((drawers = malloc(count * sizeof(double)))) {
    return NULL;
  }

  x = (height - (increment * sum1ToN(count - 1))) / count;
  for (i=0; i<count; i++) {
    drawers[i] = x;
    x = x + increment;
  }
  return drawers;
}

/* Sum of powers from 1 to n: (x^n-1)/(x-1)  The equation is undefined for x=1,
       so just return n. */

#define sumOfConsecutivePowers(x, n) (x == 1 ? n : (pow(x, n) - 1) / (x - 1))

double *geometricProgression(double height, int count, double multiplier) {

  double *drawers;
  double multiple;
  double x;
  int i;

  /* calculate the list of drawers given the count, total height, and multiplier
         for the geometric progression.   The equation for the progression is as
         follows:

            x + x*mul + x*(mul^2) ... + x*(mul^(count -1)) = height

         which can be simplified to:

             x = height/((mul^count-1)/(mul-1)) */

  /* allocate memory for the drawer list */

  if ((drawers = malloc(count * sizeof(double)))) {
    return NULL;
  }

  /* Solve for x by dividing the total height by the sum of the multipliers */

  x = height / sumOfConsecutivePowers(multiplier, count);

  /* calculate the drawer heights */

  multiple = 1;
  for (i=0; i<count; i++) {
    drawers[i] = x * multiple;
    multiple = multiple * multiplier;
  }
  return drawers;
}

double *hambridgeProgression(double width, int count) {

  double *drawers;
  double height;
  double hypotenuse;
  int i;

  /* allocate memory for the drawer list */

  if ((drawers = malloc(count * sizeof(double)))) {
    return NULL;
  }

  height = 0;

  /* the drawer heights are calculated by determining the hypotenuse of a square
         with sides equal to the width of the drawers.  The following drawer
         heights are the hypotenuse of the rectangle defined by the width of the
         drawers and the width plus the sum of all drawers beneath the drawer
         being calculated. */

  for (i=count-1; i>=0; i--) {
    hypotenuse = hypot(width, width + height);
    drawers[i] = hypotenuse - width - height;
    height = hypotenuse - width;
  }
  return drawers;
}

void sanitizeHeights(double *drawers, int count, double *height, int precision) {

  int i;
  double fixedHeight;

  /* fix any drawer sizes that don't match the desired level of precision */

  fixedHeight = 0;
  for (i=0; i<count; i++) {
    drawers[i] = round(drawers[i] * precision) / precision;
    fixedHeight = fixedHeight + drawers[i];
  }

  /* if any heights were changed, and the total is now higher, add one unit of
         measurement to the lower drawers until they match */

  i = count - 1;
  while (i && (fixedHeight < *height)) {
    drawers[i] = drawers[i] + 1 / precision;
    fixedHeight = fixedHeight + 1 / precision;
    i--;
  }
  if (*height != 0) {
    i = 0;

    /* if any heights have changed, and the total is now lower, subtract one
           unit of measurement to the upper drawers until they match */

    while ((i < count) && (fixedHeight > *height)) {
      drawers[i] = drawers[i] - 1 / precision;
      fixedHeight = fixedHeight - 1 / precision;
      i++;
    }
  } else {

    /* the height wasn't predetermined.  return it to the calling function */

    *height = fixedHeight;
  }
}

typedef struct drawerFront {
  double railWidth;
  double railLength;
  double stileWidth;
  double stileLength;
  double panelWidth;
  double panelHeight;
  double mortiseLength;
} drawerFront;

drawerFront calculateDrawerFront(double width, double height) {

  drawerFront front;

  if (height < SHORT_DRAWER) {
    front.railWidth = 0;
    front.railLength = 0;
    front.stileWidth = 0;
    front.stileLength = 0;
    front.mortiseLength = 0;
    front.panelWidth = width;
    front.panelHeight = height;
  } else {
    front.mortiseLength = MORTISE_LENGTH;
    front.stileWidth = STILE_WIDTH;
    front.stileLength = height;
    if (height < MEDUIM_DRAWER) {
      front.railWidth = RAIL_WIDTH_NARROW;
    } else {
      front.railWidth = RAIL_WIDTH_WIDE;
    }
    front.railLength = width - (2 * front.stileWidth) + (2 * front.mortiseLength);
    front.panelWidth = front.railLength;
    front.panelHeight = height - (2 * front.railWidth) + (2 * front.mortiseLength);
  }
  return front;
}

/*
 -a <increment>
 -g <multiplier>
 -h
 -w <width>
 -H <height>
 -s <spacing>
 -f <face>

*/

#define skipSpaces(x) while (*x == ' ') { x++; }

double scanMeasurement(char *measure) {

  char *p1, *p2;
  double measurement = 0;
  double numerator, denominator;

  /* Scan the string measure for a valid measurement.  A valid measurement can be any
         of the following:

             * A whole number
             * A decimal number
             * A fraction
             * A whole number followed by a fraction
             * A whole number, a '-', then a fraction

         There can be any amount of whitespace leading or trailing the number, and if
             there is a whole number and a fraction, there can be any amount of
             whitespace between them.

         -1 is returned if there is an error.
  */

  p1 = measure;
  measurement = (double)strtoul(p1, &p2, 10);
  if (p2 == p1) {
    return -1;
  }
  if (*p2 == '.') {
    p2++;
    numerator = (double)strtoul(p2, &p1, 10);
    if (p1 == p2) {
      return -1;
    }
    measurement = measurement + numerator / pow(10, p1 - p2);
    skipSpaces(p1);
    if (*p1) {
      return -1;
    }
  } else if (*p2 == '/') {
    p2++;
    denominator = (double)strtoul(p2, &p1, 10);
    if ((p2 == p1 ) || (denominator == 0)) {
      return -1;
    }
    skipSpaces(p1);
    if (*p1) {
      return -1;
    }
    measurement = measurement / denominator;
  } else {
    skipSpaces(p2);
    if (*p2) {
      if (*p2 == '-') {
        p2++;
      }
      numerator = (double)strtoul(p2, &p1, 10);
      if (p2 == p1) {
        return -1;
      }
      if (*p1 == '/') {
        p1++;
        denominator = (double)strtoul(p1, &p2, 10);
        skipSpaces(p2);
        if (*p2 || (denominator == 0)) {
          return -1;
        }
        measurement = measurement + numerator / denominator;
      } else {
        return -1;
      }
    }
  }
  return measurement;
}







int main(int argc, char **argv) {

  double width;
  double height;
  double hypotenuse;
  int drawerCount;
  int drawerNum;
  double drawer[100];
//  int num;
//  measurement measure;

  int precision = 16;

  drawerCount = 10;
  width = 30;

  height = 0;

  for (drawerNum = drawerCount; drawerNum > 0; drawerNum--) {
    hypotenuse = hypot(width, width + height);

    /* set the height to something useful. */

    hypotenuse = round(hypotenuse * precision) / precision;
    drawer[drawerNum] = hypotenuse - width - height;
    height = hypotenuse - width;
    printf("%s", formatMeasurement(drawer[drawerNum]));
    printf(" %s\n", formatMeasurement(height));
   }

  hypotenuse = (double)1 / (double)16;
  printf("%7.6f\n",scanMeasurement("   2.5    "));
  printf("%7.6f\n",scanMeasurement("      5/8   "));
  printf("%7.6f\n",scanMeasurement("   2  "));
  printf("%7.6f\n",scanMeasurement("2-1/16"));
  printf("%7.6f\n",scanMeasurement("2  1/4"));


   exit(0);
}
