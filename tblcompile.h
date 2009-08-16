#ifndef __TBLCOMPILE_H_
#define __TBLCOMPILE_H_ "tblcompile.h"

#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "xtrapbits.h"

/************************** Definitions  *************************/

#define TABLE_ERROR 0 /* Defined for invalid return value of getMinNumberOfTables */
#define SUCCESS 1;
#define FAILURE 0;

/************************** Prototypes  *************************/

/* Determine the minimum number of tables that will fit in a
   prescribed amount of memory */
uint64_t min_tables(uint64_t, uint64_t, uint64_t);

/* Find the dimensions of a rule file, number of lines and max rule length */
void file_dimensions(FILE *);

/* Parse the rule files into ? masks   */
void parse_q_masks(  uint64_t rule_count
                   , uint64_t max_bytes
                   , char ** rule_array
                   , uint8_t q_masks[rule_count][max_bytes]);

/* Parse the rule files into bitmasks  */
void parse_b_masks(  uint64_t rule_count
                   , uint64_t max_bytes
                   , char ** rule_array
                   , uint8_t b_masks[rule_count][max_bytes]);

/* Creates a binary representation of a binary string.  Adapted from:
http://stackoverflow.com/questions/699968/display-the-binary-representation-of-a-number-in-c  */
void printbits(uint8_t);

/************************** Inline functions  *************************/

/* easily find log2(r) */
#define lg(r) (log((double) (r)) / log(2.0))


#endif
