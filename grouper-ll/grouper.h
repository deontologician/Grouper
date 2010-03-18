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

#define _GNU_SOURCE /* This is to import getline (getline is linux specific) */

#include <stdlib.h>             
#include <stdint.h>             /* adds uint64_t and uint8_t */
#include <stdbool.h>            /* Adds true/false */
#include <inttypes.h>           /* For format specifiers for uint* types */
#include <math.h>               /* For log, ceil etc. */
#include <stdio.h>              /* For standard functions */
#include <string.h>             /* For string functions  */
#include <errno.h>              /* For global variable errno */
#include <pthread.h>            /* For threads */
#include <sys/time.h>           /* For clock() and gettimeofday()*/
#include <unistd.h>             /* For sysconf() to find # of cores */
#include "xtrapbits.h"          /* Bitshifting macros */
#include "printing.h"           /* Bit printing */

/************************** Definitions  *************************/
#define TABLE_ERROR 0 /* Defined for invalid return value of getMinNumberOfTables */
#define SUCCESS 1
#define FAILURE 0
#define MIN_THREADS_PER_CORE 100 /* The minimum number of threads that should be
                                  * spawned per core  */
#define min(A,B) (((A) < (B)) ? (A) : (B))

typedef struct {
        uint64_t pl;        /* Packet length */
        uint64_t n;         /* Number of rules */
        uint64_t N;         /* Number of rules rounded up to a multiple of 8*/
        uint64_t b;         /* Number of relevant bits in pattern */
        uint64_t B;         /* Number of relevant bits rounded up
                             * to a multiple of 8 */             
        uint8_t ** q_masks; /* Masks representing ? in policy pattern */
        uint8_t ** b_masks; /* Masks representing 0,1 in policy pattern */
} policy;
#define POLICY_INIT {.pl = 0, .n = 0, .N = 0, .b = 0, .B = 0, \
                        .q_masks = NULL, .b_masks = NULL}

/* Union to convert between uint64_t and uint8_t[8] */
typedef union UNION64 union64;
union UNION64 {
  uint64_t num;
  uint8_t arr[8];
};

/* Holds the dimensions of the filtering tables */
typedef struct {
        /* All of these dimensions are in bits */
        uint64_t even_h;        /* Height of even tables */
        uint64_t odd_h;         /* Height of odd tables */
        uint64_t even_d;        /* Depth of even tables */
        uint64_t odd_d;         /* Depth of odd tables */
        uint64_t bitwidth;      /* Width of all tables  */
        uint64_t bytewidth;     /* Width of tables in bytes */
        uint64_t even_s;        /* Width of even section */
        uint64_t odd_s;         /* Width of odd section */
} table_dims;

/* Structure to hold args for passing to a fill_table thread */
typedef struct {
        policy * pol;           /* pointer to policy */
        table_dims * dims;      /* pointer to table dimensions */
        uint64_t table_num;     /* which table this thread will be handling */
        uint8_t * tables;      /* pointer to tables (even or odd) */
} thread_args;

typedef struct timeval profile_t; /* redefine to indicate purpose */

/************************** Prototypes  *************************/

/* Determine the minimum number of tables that will fit in a
   prescribed amount of memory */
uint64_t min_tables(uint64_t m, uint64_t n, uint64_t b);

/* Find the dimensions of a rule file, number of lines and max rule length */
policy read_policy(FILE * file);

/* Parse the rule files into ? masks   */
void parse_q_masks(uint64_t rule_count, char **rule_array, uint8_t **q_masks);

/* Parse the rule files into bitmasks  */
void parse_b_masks(uint64_t rule_count, char **rule_array, uint8_t **b_masks);

/* Alocates a 2-d array of uint8_t in a contiguous block of memory
 * which is initialized to zeros */
uint8_t ** array2d_alloc(uint64_t height, uint64_t width);

/* Frees a 2d array that has been previously created by
 * array2d_alloc */
void array2d_free(uint8_t ** arr);

/* Fills the filtering tables given a policy */
void fill_tables(policy pol, table_dims dims, 
                 uint8_t even_tables[dims.even_h][dims.even_d][dims.bytewidth],
                 uint8_t odd_tables[dims.odd_h][dims.odd_d][dims.bytewidth]);

/* Function to fill a single even table (called by fill_tables threads) */
void * fill_even_table(void * args);

/* Function to fill a single odd table (called by fill_tables threads)*/
void * fill_odd_table(void * args);

/* Test whether a given byte array matches the b_array after being
 * masked by the q_array */
bool rule_matches( uint64_t size, uint8_t input[size],  
                   const uint8_t q_mask[size], const uint8_t b_mask[size]);

/* Copies a section of a bit array to the beginning of another bit array */
void copy_section(const uint8_t *src_array, uint8_t *dst_array, uint64_t startbit,
                  uint64_t size);

/* Creates a single table for rule matching */
uint8_t * create_single_table(policy pol,uint64_t width);

/* Classify packets with a single table */
void read_input_and_classify_single(policy pol, uint64_t width,
                                    uint8_t (*table)[width]);
/* Filters incoming packets and classifies them to stdout */
void read_input_and_classify(policy pol, table_dims dim, 
                             uint8_t even_tables[dim.even_h][dim.even_d][dim.bytewidth], 
                             uint8_t odd_tables[dim.odd_h][dim.odd_d][dim.bytewidth]);

/* AND two bit arrays together, the second argument is modified */
static inline void and_bitarray(const uint8_t *new, uint8_t *total, uint64_t size);

/* Rounds up the result of integer division */
static inline uint64_t ceil_div(uint64_t num, uint64_t denom);

/* Starts timing a section of code */
static inline void start_timing(profile_t * time);

/* Finishes timing a section of code and returns microseconds elapsed */
static inline long end_timing(profile_t * time);

/************************** Inline functions  *************************/

/* Return index in packing order (msb in byte first) */
#define PackingIndex(bit) ((((bit)/BitsInByte)*BitsInByte)  \
                          + (BitsInByte - 1 - (bit)%BitsInByte))

