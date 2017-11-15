#include <stdio.h>

int main(int argc, char **argv) {

  long fib, fib1 = 0, fib2 = 1;
  int i;

  printf("    %li, %li", fib1, fib2);
  for (i=2; i<24; i++) {
     fib = fib1 + fib2;
     if (!(i % 8)) {
        printf(",\n    %li", fib);
     } else {
       printf(", %li", fib);
     }
     fib1 = fib2;
     fib2 = fib;
  }
  printf("\n");
}
