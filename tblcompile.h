#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "xtrapbits.h"

/************************** Definitions  *************************/

#define TABLE_ERROR 0 /* Defined for invalid return value of getMinNumberOfTables */
#define SUCCESS 1;
#define FAILURE 0;

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
#define POLICY_INIT {.pl = 0, .n = 0, .N = 0, .b = 0, .B = 0, .q_masks = NULL, .b_masks = NULL}

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

/************************** Prototypes  *************************/

/* Determine the minimum number of tables that will fit in a
   prescribed amount of memory */
uint64_t min_tables(uint64_t, uint64_t, uint64_t);

/* Find the dimensions of a rule file, number of lines and max rule length */
policy read_policy(FILE *);

/* Parse the rule files into ? masks   */
void parse_q_masks(uint64_t, char **, uint8_t **);

/* Parse the rule files into bitmasks  */
void parse_b_masks(uint64_t, char **, uint8_t **);

/* Creates a binary representation of a binary string.  Adapted from:
   http://stackoverflow.com/questions/699968/display-the-binary-representation-
   of-a-number-in-c  */
void printbits(uint8_t);

/* Alocates a 2-d array of uint8_t in a contiguous block of memory
 * which is initialized to zeros */
uint8_t ** array2d_alloc(uint64_t, uint64_t);

/* Frees a 2d array that has been previously created by
 * array2d_alloc */
void array2d_free(uint8_t **);

/* Fills the filtering tables given a policy */
void fill_tables(policy, table_dims, uint8_t[*][*][*], uint8_t[*][*][*]);

/* Test whether a given byte array matches the b_array after being
 * masked by the q_array */
bool rule_matches(uint8_t[], uint8_t[], uint8_t[], uint64_t);

/* Copies a section of a bit array to the beginning of another bit array */
void copy_section(const uint8_t*, uint8_t*, uint64_t, uint64_t);

/* Creates a single table for rule matching */
uint8_t ** create_single_table(policy);

/* Prints a contiguous area of memory in binary starting with ptr and of the given size */
void print_mem(uint8_t *,  uint64_t , uint64_t);

/* Prints out lookup tables in a readable format  */
void print_tables(uint64_t, uint64_t, uint64_t, uint8_t[*][*][*]);

/* Filters incoming packets and classifies them to stdout */
void read_input_and_classify(policy, table_dims, uint8_t[*][*][*],uint8_t[*][*][*]);

/* AND two bit arrays together, the second argument is modified */
void and_bitarray(const uint8_t*, uint8_t*, uint64_t);
/************************** Inline functions  *************************/

/* easily find log2(r) */
#define lg(r) (log((double) (r)) / log(2.0))

/* Create a mask of 1's of length x */
#define ones_mask(x) ((1L << (x + 1)) - 1)
