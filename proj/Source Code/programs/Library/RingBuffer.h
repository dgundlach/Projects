struct RingBuffer {
    char *Buf;
    char *TmpBuf;
    unsigned int BufSize;
    unsigned int ReadPtr;
    unsigned int WritePtr;
};

struct RingBuffer *CreateRingBuffer(unsigned int);
unsigned int GetRingBufferReadSize(struct RingBuffer *);
unsigned int GetRingBufferWriteSize(struct RingBuffer *);
int RingBufferReadBinary(struct RingBuffer *, char *, int);
int RingBufferWriteBinary(struct RingBuffer *, char *, int);
int RingBufferPeekChar(struct RingBuffer *, unsigned int, char *);
int RingBufferFindChar(struct RingBuffer *, char, int *);
int RingBufferGetLine(struct RingBuffer *, char **);
int RingBufferReadLine(int, struct RingBuffer *, char **);
int RingBufferWriteLine(int, struct RingBuffer *);
int RingBufferRecvLine(int, struct RingBuffer *, char **, int);
int RingBufferSendLine(int, struct RingBuffer *, int);
