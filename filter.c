/* 

Filter for std input thauses std input  Lookup tables for Expressive Packet
Classification
   
Data structures algorithm design by Jay Ligatti
Implementation written by Joshua Kuhn

The data structures in this file are used by a dimension independent
table-based algorithm for classifying packets. We use the following
variables to describe different aspects of rules and packet data.

 - n is the number of rules in a classification policy
 - b is the number of packet bits relevant to classification policy
 - d is the number of dimensions relevant to classification policy
 - t is the number of lookup tables

The dimension-independent algorithm uses t lookup tables, each of
which maps approximately b/t packet bits to n-sized bit arrays.


The ideal use of filter will be piping a datasource into filter as std
input. Filter's standard output will produce a mapping from packets to
rules.

i.e.
datasource | filter tables.tbl > log.txt

 */
#include "filter.h"

int main (int argc, char *argv[])
{
  FILE *tbl_file = NULL;
  unsigned char inbyte = 0;       /* storage for incoming byte */
  uint64_t packet_count = 0;      /* total number of pack*/

  /* If too few arguments are passed, print usage to stderr*/
  if ( argc < 2 ){
    fprintf(stderr, "Usage: %s <filename.tbl>\n", argv[0]);
  }

  /* Attempt to open the table file */
  tbl_file = fopen(argv[1],"r");

  /* If file cannot be opened for whatever reason, print failure and
     exit. */
  if (tbl_file == NULL){
    fprintf(stderr, "Can't open table file %s.\n", argv[1]);
    exit(EXIT_FAILURE);
  }

  /* Store tables in memory for quick access */

  /* Close up the file */
  fclose(tbl_file);

  /* Take data from input one byte at a time until EOF is received (Ctrl+D) */
  while(scanf("%c", &inbyte) != EOF){
    printf("%c", inbyte);
    packet_count++;
  }
  /* Print a summary of the results */
  printf("\nPackets processed: %llu \n", (long long int) packet_count);

  return EXIT_SUCCESS;
}
