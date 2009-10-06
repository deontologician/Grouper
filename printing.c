#include "printing.h"

/* Creates a binary representation of a binary string.  Adapted from:
   http://stackoverflow.com/questions/699968/display-the-binary-representation-of
   -a-number-in-c  */
void printbits(uint8_t byte)
{
        for(int i = 8; i != 0; --i) putc('0' + ((byte >> (i-1)) & 1), stderr);
}

/* Print out a bitmask table */
void print_masks(uint8_t ** q_masks, uint64_t height, uint64_t width)
{
        for (uint64_t i = 0; i < height; ++i){
                for(uint64_t j = 0; j < width; j++){
                        printbits(q_masks[i][j]);
                        Trace(" ");
                }
                Trace( "\n");
        }
}

/* Prints a contiguous area of memory in binary starting with ptr to an array of
 * the given size */
void print_mem(uint8_t * start, uint64_t size, uint64_t cols)
{
        for(uint64_t i = size; i != 0; --i){
                printbits(start[i-1]);
                if ((i-1) % cols == 0 || i == 1){ Trace("\n"); }
                else {Trace(" ");}
        }
}

/* Prints out lookup tables in a readable format */
void print_tables(uint64_t h, uint64_t d, uint64_t w, uint8_t tables[h][d][w])
{
        for( uint64_t j = 0; j < d; ++j){
                Trace("Table #%"PRIu64":\n",j+1);
                for (uint64_t i = 0; i < h; ++i){
                        Trace("%"PRIu64" ",i);
                        for( uint64_t k = 0; k < w; ++k){
                                printbits(tables[i][j][k]);
                                Trace(" ");
                        }
                        Trace("\n");
                }
                Trace("-------------------------------------------\n");
        }
}

