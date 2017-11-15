int BitCount(unsigned int n) {
    
    register unsigned int tmp;
    
    tmp = n - ((n >> 1) & 033333333333)
            - ((n >> 2) & 011111111111);
    return ((tmp + (tmp >> 3)) & 030707070707) % 63;
}
