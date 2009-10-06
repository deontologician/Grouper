#pragma once

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

/* Printing macros */
#ifdef DEBUG
 #define Trace(...) fprintf(stderr,__VA_ARGS__)
#else
 #define Trace(...)
#endif

#define Error(...) fprintf(stderr,__VA_ARGS__)
#define Print(...) fprintf(stdout,__VA_ARGS__) 

/* Prototypes */
void print_tables(uint64_t h, uint64_t d, uint64_t w, uint8_t tables[h][d][w]);
void printbits(uint8_t byte);
void print_mem(uint8_t * start, uint64_t size, uint64_t cols);
void print_masks(uint8_t ** q_masks, uint64_t height, uint64_t width);

