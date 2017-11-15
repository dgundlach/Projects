#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "measurement.h"

#define DEFAULT_WIDTH 2.625
#define DEFAULT_MORTISE_LENGTH 0.25
#define SHORT_WIDTH 1.875

void usage(char *program) {
  fprintf(stderr, "Usage: %s [-r <rail width>] [-w <rail width (short)]\n \
    [-s <stile width>] [-m <mortise length] width height\n", program);
  exit(1);
}

double getMeasurement(char *str) {

  double measure;

  measure = scanMeasurement(str);
  if (measure == -1) {
    fprintf(stderr, "Invalid measurement: %s\n", str);
    exit(1);
  }
  return measure;
}

int main(int argc, char **argv) {

  int opt;
  double width, height;
  double shortWidth = SHORT_WIDTH;
  double railLength, railWidth = DEFAULT_WIDTH;
  double stileLength, stileWidth = DEFAULT_WIDTH;
  double panelHeight, panelWidth;
  double mortiseLength = DEFAULT_MORTISE_LENGTH;

  while ((opt = getopt(argc, argv, "r:s:w:m:")) != -1) {
    switch(opt) {
      case 'r':
        railWidth = getMeasurement(optarg);
        break;
      case 'w':
        shortWidth = getMeasurement(optarg);
        break;
      case 's':
        stileWidth = getMeasurement(optarg);
        break;
      case 'm':
        mortiseLength = getMeasurement(optarg);
        break;
      default:
        usage(argv[0]);
    }
  }
  if ((optind + 2) != argc) {
    usage(argv[0]);
  }
  width = getMeasurement(argv[optind++]);
  height = getMeasurement(argv[optind]);
  if (height < 5) {
    printf("\nSolid Panel\n");
    printf("Panel Width:     %s\n", formatMeasurement(width));
    printf("Panel Height:    %s\n\n", formatMeasurement(height));
  } else {
    if (height < 7) {
      railWidth = shortWidth;
    }
    stileLength = height;
    railLength = width - 2 * stileWidth + 2 * mortiseLength;
    panelWidth = railLength;
    panelHeight = height - 2 * railWidth + 2 * mortiseLength;
    printf("\nRail Width:      %s\n", formatMeasurement(railWidth));
    printf("Rail Length:     %s\n", formatMeasurement(railLength));
    printf("Stile Width:     %s\n", formatMeasurement(stileWidth));
    printf("Stile Length:    %s\n", formatMeasurement(stileLength));
    printf("Panel Width:     %s\n", formatMeasurement(panelWidth));
    printf("Panel Height:    %s\n", formatMeasurement(panelHeight));
    printf("Mortise Length:  %s\n\n", formatMeasurement(mortiseLength));
  }
  exit(0);
}
