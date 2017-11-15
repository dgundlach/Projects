#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv) {

    printf("%i\n", getpagesize());
    exit(0);
}
