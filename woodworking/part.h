#ifndef PART_DOT_C
extern
#endif
struct {
  char *options;           // getopt string: "f:F:m:M:p:P:i:I:s:S:l:o:O:x:"
  char *usageOptions;
  char *usageAppend;
  double faceRail;         // f
  double faceStile;        // F
  double faceMidRail;      // m
  double faceMidStile;     // M
  double panelRail;        // p
  double panelStile;       // P
  double panelRailMed;     // i
  double panelStileMed;    // I
  double panelMed;         // s
  double panelShort;       // S
  double mortiseLength;    // l
  double overlay;          // o,O
  char proportionalFace;   // x
} setting;

typedef struct part {
  double width;
  double length;
} part;

typedef struct faceFrame {
  part rail;
  part stile;
} faceFrame;

typedef struct facePanel {
  part rail;
  part stile;
  part panel;
} facePanel;

int init(char *, char *, char *);
void usage(char *);
int addOptions(char *);
void displaySettings(void);
int setSetting(int, char *);
faceFrame *newFaceFrame(double, double);
facePanel *newFacePanel(double, double);

#define opening(o,f) o.width = f->rail.length - 2 * f->stile.width; \
                    o.height = f->stile.length - 2 * f->rail.width;
