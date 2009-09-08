#include "tblcompile.h"
#define DEBUG true

int main(int argc, char* argv[])
{
        /* Check for the proper number of arguments. Print usage if wrong
         * number */
        if (argc < 3){
                fprintf(stderr, 
                        "Usage: %s <max memory> <policy file.pol> [<input file>]"
                        " [<output file]\n", argv[0] );
                exit(EXIT_FAILURE);
        }
        /* Parse the number of memory bits available */
        /* The input is in bytes, so convert it to bits for the algorithm */
        uint64_t memsize_bits = atoll(argv[1]) * 8; 
  
        /* open the policy file*/
        FILE *pol_file = fopen(argv[2], "r");
        /* Exit if file fails to open */
        if (pol_file == NULL){
                fprintf(stderr, "Invalid policy file: \'%s\' \n", argv[2]);
                exit(EXIT_FAILURE);
        }

        /* Check for input file & ensure it can be opened. */
        if(argc >= 4){
                FILE * in_temp = stdin;
                stdin = fopen(argv[3], "r");
                if(stdin == NULL){
                        fprintf(stderr,"Input file '%s' is invalid or "
                                "non-existent. Falling back to stdin.\n", 
                                argv[3]);
                        stdin = in_temp;
                }
        }

        /* Check for output file & ensure it can be opened. */
        if(argc >= 5){
                FILE * out_temp = stdout;               
                stdout = fopen(argv[4], "w");
                if(stdout == NULL){
                        fprintf(stderr,"Output file '%s' cannot be opened for "
                                "writing. Falling back to stdout.\n", argv[4]);
                        stdout = out_temp;
                }
        }

        policy pol = read_policy(pol_file);
        fclose(pol_file);

        /* Calculate number of tables required.  */
        uint64_t t = min_tables(memsize_bits, pol.N , pol.b);

        if (t == TABLE_ERROR){
                fprintf(stderr,"Error: not enough memory to build tables. "
                        "Needs at least %"PRIu64" bytes.\n",(2*pol.N*pol.b)/8);
                exit(EXIT_FAILURE);
        }

#if DEBUG
        fprintf(stderr, "%"PRIu64" tables needed for memory size of %"PRIu64
                " bits.\n",t, memsize_bits);
#endif
        /* Handle the special single table case */
        if (t == 1){    
                uint8_t ** single_table = create_single_table(pol);
        }else{
                /* Calculate heights and depths */
                table_dims d = {
                        .even_s  = pol.b/t,
                        .odd_s   = pol.b/t + 1,
                        .even_h  = (uint64_t) exp2(pol.b/t),
                        .odd_h   = (uint64_t) exp2(pol.b/t + 1),
                        .even_d  = t - pol.b % t,
                        .odd_d   = pol.b % t,
                        .bitwidth   = pol.N,
                        .bytewidth  = pol.N / 8
                };

#if DEBUG
                fprintf (stderr,"\n\nCreating %lu tables %lu of which will be "
                         "%lux%lu,\nand %lu of which will be %lux%lu.\n\n", 
                         t, d.even_d, d.bitwidth,d.even_h, d.odd_d, d.bitwidth, 
                         d.odd_h);
#endif

                /* Create two large table arrays */
                uint8_t (*even_tables)[d.even_d][d.bytewidth] =
                        calloc(d.even_h * d.even_d, 
                               sizeof(uint8_t[d.bytewidth]));
                uint8_t (*odd_tables) [d.odd_d] [d.bytewidth] = 
                        calloc(d.odd_h  * d.odd_d,  
                               sizeof(uint8_t[d.bytewidth]));
                
                fill_tables(pol, d, even_tables, odd_tables);

#if DEBUG
                fprintf(stderr,"Even tables: \n");
                print_tables(d.even_h, d.even_d, d.bytewidth, even_tables);
                fprintf(stderr,"Odd tables: \n");
                print_tables(d.odd_h, d.odd_d, d.bytewidth, odd_tables);
#endif
                /* Free intermittant resources */
                array2d_free(pol.q_masks);
                array2d_free(pol.b_masks);
                pol.q_masks = NULL;
                pol.b_masks = NULL;

                /* Read input and classify it until EOF */
                read_input_and_classify(pol,d,even_tables,odd_tables);
                
                /* Release resources */
                free(even_tables);
                free(odd_tables);
        }

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
        if(m >= 8*ceil(ceil(lg(n))/8.0)*exp2(b)) return 1;

        /* Initial highest number of tables that might be needed */
        uint64_t high = (uint64_t) ceil(b/2.0);
        /* Initial lowest number of tables that might be needed  */
        uint64_t low = 1;
        /* Binary search through possible table numbers. */
        while((high - low) > 1) {
                uint64_t mid = (high + low)/2;
                uint64_t memNeededForMidTables = 
                        (mid - (b % mid))*exp2(b/mid)*n + 
                        (b%mid)*exp2((b/mid) + 1)*n;                                  
                if(m < memNeededForMidTables)
                        low = mid;
                else 
                        high = mid;
        }
        return low;
}

/* reads in a policy from a file and creates the relevant patterns in memory */
policy read_policy(FILE * file)
{
        policy pol = POLICY_INIT;
        uint64_t curr_bits = 0;
        char tmp;
        size_t n = 80;
        char* tmpstring = malloc(sizeof(char[n]));

        /* Consume the first line (this is a gcc extension function)*/
        getline(&tmpstring, &n, file); 
        
        pol.pl = atoll(tmpstring);

#if DEBUG
        fprintf(stderr,"Packet length: %"PRIu64"\n", pol.pl);
#endif

        /* Scan the rest of the file, finding the number of lines and the max
         * line length (which will be the max relevant bits */
        while(fscanf(file, "%c", &tmp) != EOF){
                /* Ensure 1,0,?,\n are the only characters in the file  */
                if(tmp != '0' && tmp != '1' && tmp != '?' && tmp != '\n'){
                        fprintf(stderr, "Invalid character \'%c\' in policy "
                                "file. Aborting... \n", tmp);
                        exit(EXIT_FAILURE);
                }

                if(tmp == '\n'){
                        pol.n++;
                        if(curr_bits > pol.b) 
                                pol.b = curr_bits;
                        curr_bits = 0;
                }else {
                        curr_bits ++ ;
                }
        }

#if DEBUG
        fprintf(stderr, "%"PRIu64" rules parsed, with a max rule size of"
                " %"PRIu64" bits.\n", pol.n, pol.b);
#endif
        
        /* Allocate the array since we know the max size. The additional 2 is
         * for the \n\0 */    
        char ** rule_array = (char**) array2d_alloc(pol.n,  pol.b + 2);
           
        rewind(file);
        getline(&tmpstring, &n, file); /*consume the first line again.*/
        free(tmpstring);

        /* Read in the rest of the file */
        for (uint64_t i = 0; i < pol.n; i++){
                fgets(rule_array[i], pol.b + 2, file);
        }
        
        /* We convert bits to bytes rounding up to the next byte */
        pol.B = 8 * (uint64_t) ceil(pol.b/8.0);
        pol.N = 8 * (uint64_t) ceil(pol.n/8.0);

        /* Allocate space for ? masks. */
        pol.q_masks = array2d_alloc(pol.n, pol.B/8);

        /* Allocate space for 0,1 masks */
        pol.b_masks = array2d_alloc(pol.n, pol.B/8);
        
        /* Convert the ascii rules into bitmasks  */
        parse_q_masks(pol.n, rule_array, pol.q_masks);
        parse_b_masks(pol.n, rule_array, pol.b_masks);

        /* ascii rules are no longer needed */
        array2d_free((uint8_t**) rule_array);
        
        return pol;
}

/* Parse the rule files into ? masks   */
void parse_q_masks(uint64_t rule_count, char ** rule_array, uint8_t ** q_masks)
{
        /* Go through each character in the rules and set the corresponding bit
         * in the q_mask array to 0 or 1  */
        for(uint64_t i = 0; i < rule_count; ++i){
                for(uint64_t j = 0; rule_array[i][j] != '\n'; ++j){
                        if (rule_array[i][j] == '?') 
                                BitFalse(q_masks[i], j); 
                        else 
                                BitTrue(q_masks[i], j);
                }
        }
}

/* Parse the rule files into bitmasks  */
void parse_b_masks(uint64_t rule_count, char ** rule_array, uint8_t ** b_masks) 
{
        /* Go through each character in the rules and set the
         * corresponding bit in the b_mask array to 0 or 1  */
        for(uint64_t i = 0; i < rule_count; ++i){
                for(uint64_t j = 0; rule_array[i][j] != '\n'; ++j){
                        if (rule_array[i][j] == '0' || rule_array[i][j]=='?') 
                                BitFalse(b_masks[i],j);
                        if (rule_array[i][j] == '1') 
                                BitTrue(b_masks[i], j);
                }
        }
}

/* Print out a bitmask table */
void print_masks(uint8_t ** q_masks, uint64_t height, uint64_t width)
{
        for (uint64_t i = 0; i < height; ++i){
                for(uint64_t j = 0; j < width; j++){
                        printbits(q_masks[i][j]);
                        fprintf(stderr," ");
                }
                fprintf(stderr, "\n");
        }
}

/* Creates a binary representation of a binary string.  Adapted from:
   http://stackoverflow.com/questions/699968/display-the-binary-representation-of
   -a-number-in-c  */
void printbits(uint8_t v)
{
        for(int i = 0; i < 8; ++i) putc('0' + ((v >> i) & 1), stderr);
}

/* Allocates a 2D array of uint8_t in a contiguous block of memory
 * which is initialized to zeros
 * width is in bytes */
uint8_t ** array2d_alloc(uint64_t height, uint64_t width)
{
        uint8_t ** arr = malloc(height * sizeof(uint8_t*));
        arr[0] = calloc(height, sizeof(uint8_t) * width);
        for(uint64_t i = 1; i < height; i++)
                arr[i] = arr[0] + i * width;
        return arr;
}

/* Frees a 2d array that has been previously created by array2d_alloc */
void array2d_free(uint8_t ** arr)
{
        free(arr[0]);
        free(arr);
}

/* Fills a filtering table given a policy  */
void fill_tables(policy pol,
                 table_dims dims,
                 uint8_t even_tables[dims.even_h][dims.even_d][dims.bytewidth],
                 uint8_t odd_tables [dims.odd_h][dims.odd_d][dims.bytewidth])
{
        /* Precalculate even array Byte width */
        uint64_t e_array_Bwidth= (uint64_t) ceil(dims.even_s/8.0);

        for (uint64_t d = 0; d < dims.even_d; ++d){
                /* This next loop iterates to pol.n instead of dims.bitwidth
                 * because there are only n rules in pol.q_masks and
                 * pol.b_masks*/
                for(uint64_t w = 0; w < pol.n; ++w){
                        /* Create a temporary array to copy the relevant q_mask
                         * bits into */
                        uint8_t q_temp[e_array_Bwidth];
                        memset(q_temp, 0, e_array_Bwidth); /* zero out */
                        /* Copy the relevant bits into the temp array */
                        copy_section(pol.q_masks[w], q_temp, d*dims.even_s, 
                                     dims.even_s);
                        
                        /* Create a temporary array to copy the relevant b_mask
                         * bits into */
                        uint8_t b_temp[e_array_Bwidth];
                        memset(b_temp, 0, e_array_Bwidth); /* zero out */
                        /* Copy the relevant bits into the temp array */
                        copy_section(pol.b_masks[w], b_temp, d*dims.even_s, 
                                     dims.even_s); 
                        for(uint64_t h = 0; h < dims.even_h; ++h) {
                                uint8_t num_temp[e_array_Bwidth];
                                memset(num_temp, 0, 
                                       e_array_Bwidth*sizeof(uint8_t));
                                /* convert to big-endian */
                                uint64_t h_temp = __builtin_bswap64(h); 

                                /* Copy even_s bits of big-endian version of h
                                 * to the num_temp */
                                copy_section((uint8_t*) &h_temp, num_temp, 
                                             64 - (dims.even_s*(dims.even_d-d) + 
                                                   dims.odd_s*dims.odd_d),
                                             dims.even_s);
                                /* fprintf(stderr, "\nmasks (%lu,%lu,%lu)\n",h,d,w); */
                                /* print_mem(pol.q_masks[w], 1, 8);         /\* DEBUG *\/ */
                                /* print_mem(pol.b_masks[w], 1, 8);         /\* DEBUG *\/ */
                                /* print_mem((uint8_t*)&h_temp, 8, 8);      /\* DEBUG *\/ */
                                /* fprintf(stderr, "\ntemps (%lu,%lu,%lu)\n",h,d,w); /\* DEBUG *\/ */
                                /* print_mem(q_temp, e_array_Bwidth,8);     /\* DEBUG *\/ */
                                /* print_mem(b_temp, e_array_Bwidth,8);     /\* DEBUG *\/ */
                                /* print_mem(num_temp, e_array_Bwidth,8);   /\* DEBUG *\/ */

                                /* Set the appropriate bit in the lookup table
                                 * to 1 or 0 depending on whether the rule
                                 * matches */
                                if(rule_matches(num_temp, q_temp, b_temp, 
                                                e_array_Bwidth)){
                                        BitTrue(&even_tables[h][d][0], w);
                                }
                        }
                }
        }
        /* Precalculate odd array size */
        uint64_t o_array_Bwidth = (uint64_t) ceil(dims.odd_s/8.0);
        /* Offset to get to the beginning of the odd sections of the b and q
         * masks */
        uint64_t offset = dims.even_d * dims.even_s;
        fprintf(stderr, "Odds %d\n", __LINE__); /* DEBUG */
        for (uint64_t d = 0; d < dims.odd_d; ++d){
                /* This next loop iterates to pol.n instead of dims.bitwidth
                 * because there are only n rules in pol.q_masks and
                 * pol.b_masks */
                for(uint64_t w = 0; w < pol.n; ++w){
                        /* Create a temporary array to copy the relevant q_mask
                         * bits into */
                        uint8_t q_temp[o_array_Bwidth];
                        memset(q_temp, 0, o_array_Bwidth); /* zero out */
                        /* Copy the relevant bits into the temp array */
                        copy_section(pol.q_masks[w], q_temp,
                                     offset + d*dims.odd_s, dims.odd_s);
                        
                        /* Create a temporary array to copy the relevant b_mask
                         * bits into */
                        uint8_t b_temp[o_array_Bwidth];
                        memset(b_temp, 0, o_array_Bwidth); /* zero out */
                        /* Copy the relevant bits into the temp array */
                        copy_section(pol.b_masks[w], b_temp, 
                                     offset + d*dims.odd_s, dims.odd_s);
                        for(uint64_t h = 0; h < dims.odd_h; ++h){
                                uint8_t num_temp[o_array_Bwidth];
                                memset(num_temp, 0, o_array_Bwidth);
                                /* convert to big-endian */
                                uint64_t h_temp = h;//__builtin_bswap64(h);
                                /* Copy even_s bits of big-endian version of h
                                 * to the h_temp */
                                copy_section((uint8_t*) &h_temp, num_temp,
                                             0,
                                             dims.odd_s);
                                fprintf(stderr, "\nmasks (%"PRIu64",%lu,%lu)\n",h,d,w);
                                print_mem(pol.q_masks[w], 1, 8);         /* DEBUG */
                                print_mem(pol.b_masks[w], 1, 8);         /* DEBUG */
                                print_mem((uint8_t*) &h_temp, 8, 8);      /* DEBUG */
                                fprintf(stderr, "\ntemps (%lu,%lu,%lu)\n",h,d,w); /* DEBUG */
                                print_mem(q_temp, o_array_Bwidth,8);     /* DEBUG */
                                print_mem(b_temp, o_array_Bwidth,8);     /* DEBUG */
                                print_mem(num_temp, o_array_Bwidth,8);   /* DEBUG */

                                /* Set the appropriate bit in the lookup table
                                 * to 1 or 0 depending on whether the rule
                                 * matches */
                                if(rule_matches(num_temp, q_temp, b_temp, 
                                                o_array_Bwidth)){
                                        BitTrue(&odd_tables[h][d][0], w);
                                }
                        }
                }
        }
}
        
/* test whether a given byte array matches the b_array after being
 * masked by the q_array  */
bool rule_matches(uint8_t input[], const uint8_t q_mask[], 
                  const uint8_t b_mask[], uint64_t size){
        /* test for equality a byte at a time */
        for (uint64_t i = 0; i < size; ++i){
                if((input[i] & q_mask[i]) != b_mask[i]){
                        return false;
                }
        }
        return true;
}

/* Copies a section of a bit array to the beginning of another bit array */
void copy_section(const uint8_t * src_array, uint8_t * dst_array, 
                  uint64_t startbit, uint64_t size){
        for (uint64_t i = startbit; i < startbit + size; i++){
                if(BitIsTrue(src_array, i)){
                        BitTrue(dst_array, i - startbit);
                }else{
                        BitFalse(dst_array, i - startbit);
                }
        }
} 
        
/* Creates a single table for rule matching */
uint8_t ** create_single_table(policy pol){
        uint64_t height = (uint64_t) exp2(pol.b);
        uint64_t width  = 8 * (uint64_t) ceil(ceil(lg(pol.n)) / 8.0);
        uint8_t (*table)[width] = calloc(height, width);
        
        for(uint64_t i = 0; i < height; ++i){
                
                /* TODO: complete this code */

        }        
}

/* Prints a contiguous area of memory in binary starting with ptr to an array of
 * the given size */
void print_mem(uint8_t * ptr, uint64_t size, uint64_t cols){
        for(uint64_t i = 0; i < size; ++i){
                printbits(ptr[i]);
                if ((i+1) % cols == 0 || i == size - 1) fprintf(stderr,"\n");
                else fprintf(stderr," ");
        }
}

/* Prints out lookup tables in a readable format */
void print_tables(uint64_t h, uint64_t d, uint64_t w, uint8_t tables[h][d][w]){
        for( uint64_t j = 0; j < d; ++j){
                fprintf(stderr,"Table #%"PRIu64":\n",j+1);
                for (uint64_t i = 0; i < h; ++i){
                        fprintf(stderr,"%"PRId64" ",i);
                        for( uint64_t k = 0; k < w; ++k){
                                printbits(tables[j][i][k]);
                                fprintf(stderr," ");
                        }
                        fprintf(stderr,"\n");
                }
                fprintf(stderr,"-------------------------------------------\n");
        }
}

/* Filters incoming packets and classifies them to stdout */
void read_input_and_classify(policy pol, table_dims dim, 
                     uint8_t even_tables[dim.even_h][dim.even_d][dim.bytewidth], 
                     uint8_t odd_tables[dim.odd_h][dim.odd_d][dim.bytewidth])
{
        uint64_t loop_count = 0;
        /* Process input until EOF */
        while(true){
                /* Place to store the current input */
                uint8_t inpacket[dim.bytewidth];

                /* Read in bytewidth bytes, and break if less than that is read
                 * in */
                if (fread(inpacket, sizeof(uint8_t), dim.bytewidth, stdin) 
                    < dim.bytewidth){
                        break;
                }

                /* Create a temporary spot for reading in the inpacket  */
                uint64_t even_index[dim.even_d];
                uint64_t odd_index[dim.odd_d];
                /* Zero out byte arrays */
                memset(even_index, 0, dim.even_d*sizeof(uint64_t));
                memset(odd_index, 0, dim.odd_d*sizeof(uint64_t));
                
                /* Copy in sections of inpacket to even_index array*/
                for (uint64_t i = 0; i < dim.even_d; ++i){
                        copy_section(inpacket,
                                     (uint8_t*) &even_index[i], 
                                     i*dim.even_s, dim.even_s);
                }

                /* precompute bit offset of odd sections  */
                uint64_t offset = dim.even_d*dim.even_s;

                /* Copy in sections of inpacket to odd index arrays */
                for (uint64_t i = 0; i < dim.odd_d; ++i){
                        copy_section(inpacket,
                                     (uint8_t*) &odd_index[i], 
                                     offset + i*dim.odd_s, dim.odd_s);
                }
                
                /* create a running total to decide which rule is satisfied */
                uint8_t bit_total[pol.N/8];
                /* Zero out bit_total (may be a bit paranoid) */
                memset(bit_total, 0, pol.N/8);
                /* Copy the first bit array into the total */
                memcpy(bit_total, &even_tables[even_index[0]][0][0], pol.N/8);
                /* Create a temporary variable to and with the total */
                uint8_t tmp[pol.N/8];

                /* Loop through the remaining even bit arrays and AND them with
                 * the total */
                for(uint64_t i = 1; i < dim.even_d;++i){
                        memcpy(tmp, &even_tables[even_index[i]][i][0], pol.N/8);
                        and_bitarray(bit_total,tmp, pol.N/8);
                }

                /* Loop through the remaining odd bit arrays and AND them with
                 * the total */
                for(uint64_t i = 0; i < dim.odd_d; ++i){
                        memcpy(tmp, &odd_tables[odd_index[i]][i][0], pol.N/8);
                        and_bitarray(bit_total,tmp, pol.N/8);
                }
                
                for(uint64_t i = 0; i < pol.N; ++i){
                        if(BitValue(bit_total,i) == true){
                                fprintf(stdout,"%"PRIu64"\n",i);
                                break;
                        }
                }
                loop_count++;
        }

#if DEBUG
        fprintf(stderr, "Packets read in: %"PRIu64"\n", loop_count);
#endif

}

/* AND two bit arrays together, the second argument holds the results */
void and_bitarray(const uint8_t* new, uint8_t* total, uint64_t size){
        for (uint64_t i = 0; i < size; ++i){
                total[i] &= new[i];
        }
}