#define SER_BUF_SIZE    0x0100 // This should be a power of 2
#define SER_BUF_MASK    0x00FF // Buffer size - 1
#define SER_BUF_EMPTY   0xFFFF

unsigned char   ser_buffer[SER_BUF_SIZE];
unsigned int    ser_wr_index,
                ser_rd_index;


void ser_put(unsigned char data)
{
   ser_buffer[ser_wr_index] = data;
   ser_wr_index++;
   ser_wr_index &= SER_BUF_MASK;

   if(ser_rd_index == ser_wr_index)
   {
      ser_rd_index++;
      ser_rd_index &= SER_BUF_MASK;
   }
}


int ser_get(void)
{
   unsigned int data;

   if(ser_rd_index == ser_wr_index)
      return SER_BUF_EMPTY;

   data = (int)ser_buffer[ser_rd_index];
   ser_rd_index++;
   ser_rd_index &= SER_BUF_MASK;

   return data;
}
