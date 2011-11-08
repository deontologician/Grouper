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

#pragma once

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

/* Printing macros */
#ifdef DEBUG
 #define Trace(...) fprintf(stderr,__VA_ARGS__)
 #define printbits(...) __printbits(__VA_ARGS__)
 #define print_masks(...) __print_masks(__VA_ARGS__)
 #define print_mem(...) __print_mem(__VA_ARGS__)
#define print_tables(...) __print_tables(__VA_ARGS__)
#else
 #define Trace(...) 
 #define printbits(...) 
 #define print_masks(...) 
 #define print_mem(...) 
 #define print_tables(...) 
#endif

#define Error(...) fprintf(stderr,__VA_ARGS__)
#define Print(...) fprintf(stdout,__VA_ARGS__) 

/* Prototypes */
void __print_tables(uint64_t h, uint64_t d, uint64_t w, const uint8_t tables[h][d][w]);
void __printbits(uint8_t byte);
void __print_mem(const uint8_t * start, uint64_t size, uint64_t cols);
void __print_masks(const uint8_t ** q_masks, uint64_t height, uint64_t width);

