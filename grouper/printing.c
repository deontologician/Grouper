/*
Copyright (c) 2009-2010 University of South Florida
Created by Josh Kuhn, Jay Ligatti, Chris Gage
All rights reserved.

This file is part of Grouper.

    Grouper is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Grouper is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Grouper.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "printing.h"

/* Creates a binary representation of a binary string.  Adapted from:
   http://stackoverflow.com/questions/699968/display-the-binary-representation-of
   -a-number-in-c  */

void __printbits(uint8_t byte)
{

        for(int i = 8; i != 0; --i) putc('0' + ((byte >> (i-1)) & 1), stderr);
}

/* Print out a bitmask table */
void __print_masks(const uint8_t ** q_masks, uint64_t height, uint64_t width)
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
void __print_mem(const uint8_t * start, uint64_t size, uint64_t cols)
{
        for(uint64_t i = size; i != 0; --i){
                printbits(start[i-1]);
                if ((i-1) % cols == 0 || i == 1){ Trace("\n"); }
                else {Trace(" ");}
        }
}

/* Prints out lookup tables in a readable format */
void __print_tables(uint64_t h, uint64_t d, uint64_t w, const uint8_t tables[h][d][w])
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

