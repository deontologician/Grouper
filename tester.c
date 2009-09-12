#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include "xtrapbits.h"

typedef union UNION64 union64;

union UNION64 {
  uint64_t num;
  uint8_t arr[8];
};

/* Creates a binary representation of a binary string.  Adapted from:
   http://stackoverflow.com/questions/699968/display-the-binary-representation-of
   -a-number-in-c  */
void printbits(uint8_t v)
{
        for(int i = 0; i < 8; ++i) putc('0' + ((v >> i) & 1), stderr);
}

/* Prints a contiguous area of memory in binary starting with ptr to an array of
 * the given size */
void print_mem(uint8_t * ptr, uint64_t size, uint64_t cols)
{
        for(uint64_t i = 0; i < size; ++i){
                printbits(ptr[i]);
                if ((i+1) % cols == 0 || i == size - 1) fprintf(stderr,"\n");
                else fprintf(stderr," ");
        }
}

/* Prints out lookup tables in a readable format */
void print_tables(uint64_t h, uint64_t d, uint64_t w, uint8_t tables[h][d][w])
{
        for( uint64_t j = 0; j < d; ++j){
                fprintf(stderr,"Table #%"PRIu64":\n",j+1);
                for (uint64_t i = 0; i < h; ++i){
                        fprintf(stderr,"%"PRIu64" ",i);
                        for( uint64_t k = 0; k < w; ++k){
                                printbits(tables[j][i][k]);
                                fprintf(stderr," ");
                        }
                        fprintf(stderr,"\n");
                }
                fprintf(stderr,"-------------------------------------------\n");
        }
}

#define print(b) fprintf(stderr,b)
#define LINE fprintf(stderr,"-------------------------------------------------\
-------------------------------\n");

int main(int argc, char* argv[])
{
        union64 h = {.num = 0};
        union64 r = {.arr = {0}};

        for(uint64_t i=0;i < 64;i++){
                h.num = 1L << i;
                BitTrue(r.arr, PackingIndex(i));
                print("h:");print_mem(h.arr,8,8);
                print("r:");print_mem(r.arr,8,8);
                LINE
        }
}
/*
Strategies that might work
Copy in bits backwards
reverse mask bits on creation... (seems redundant)
for each line:
   for each byte:
       [0000 0000,0000 0000,0000 0000,0000 0000]
        0         8         16        24      31
        ByteInArray*8 + (7 - i%BitsInByte)
*/
