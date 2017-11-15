#define skipSpaces(x) while (*x == ' ') { x++; }
#define SIGNIFICANT_DIGITS 6

typedef struct measurement {
  unsigned long inches;
  unsigned long numerator;
  unsigned long denominator;
} measurement;

measurement splitMeasurement(double);
char *formatMeasurement(double);
double scanMeasurement(char *);
