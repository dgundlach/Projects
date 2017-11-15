int bitcount8(uint8_t);
int bitcount16(uint16_t);
int bitcount32(uint32_t);
int bitcount64(uint64_t);
int (*bitcount)(unsigned int) = &bitcount32;
