#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "measurement.h"
#include "part.h"
#include "formulas.h"

#define SHORT_DRAWER 5
#define MEDUIM_DRAWER 7
#define MORTISE_LENGTH 0.25
#define STILE_WIDTH 2.625
#define RAIL_WIDTH_NARROW 1.875
#define RAIL_WIDTH_WIDE 2.625

part *arithmeticProgression(part opening, int count, double increment) {

  part *drawers;
  double x;
  int i;

  /* calculate the list of drawers given the count, total height, and increment
         for the additive progression.  The equation for the progression is as
         follows:

             x + (x + inc) + (x + inc * 2) ... + (x + inc * (count - 1)) = height

         which can be simplified to:

             x = height - (inc(count(count+1))/2)/count) */

  /* allocate memory for the drawer list */

  if ((drawers = malloc(count * sizeof(part))) == NULL) {
    return NULL;
  }

  x = (opening.length - (increment * sum1ToN(count - 1))) / count;
  for (i=0; i<count; i++) {
    drawers[i].length = x;
    drawers[i].width = opening.width;
    x = x + increment;
  }
  return drawers;
}

part *geometricProgression(part opening, int count, double multiplier) {

  part *drawers;
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

  if ((drawers = malloc(count * sizeof(part))) == NULL) {
    return NULL;
  }

  /* Solve for x by dividing the total height by the sum of the multipliers */

  x = opening.length / sumOfConsecutivePowers(multiplier, count);

  /* calculate the drawer heights */

  multiple = 1;
  for (i=0; i<count; i++) {
    drawers[i].length = x * multiple;
    drawers[i].width = opening.width;
    multiple = multiple * multiplier;
  }
  return drawers;
}

part *hambridgeProgression(part *opening, int count) {

  part *drawers;
  int i;

  if ((drawers = malloc(count * sizeof(part))) == NULL) {
    return NULL;
  }
  if (opening->length != 0.0) {
    opening->width = opening->length / (sqrt(count + 1) - 1);
  } else {
    opening->length = (sqrt(count + 1) - 1) * opening->width;
  }
  for (i=count-1; i>=0; i--) {
    drawers[i].length = (opening->width * sqrt(i + 2)) - (opening->width * sqrt(i + 1));
    drawers[i].width = opening->width;
  }
  return drawers;
}

part *fibonacciProgression(part *opening, int count) {

  part *drawers;
  int i;
  double multiplier;
  static double *fib = NULL;
  static int fibcount = 0;

  /* Calculate (count + 3) Fibonacci numbers so that the sum can be calculated */

  if (fibcount < (count + 3)) {
    if ((fib = realloc(fib, (count + 3) * sizeof(double))) == NULL) {
      return NULL;
    }

    /* Initialize the array with the first 2 Fibonacci numbers */

    if (!fibcount) {
      fib[fibcount++] = 0;
      fib[fibcount++] = 1;
    }

    /* Fill or extend the array with the newly needed numbers */

    while (fibcount <= (count + 3)) {
      fib[fibcount] = fib[fibcount -1] + fib[fibcount - 2];
      fibcount++;
    }
    fibcount--;
  }
  if ((drawers = malloc(count * sizeof(part))) == NULL) {
    return NULL;
  }

  /* Divide the openeng height by the sum of Fibonacci numbers for the number
         of drawers required, then use that value to adjust the Fibonacci
         numbers. */

  multiplier = opening->length / (fib[count + 2] - 1);
  for (i=0; i<count; i++) {
    drawers[i].length = fib[i + 1] * multiplier;
    drawers[i].width = opening->width;
  }
  return drawers;
}

void sanitizeHeights(part *drawers, int count, double *height, int precision) {

  int i;
  double fixedHeight;

  /* fix any drawer sizes that don't match the desired level of precision */

  fixedHeight = 0;
  for (i=0; i<count; i++) {
    drawers[i].length = roundFraction(drawers[i].length, precision);
    fixedHeight = fixedHeight + drawers[i].length;
  }

  /* if any heights were changed, and the total is now higher, add one unit of
         measurement to the lower drawers until they match */

  i = count - 1;
  while (i && (fixedHeight < *height)) {
    drawers[i].length = drawers[i].length + 1 / precision;
    fixedHeight = fixedHeight + 1 / precision;
    i--;
  }
  if (*height != 0) {
    i = 0;

    /* if any heights have changed, and the total is now lower, subtract one
           unit of measurement to the upper drawers until they match */

    while ((i < count) && (fixedHeight > *height)) {
      drawers[i].length = drawers[i].length - 1 / precision;
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

double *calculateDrawerOpenings(double width, double height, int count) {

  double workingHeight;
  double *openings;

  if ((openings = malloc(count * sizeof(double))) == NULL) {
    return NULL;
  }

  if ((setting.overlay > 0) && setting.proportionalFace) {
    workingHeight = height - ((count - 1) * (setting.faceMidRail + setting.overlay));
  } else {
    workingHeight = height - ((count -1) * setting.faceMidRail);
  }

  return openings;
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

  part *drawers;
  part opening;

  opening.width = 27;
  opening.length = 30.5;

  drawerCount = 10;
  width = 30;

  height = 0;

  drawers = fibonacciProgression(&opening, 5);
  printf("%s\n", formatMeasurement(opening.length));
  for (drawerNum=0; drawerNum<5; drawerNum++) {
    printf ("%s\n", formatMeasurement(drawers[drawerNum].length));
  }
  sanitizeHeights(drawers,5,&(opening.length),precision);
  printf("%s\n", formatMeasurement(opening.length));
  for (drawerNum=0; drawerNum<5; drawerNum++) {
    printf ("%s\n", formatMeasurement(drawers[drawerNum].length));
  }
  exit(0);



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

   exit(0);
}
