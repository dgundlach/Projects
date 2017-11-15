#define BYTE_COUNT		32

typedef struct CSet {
    unsigned char bitfields[BYTE_COUNT];
} CSet;

void ClearCSet(CSet *);
int CharInCSet(CSet *, unsigned char);
void AddToCSet(CSet *, unsigned char);
void AddRangeToCSet(CSet *, unsigned char , unsigned char );
void RemoveFromCSet(CSet *, unsigned char);
void RemoveRangeFromCSet(CSet *, unsigned char, unsigned char);
