#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "measurement.h"
#include "part.h"

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

  facePanel *panel;

  int opt;
  double width, height;

  init("d", "    [-d (Display Settings)]", "");

  while ((opt = getopt(argc, argv, setting.options)) != -1) {
    switch(opt) {
      case 'd':
        displaySettings();
        exit(1);
      default:
        if (setSetting(opt, optarg)) {
          usage(argv[0]);
          exit(1);
        }
    }
  }
  if ((optind + 2) != argc) {
    usage(argv[0]);
  }
  width = getMeasurement(argv[optind++]);
  height = getMeasurement(argv[optind]);
  panel = newFacePanel(width, height);
  printf("Rail Width:     %s\n",
          formatMeasurement(panel->rail.width));
  printf("Rail Length:    %s\n",
          formatMeasurement(panel->rail.length));
  printf("Stile Width:    %s\n",
          formatMeasurement(panel->stile.width));
  printf("Stile Length:   %s\n",
          formatMeasurement(panel->stile.length));
  printf("Panel Width:    %s\n",
          formatMeasurement(panel->panel.width));
  printf("Panel Height:   %s\n",
          formatMeasurement(panel->panel.length));
  printf("Mortise Length: %s\n",
          formatMeasurement(setting.mortiseLength));
  printf("Overlay:        %s\n",
          formatMeasurement(setting.overlay));
  exit(0);
}
