
#include "tblcompile.h"

int main(int argc, char* argv[])
{

        /* Check for the proper number of arguments. Print usage if wrong number */
        if (argc != 3){
                fprintf(stderr, "Usage: %s <max memory> <policy file.pol>\n", argv[0] );
                exit(EXIT_FAILURE);
        }
        /* Parse the number of memory bits available */
        uint64_t memsize_bits = atol (argv[1]) * 8; /* Convert argument in bytes to bits */
  
        /* open the policy file*/
        FILE *pol_file = fopen(argv[2], "r");
        
        file_dimensions(pol_file);

        //uint64_t table_count = min_tables(memsize_bits, rule_count , rel_bits); 
        //printf("%"PRIu64" tables needed for memory size of %"PRIu64" bits.\n",
        //       table_count, memsize_bits);

        /* Close the file */
        fclose(pol_file);

        return EXIT_SUCCESS;
}

/* Determine the minimum number of tables that will fit in a
   prescribed amount of memory 
   m is max memory used in bits
   n is the number of rules
   b is the number of relevant pattern bits*/
uint64_t min_tables(uint64_t m , uint64_t n , uint64_t b)
{
        /* Check to ensure n and b are positive, and that the max memory is
           large enough to hold the smallest tables for the number of rules
           specified. Return error if not. */
        if(m < 2*n*b || n < 1 || b < 1) return TABLE_ERROR;
        /* If the amount of memory available is larger than the amount
           needed for a 1 table solution, return 1. 1 table is a special
           case because it allows only using lg(n) bits to store the rules */
        if(m >= lg(n)*exp2(b)) return 1;

        /* Initial highest number of tables that might be needed */
        uint64_t high = ceil(b/2.0);
        /* Initial lowest number of tables that might be needed  */
        uint64_t low = 1;
        /* Binary search through possible table numbers. */
        while((high - low) > 1) {
                uint64_t mid = (high + low)/2;
                uint64_t memNeededForMidTables = 
                        (mid - (b % mid))*exp2(b/mid)*n + (b%mid)*exp2((b/mid) + 1)*n;
                if(m < memNeededForMidTables)
                        low = mid;
                else 
                        high  = mid;
        }
        return low;
}

/* Finds dimensions for the file */ 
void file_dimensions(FILE * file)
{
        uint64_t rule_count = 0;
        uint64_t max_bits = 0;
        uint64_t curr_bits = 0;
        char tmp;
        
        /* Scan the file, finding the number of lines and the max line
         * length (which will be the max relevant bits */
        while(fscanf(file, "%c", &tmp) != EOF){
                /* Ensure 1,0,?,\n are the only characters in the file  */
                if(tmp != '0' && tmp != '1' && tmp != '?' && tmp != '\n'){
                        printf("Invalid character \'%c\' in policy file. Aborting... \n", tmp);
                        exit(EXIT_FAILURE);
                }
                if(tmp == '\n'){
                        rule_count++;
                        if(curr_bits > max_bits) 
                                max_bits = curr_bits;
                        curr_bits = 0;
                }else {
                        curr_bits ++ ;
                }
        }

        printf("%"PRIu64" rules parsed, with a max rule size of %"PRIu64" bits.\n", rule_count, max_bits);
        
        /* Allocate the array since we know the max size. The additional 2 is for the \n\0 */    
        //char **rule_array = malloc(rule_count * (max_bits + 2) * sizeof(char));
        char ** rule_array = malloc(sizeof(char*));
        for (uint64_t i = 0; i < rule_count; i++){
                rule_array[i] = calloc(max_bits + 2, sizeof(char));
        }
           
        rewind(file);
        /* Read in the file */
        for (uint64_t i = 0; i < rule_count; i++){
                fgets(rule_array[i], max_bits + 2, file);
        }
        
        /* We convert bits to bytes rounding up */
        uint64_t max_bytes = 0;
        if (max_bits % 8)
                max_bytes = ceil(max_bits/8.0);
        else
                max_bytes = max_bits/8;

        /* Allocate space for ? masks corresponding to rules */
        uint8_t (*q_masks)[max_bytes]  =  calloc(rule_count , max_bytes);
        /* Allocate space for bit masks corresponding to rules */
        uint8_t (*b_masks)[max_bytes]  =  calloc(rule_count , max_bytes);

        parse_q_masks(rule_count,  max_bits, rule_array, q_masks);

        parse_b_masks(rule_count, max_bits, rule_array, b_masks);
        
        printf("Q Masks: \n");
        for (uint64_t i = 0; i < rule_count; i++){
                printf("Rule %"PRIu64": ", i);
                for (uint64_t j = 0; j < max_bytes; j++){
                        printbits(q_masks[i][j]);
                        printf(" ");
                }
                printf("\n");
        }
        printf("\nB Masks: \n");
        for (uint64_t i = 0; i < rule_count; i++){
                printf("Rule %"PRIu64": ", i);
                for (uint64_t j = 0; j < max_bytes; j++){
                        printbits(b_masks[i][j]);
                        printf(" ");
                }
                printf("\n");
        }
}

/* Parse the rule files into ? masks   */
void parse_q_masks(  uint64_t rule_count
                   , uint64_t max_bytes
                   , char **  rule_array
                   , uint8_t q_masks[rule_count][max_bytes]
                  )
{
        /* Go through each character in the rules and set the
         * corresponding bit in the q_mask array to 0 or 1  */
        for(uint64_t i = 0; i < rule_count; i++){
                for(uint64_t j = 0; rule_array[i][j] != '\n'; j++){
                        if (rule_array[i][j] == '?') BitFalse(q_masks[i],j); 
                        else BitTrue(q_masks[i], j);
                }
        }
}

/* Parse the rule files into bitmasks  */
void parse_b_masks(   uint64_t rule_count
                    , uint64_t max_bytes
                    , char ** rule_array
                    , uint8_t b_masks[rule_count][max_bytes]
                  )
{
        /* Go through each character in the rules and set the
         * corresponding bit in the b_mask array to 0 or 1  */
        for(uint64_t i = 0; i < rule_count; i++){
                for(uint64_t j = 0; rule_array[i][j] != '\n'; j++){
                        if (rule_array[i][j] == '0') BitFalse(b_masks[i],j);
                        if (rule_array[i][j] == '1') BitTrue(b_masks[i], j);
                }
        }
}

/* Creates a binary representation of a binary string.  Adapted from:
http://stackoverflow.com/questions/699968/display-the-binary-representation-of-a-number-in-c  */
void printbits(uint8_t v)
{
        for(int i = 0; i < 8; i++) putchar('0' + ((v >> i) & 1));
}
