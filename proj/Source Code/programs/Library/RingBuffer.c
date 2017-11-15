#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
//#include "Round2.h"
#include "SendAll.h"
#include "RingBuffer.h"

struct RingBuffer *CreateRingBuffer(unsigned int bufsize) {

    int upsize;
    struct RingBuffer *rb;

//  upsize = (Round2(bufsize) << 1) + sizeof(struct RingBuffer);
    upsize = (bufsize << 1) + sizeof(struct RingBuffer);
    if (!(rb = malloc(upsize))) {
        return NULL;
    }
    bzero(rb, upsize);
    rb->BufSize = (upsize - sizeof(struct RingBuffer)) >> 1;
    rb->Buf = (char *)rb + sizeof(struct RingBuffer);
    rb->TmpBuf = rb->Buf + rb->BufSize;
    return rb;
}

unsigned int GetRingBufferReadSize(struct RingBuffer *rb) {

    if (rb) {
        if (rb->ReadPtr == rb->WritePtr) {
            return 0;
        }
        if (rb->ReadPtr < rb->WritePtr) {
            return rb->WritePtr - rb->ReadPtr;
        }
        return rb->BufSize - rb->ReadPtr + rb->WritePtr;
    }
    return 0;
}

unsigned int GetRingBufferWriteSize(struct RingBuffer *rb) {

    if (rb) {
        if (rb->ReadPtr == rb->WritePtr) {
            return rb->BufSize;
        }
        if (rb->WritePtr < rb->ReadPtr) {
            return rb->ReadPtr - rb->WritePtr;
        }
        return rb->BufSize - rb->WritePtr + rb->ReadPtr;
    }
    return 0;
}

int RingBufferReadBinary(struct RingBuffer *rb, char *buffer, int bufsize) {

    unsigned int size1, size2;
    
    if (bufsize <= GetRingBufferReadSize(rb)) {
        if (rb->ReadPtr + bufsize <= rb->BufSize) {
            memcpy(buffer, rb->Buf + rb->ReadPtr, bufsize);
            rb->ReadPtr += bufsize;
        } else {
            size1 = rb->BufSize - rb->ReadPtr;
            size2 = bufsize - size1;
            memcpy(buffer, rb->Buf + rb->ReadPtr, size1);
            memcpy(buffer + size1, rb->Buf, size2);
            rb->ReadPtr = size2;
        }
        return 1;
    }
    return 0;
}

int RingBufferWriteBinary(struct RingBuffer *rb, char *buffer, int bufsize) {

    unsigned int size1, size2;
    
    if (bufsize < GetRingBufferWriteSize(rb)) {
        if (rb->WritePtr + bufsize < rb->BufSize) {
            memcpy(rb->Buf + rb->WritePtr, buffer, bufsize);
            rb->WritePtr += bufsize;
        } else {
            size1 = rb->BufSize - rb->WritePtr;
            size2 = bufsize - size1;
            memcpy(rb->Buf + rb->WritePtr, buffer, size1);
            memcpy(rb->Buf, buffer + size1, size2);
            rb->WritePtr = size2;
        }
        return 1;
    }
    return 0;
}

int RingBufferPeekChar(struct RingBuffer *rb, unsigned int pos, char *ch) { 
    
    if (pos < GetRingBufferReadSize(rb)) {
        if (rb->WritePtr > rb->ReadPtr) {
            *ch = *(rb->Buf + rb->ReadPtr + pos);
            return 1;
        }
        if (rb->WritePtr < rb->ReadPtr) {
            if (rb->ReadPtr + pos < rb->BufSize) {
               *ch = *(rb->Buf + rb->ReadPtr + pos);
            } else {
               *ch = *(rb->Buf + pos - (rb->BufSize - rb->ReadPtr));
            }
            return 1;
        }
    }
    return 0;
}

int RingBufferFindChar(struct RingBuffer *rb, char ch, int *pos) {

    int i, size;
    char c;
    
    size = GetRingBufferReadSize(rb);
    for (i=0; i<size; i++) {
        if (RingBufferPeekChar(rb, i, &c)) {
            if (c == ch) {
                *pos = i;
                return 1;
            }
        }
    }
    return 0;
}

int RingBufferGetLine(struct RingBuffer *rb, char **string) {

    int pos;
    int size;
    
    if (RingBufferFindChar(rb, '\n', &pos)) {
        size = pos + 1;
        if (RingBufferReadBinary(rb, rb->TmpBuf, size)) {
            *(rb->TmpBuf + size) = '\0';
            *string = rb->TmpBuf;
            return 1;
        }
    }
    return 0;
}

int RingBufferReadLine(int fd, struct RingBuffer *rb, char **string) {

    int pos;
    int len;
    int size1, size2;
    int rc = -1;
    
    if (!RingBufferFindChar(rb, '\n', &pos)) {
        len = GetRingBufferWriteSize(rb);
        if (!rb->ReadPtr) {
            rc = read(fd, rb->Buf + rb->WritePtr, len);
            if (rc != -1) {
                rb->WritePtr += rc;
            }
        } else {
            size1 = rb->BufSize - rb->WritePtr;
            size2 = len - size1;
            rc = read(fd, rb->Buf + rb->WritePtr, size1);
            if (rc != -1) {
                if (rc == size1) {
                    rb->WritePtr = 0;
                    rc = read(fd, rb->Buf, size2);
                    if (rc != -1) {
                        rb->WritePtr = rc;
                    }
                } else {
                    rb->WritePtr += rc;
                }
            }
        }
        if (!RingBufferFindChar(rb, '\n', &pos)) {
            rc = -1;
        }
    } else {
        rc = 0;
    }
    if (rc != -1) {
        len = pos + 1;
        if (RingBufferReadBinary(rb, rb->TmpBuf, len)) {
            *(rb->TmpBuf + len) = '\0';
            *string = rb->TmpBuf;
        } else {
            *string = NULL;
            rc = -1;
        }
    }
    return (rc == -1) ? 0 : 1;
}

int RingBufferWriteLine(int fd, struct RingBuffer *rb) {

    int pos;
    int len;
    int size1, size2;
    int rc = -1;
    
    if (RingBufferFindChar(rb, '\n', &pos)) {
        len = pos + 1;
        if (rb->WritePtr + pos < rb->BufSize) {
            rc = write(fd, rb->Buf + rb->WritePtr, len);
            if (rc != -1) {
                rb->WritePtr += len;
            }
        } else {
            size1 = rb->BufSize - rb->WritePtr;
            size2 = len - size1;
            rc = write(fd, rb->Buf + rb->WritePtr, size1);
            if (rc != -1) {
                rb->WritePtr = 0;
                rc = write(fd, rb->Buf, size2);
                if (rc != -1) {
                    rb->WritePtr = size2;
                }
            }
        }
    }
    return (rc == -1) ? 0 : 1;
}

int RingBufferRecvLine(int s, struct RingBuffer *rb, char **string, int flags) {

    int pos;
    int len;
    int size1, size2;
    int rc = -1;
    fd_set rfds;
    
    if (!RingBufferFindChar(rb, '\n', &pos)) {
        len = GetRingBufferWriteSize(rb);
        FD_ZERO(&rfds);
        FD_SET(s, &rfds);
        if ((select(s + 1, &rfds, NULL, NULL, NULL) != -1) 
                                       && FD_ISSET(s, &rfds)) {
            if (!rb->ReadPtr) {
                rc = recv(s, rb->Buf + rb->WritePtr, len, flags);
                if (rc != -1) {
                    rb->WritePtr += rc;
                }
            } else {
                size1 = rb->BufSize - rb->WritePtr;
                size2 = len - size1;
                rc = recv(s, rb->Buf + rb->WritePtr, size1, flags);
                if (rc != -1) {
                    if (rc == size1) {
                        rb->WritePtr = 0;
                        if ((select(s + 1, &rfds, NULL, NULL, NULL) != -1) 
                                                    && FD_ISSET(s, &rfds)) {
                            rc = recv(s, rb->Buf, size2, flags);
                            if (rc != -1) {
                                rb->WritePtr = rc;
                            }
                        }
                    } else {
                        rb->WritePtr += rc;
                    }
                }
            }
        }
        if (!RingBufferFindChar(rb, '\n', &pos)) {
            rc = -1;
        }
    } else {
        rc = 0;
    }
    if (rc != -1) {
        len = pos + 1;
        if (RingBufferReadBinary(rb, rb->TmpBuf, len)) {
            *(rb->TmpBuf + len) = '\0';
            *string = rb->TmpBuf;
        } else {
            *string = NULL;
            rc = -1;
        }
    }
    return (rc == -1) ? 0 : 1;
}

int RingBufferSendLine(int s, struct RingBuffer *rb, int flags) {

    int pos;
    int len;
    int size1, size2;
    int rc = -1;
    
    if (RingBufferFindChar(rb, '\n', &pos)) {
        len = pos + 1;
        if (rb->WritePtr + pos < rb->BufSize) {
            rc = SendAll(s, rb->Buf + rb->WritePtr, &len, flags);
            rb->WritePtr += len;
        } else {
            size1 = rb->BufSize - rb->WritePtr;
            size2 = len - size1;
            rc = SendAll(s, rb->Buf + rb->WritePtr, &size1, flags);
            rb->WritePtr += size1;
            if (!rc) {
                rb->WritePtr = 0;
                rc = SendAll(s, rb->Buf, &size2, flags);
                rb->WritePtr = size2;
            }
        }
    }
    return (rc == -1) ? 0 : 1;
}
