#define PATHENT(e) ((*(e) == '.') && ((*((e) + 1) == '\0') || ((*((e) + 1) == '.') && (*((e) + 2) == '\0'))))
