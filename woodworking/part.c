#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include "measurement.h"
#define PART_DOT_C 1
#include "part.h"

int init(char *options, char *usageOptions, char *usageAppend) {

  char *newopts;

  setting.options          = "f:F:m:M:p:P:i:I:s:S:l:o:O:x:";
  setting.usageOptions     = "";
  setting.usageAppend      = "";
  setting.faceRail         = 1.5;
  setting.faceStile        = 1.5;
  setting.faceMidRail      = 1.5;
  setting.faceMidStile     = 1.5;
  setting.panelRail        = 2.625;
  setting.panelRailMed     = 1.875;
  setting.panelStile       = 2.625;
  setting.panelStileMed    = 2.625;
  setting.panelMed         = 7;
  setting.panelShort       = 5;
  setting.mortiseLength    = 0.25;
  setting.overlay          = -0.0625;
  setting.proportionalFace = '\0';

  if (options) {
    if ((newopts = malloc(strlen(setting.options) + strlen(options) + 1)) == NULL) {
      return -1;
    }
    sprintf(newopts, "%s%s", setting.options, options);
    setting.options = newopts;
  }
  if (usageOptions) {
    setting.usageOptions = usageOptions;
  }
  if (usageAppend) {
    setting.usageAppend = usageAppend;
  }
  return 0;
}

void usage(char *program) {

  fprintf(stderr,
      "\nUsage: %s [-f <Face Rail Width>] [-F <Face Stile Width>]\n"
      "    [-m <Face Mid-Rail Width>] [-M <Face Mid-Stile Width>]\n"
      "    [-p <Panel Rail Width>] [-P <Panel Stile Width>]\n"
      "    [-i <Medium Panel Rail Width>] [-I <Medium Panel Stile Width>]\n"
      "    [-s <Medium Panel Limit>] [-S <Short Panel Limit>]\n"
      "    [-o <Overlay Offset>] [-O <Inset Offset>]\n"
      "    [-l <Mortise Length>] [-x <Proportional Face (0 or 1)>]\n"
      "%s\n%s\n", basename(program), setting.usageOptions, setting.usageAppend);
}

void displaySettings(void) {
  printf("Face Rail Width:            %s\n",
          formatMeasurement(setting.faceRail));
  printf("Face Stile Width:           %s\n",
          formatMeasurement(setting.faceStile));
  printf("Panel Rail Width:           %s\n",
          formatMeasurement(setting.panelRail));
  printf("Panel Rail Width (Medium):  %s\n",
          formatMeasurement(setting.panelRailMed));
  printf("Panel Stile Width:          %s\n",
          formatMeasurement(setting.panelStile));
  printf("Panel Stile Width (Medium): %s\n",
          formatMeasurement(setting.panelStileMed));
  printf("Panel Limit Medium:         %s\n",
          formatMeasurement(setting.panelMed));
  printf("Panel Limit Short:          %s\n",
          formatMeasurement(setting.panelShort));
  printf("Mortise Length:             %s\n",
          formatMeasurement(setting.mortiseLength));
  printf("Overlay (Inset):            ");
  if(setting.overlay < 0) {
    printf("(%s)\n", formatMeasurement(setting.overlay));
  } else {
    printf("%s\n", formatMeasurement(setting.overlay));
  }
  return;
}

#define checkProportional \
      if ((setting.overlay > 0) && setting.proportionalFace) { \
        setting.panelRailMed = setting.panelRail + setting.overlay; \
        setting.panelStileMed = setting.panelStile + setting.overlay; \
      }

int setSetting(int opt, char *arg) {

  double value = 0.0;

  if (strchr(setting.options, opt)) {
    if ((value = scanMeasurement(arg)) == -1) {
      return 1;
    }
  }
  switch (opt) {
    case 'f':
      setting.faceRail = value;
      checkProportional;
      break;
    case 'F':
      setting.faceStile = value;
      checkProportional;
      break;
    case 'm':
      setting.faceMidRail = value;
      break;
    case 'M':
      setting.faceMidStile = value;
      break;
    case 'p':
      setting.panelRail = value;
      break;
    case 'P':
      setting.panelStile = value;
      break;
    case 'i':
      setting.panelRailMed = value;
      break;
    case 'I':
      setting.panelStileMed = value;
      break;
    case 's':
      setting.panelMed = value;
      break;
    case 'S':
      setting.panelShort = value;
      break;
    case 'l':
      setting.mortiseLength = value;
      break;
    case 'O':
      value = 0.0 - value;  // Fall through to set the proper setting.
    case 'o':
      setting.overlay = value;
      checkProportional;
      break;
    case 'x':
      if (value == 0.0) {
        setting.proportionalFace = '\0';
      } else {
         setting.proportionalFace = '1';
         checkProportional;
      }
    default:
      return 1;
  }
  return 0;
}

faceFrame *newFaceFrame(double width, double height) {

  faceFrame *new;

  if ((new = malloc(sizeof(struct faceFrame))) == NULL) {
    return new;
  }
  new->rail.width = setting.faceRail;
  new->rail.length = height - (2 * setting.faceStile);
  new->stile.width = setting.faceStile;
  new->stile.length = height;
  return new;
}

facePanel *newFacePanel(double width, double height) {

  facePanel *new;

  if ((new = malloc(sizeof(struct facePanel))) == NULL) {
    return new;
  }
  if (height < setting.panelShort) {
    new->stile.width = 0;
    new->stile.length = 0;
    new->rail.width = 0;
    new->rail.length = 0;
    new->panel.width = width + (2 * setting.overlay);
    new->panel.length = height + (2 * setting.overlay);
  } else {
    if (height < setting.panelMed) {
      new->stile.width = setting.panelStileMed;
      new->rail.width = setting.panelRailMed;
    } else {
      new->stile.width = setting.panelStile;
      new->rail.width = setting.panelRail;
    }
    new->stile.length = height + (2 * setting.overlay);
    new->rail.length = width - (2 * new->stile.width)
        + (2 * setting.mortiseLength) + (2 * setting.overlay);
    new->panel.width = new->rail.length;
    new->panel.length = new->stile.length - (2 * new->rail.width)
        + (2 * setting.mortiseLength);
  }
  return new;
}
