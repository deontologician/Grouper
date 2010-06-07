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

#include "grouper.h"

int main(int argc, char* argv[])
{
        profile_t outer_time, inner_time;
        long total_time, read_time, build_time, real_process_time;
        clock_t cpu_process_time;   /* We measure processing time in CPU seconds */
        start_timing(&outer_time);
        
        /* Check for the proper number of arguments. Print usage if wrong
         * number */
        if (argc < 3){
                Error( 
                        "Usage: %s <max memory> <policy file.pol> [<input file>]"
                        " [<output file>]\n", argv[0] );
                exit(EXIT_FAILURE);
        }
        /* Parse the number of memory bits available */
        /* The input is in bytes, so convert it to bits for the algorithm */
        uint64_t memsize_bits = atoll(argv[1]) * 8; 
  
        /* open the policy file*/
        FILE *pol_file = fopen(argv[2], "r");
        /* Exit if file fails to open */
        if (pol_file == NULL){
                Error( "Invalid policy file: \'%s\' \n", argv[2]);
                exit(EXIT_FAILURE);
        }

        /* Check for input file & ensure it can be opened. */
        if(argc >= 4){
                FILE * in_temp = stdin;
                stdin = fopen(argv[3], "r");
                if(stdin == NULL){
                        Error("Input file '%s' is invalid or "
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
                        Error("Output file '%s' cannot be opened for "
                                "writing. Falling back to stdout.\n", argv[4]);
                        stdout = out_temp;
                }
        }
        
        start_timing(&inner_time);
        policy pol = read_policy(pol_file);
        fclose(pol_file);
        read_time = end_timing(&inner_time);
        Trace("Took %ld microseconds to finish reading the input file.\n", read_time);

        /* Calculate number of tables required.  */
        uint64_t t = min_tables(memsize_bits, pol.n , pol.b);

        if (t == TABLE_ERROR){
                Error("Error: not enough memory to build tables. "
                      "Needs at least %"PRIu64" bytes.\n",
                      ceil_div((2*pol.N*pol.b),8));
                exit(EXIT_FAILURE);
        }

        Trace( "%"PRIu64" tables needed for memory size of %"PRIu64
                " bits.\n",t, memsize_bits);

        /* Handle the special single table case */
        if (t == 1){
                /* Find the width of the binary representation of the number of
                 * rules in bytes  */
                uint64_t width = ceil_div(ceil(log2(pol.n)), 8);
                uint8_t (*single_table)[width];
                start_timing(&inner_time);
                single_table = (uint8_t (*)[width]) create_single_table(pol, width);
                array2d_free(pol.q_masks);
                array2d_free(pol.b_masks);
                build_time = end_timing(&inner_time);
                Trace("Took %ld microseconds to finish building single table\n",
                      build_time);

                start_timing(&inner_time);
                cpu_process_time = clock();
                /* Process packets with single table here */
                read_input_and_classify_single(pol, width, single_table);
                
                real_process_time = end_timing(&inner_time);
                cpu_process_time = clock() - cpu_process_time;
                Trace("Took %ld microseconds to finish processing with single table\n",
                        real_process_time);
                
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

                Trace("\nCreating %"PRIu64" tables %"PRIu64" of "
			"which will be %"PRIu64" x %"PRIu64",\nand %"PRIu64" of "
			"which will be %"PRIu64" x %"PRIu64".\n\n",t, d.even_d, 
			d.bitwidth, d.even_h, d.odd_d, d.bitwidth, d.odd_h);

                /* Create two large table arrays */
                uint8_t (*even_tables)[d.even_d][d.bytewidth] =
                        calloc(d.even_h * d.even_d, 
                               sizeof(uint8_t[d.bytewidth]));
                if(even_tables == NULL){
                        Error("Could not allocate memory for even tables!"
                              " errno = %d\n", errno);
                        exit(EXIT_FAILURE);
                }
                uint8_t (*odd_tables) [d.odd_d] [d.bytewidth] = 
                        calloc(d.odd_h  * d.odd_d, 
                               sizeof(uint8_t[d.bytewidth]));
                if(odd_tables == NULL){
                        Error("Could not allocate memory for odd tables!"
                              " errno = %d\n", errno);
                        exit(EXIT_FAILURE);
                }
                
                start_timing(&inner_time);
                fill_tables(pol, d, even_tables, odd_tables);

                /* Free intermittant resources */
                array2d_free(pol.q_masks);
                array2d_free(pol.b_masks);
                pol.q_masks = NULL;
                pol.b_masks = NULL;
                
                build_time = end_timing(&inner_time);
                Trace("Took %ld microseconds to finish building tables.\n",build_time);

                /* Read input and classify input until EOF */
                start_timing(&inner_time);
                cpu_process_time = clock();
                read_input_and_classify(pol,d,even_tables,odd_tables);
                real_process_time = end_timing(&inner_time);
                cpu_process_time = clock() - cpu_process_time;
                Trace("Took (%ld cpu, %ld real) microseconds to finish processing "
                      "packets\n", cpu_process_time, real_process_time);
                
                /* Release resources: */
                free(even_tables);
                free(odd_tables);
                
        }
        
        total_time = end_timing(&outer_time);
        Trace("Took %ld microseconds total\n", total_time);
        /* We print the next line unconditionally for external tools to do
         * record keeping */
        fprintf(stderr, "{ 'read' : %ld, 'build' : %ld, 'cpu_process' : %ld,"
                " 'real_process' : %ld, 'total' : %ld }\n", read_time, build_time, 
                cpu_process_time, real_process_time, total_time);

        return EXIT_SUCCESS;
}

/* Determine the minimum number of tables that will fit in a
   prescribed amount of memory 
   m is max memory used in bits
   n is the number of rules
   b is the number of relevant pattern bits*/
uint64_t min_tables(uint64_t m , uint64_t n , uint64_t b)
{
        /* Note: all values represent a number of bits in this function, the
         * caller will convert to necessary bytes etc */

        /* Check to ensure n and b are positive, and that the max memory is
           large enough to hold the smallest tables for the number of rules
           specified. Return error if not. */
        uint64_t N = 8 * ceil_div(n, 8); // n rounded to the next multiple of 8
        if(m < 2 * N * b 
           || n < 1 || b < 1) return TABLE_ERROR;

        /* If the amount of memory available is larger than the amount
         * needed for a 1 table solution, return 1. 1 table is a special
         * case because it allows only using log2(n) bits to store the rules 
         *
         * In addition, we need to check to make (reasonably) sure overflow
         * isn't going to happen (when we convert a double to a uint64_t), so on
         * the assumption that the log of the number of rules is 3 bytes or
         * less, we won't allow bitlengths greater than 58 to be considered for
         * the one table solution. */
        if(b <= 58 && 
           m >= (8 * ceil_div((uint64_t)log2(n),8)) * (uint64_t)exp2(b) ) return 1;

        /* Initial highest number of tables that might be needed */
        uint64_t high = ceil_div(b,2);
        /* Initial lowest number of tables that might be needed  */
        uint64_t low = 1;
        /* variables used in calculation */
        uint64_t memNeededForMidTables = 0;
        uint64_t mid = 0;
        /* Binary search through possible table numbers. */
        while((high - low) > 1) {
                mid = (high + low)/2;
                memNeededForMidTables = 
                        (mid - (b % mid))*exp2(b/mid) * N + 
                        (b%mid)*exp2((b/mid) + 1) * N;
                if(m < memNeededForMidTables){
                        low = mid;
                }else{ 
                        high = mid;
                }
        }
        return high;
}

/* reads in a policy from a file and creates the relevant patterns in memory */
policy read_policy(FILE * file)
{
        policy pol = POLICY_INIT;
        uint64_t curr_bits = 0;
        char tmp;
        size_t n = 80;
        char * tmpstring = malloc(sizeof(char[n]));

        /* Consume the first line */
        getline(&tmpstring, &n, file); 
        
        pol.pl = atoll(tmpstring);


        Trace("Packet length: %"PRIu64"\n", pol.pl);


        /* Scan the rest of the file, finding the number of lines and the max
         * line length (which will be the max relevant bits */
        while(fscanf(file, "%c", &tmp) != EOF){
                /* Ensure 1,0,?,\n are the only characters in the file  */
                if(tmp != '0' && tmp != '1' && tmp != '?' && tmp != '\n'){
                        Trace( "Invalid character \'%c\' in policy "
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


        Trace( "%"PRIu64" rules parsed, with a max rule size of "
               "%"PRIu64" bits.\n", pol.n, pol.b);
        
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
        pol.B = 8 * ceil_div(pol.b, 8);
        pol.N = 8 * ceil_div(pol.n, 8);

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
                                BitFalse(q_masks[i], PackingIndex(j)); 
                        else 
                                BitTrue(q_masks[i], PackingIndex(j));
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
                        if (rule_array[i][j] == '0' || rule_array[i][j]=='?'){ 
                                BitFalse(b_masks[i],PackingIndex(j));}
                        if (rule_array[i][j] == '1'){
                                BitTrue(b_masks[i], PackingIndex(j));}
                }
        }
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
        /* Determine number of threads to spawn based on the number of
         * cores. Empirically, (albeit with limited testing) it seems that
         * having threads per core be about half of the total number of tables
         * generates the best build time overall. */
        const uint64_t max_threads = sysconf(_SC_NPROCESSORS_ONLN) * 
                min(MIN_THREADS_PER_CORE, ceil_div(dims.even_d + dims.odd_d, 2));
        uint64_t active_threads = 0; /* How many unjoined threads exist */
        /* Create an array for handling threads */
        pthread_t even_threads[dims.even_d];
        thread_args even_args[dims.even_d];
        pthread_t odd_threads[dims.odd_d];
        thread_args odd_args[dims.odd_d];

        /* These two variables keep track of the minimum index at which there
         * could be an open thread in the thread arrays (in other words we have
         * joined all threads at lower indices) */
        uint64_t min_even_thread = 0;
        uint64_t min_odd_thread = 0;

        /* Initialize the threads */
        for (uint64_t i = 0; i < dims.even_d; ++i){
                if(active_threads == max_threads){
                        /* We've hit the maximum number of active threads so we
                         * need to join some before continuing */
                        for(;min_even_thread<i; min_even_thread++, active_threads--){
                                pthread_join(even_threads[min_even_thread], NULL);
                        }
                }
                /* We do some assigning to auto variables here because all
                 * threads will be joined before this function returns. */
                even_args[i].pol = &pol;
                even_args[i].dims = &dims;
                even_args[i].table_num = i;
                even_args[i].tables = (uint8_t *) even_tables;
                pthread_create(&even_threads[i], NULL, fill_even_table, 
                               (void *) &even_args[i]);
                active_threads++;
        }
        for (uint64_t i = 0; i < dims.odd_d; ++i){
                if(active_threads == max_threads){
                        /* We've hit the maximum number of active threads so we
                         * need to join some before continuing */
                        /* First join any even threads */
                        for(; min_even_thread < dims.even_d;
                            min_even_thread++, active_threads--){
                                pthread_join(even_threads[min_even_thread], NULL);
                        }
                        /* Then join any odd threads */
                        for(;min_odd_thread<i;min_odd_thread++, active_threads--){
                                pthread_join(odd_threads[min_odd_thread], NULL);
                        }
                }
                /* We do some assigning to auto variables here because all
                 * threads will be joined before this function returns. */
                odd_args[i].pol = &pol;
                odd_args[i].dims = &dims;
                odd_args[i].table_num = i;
                odd_args[i].tables = (uint8_t *) odd_tables;
                
                pthread_create(&odd_threads[i], NULL, fill_odd_table, 
                               (void *) &odd_args[i]);
                active_threads++;
        }
        /* join all remaining even threads */
        for(uint64_t i = min_even_thread; i < dims.even_d; i++, active_threads--){
                pthread_join(even_threads[i], NULL);
        }
        /* join all remaining odd threads */
        for(uint64_t i = min_odd_thread; i < dims.odd_d; i++, active_threads--){
                pthread_join(odd_threads[i], NULL);
        }
        
}

/* Function to fill a single even table (called by fill_tables threads)  */
void * fill_even_table(void * args)
{
        /* Unpack args from struct pointer */
        policy * pol = ((thread_args*)args)->pol;
        table_dims * dims = ((thread_args*)args)->dims;
        uint64_t d = ((thread_args*)args)->table_num;
        uint8_t (* even_tables)[dims->even_h][dims->even_d][dims->bytewidth] = 
                /* Big ugly cast to eliminate pointer warnings */
                (uint8_t (*) [dims->even_h][dims->even_d][dims->bytewidth])
                ((thread_args*)args)->tables;

        /* Precalculate even array Byte width */
        uint64_t e_array_Bwidth = ceil_div(dims->even_s, 8);

        Trace("Generating even table %"PRIu64"\n",d);
        /* This next loop iterates to pol.n instead of dims.bitwidth
         * because there are only n rules in pol.q_masks and
         * pol.b_masks*/
        for(uint64_t w = 0; w < pol->n; ++w){
                /* Create a temporary array to copy the relevant q_mask
                 * bits into */
                uint8_t q_temp[e_array_Bwidth];
                memset(q_temp, 0, e_array_Bwidth); /* zero out */
                /* Copy the relevant bits into the temp array */
                copy_section(pol->q_masks[w], q_temp, d*dims->even_s, 
                             dims->even_s);
                /* Create a temporary array to copy the relevant b_mask
                 * bits into */
                uint8_t b_temp[e_array_Bwidth];
                memset(b_temp, 0, e_array_Bwidth); /* zero out */
                        
                /* Copy the relevant bits into the temp array */
                copy_section(pol->b_masks[w], b_temp, d*dims->even_s, 
                             dims->even_s);
                        
                for(union64 h = {.num = 0}; h.num < dims->even_h; ++h.num) {
                        uint8_t num_temp[e_array_Bwidth];
                        memset(num_temp, 0, e_array_Bwidth * sizeof(uint8_t));
                        memcpy(num_temp, h.arr, e_array_Bwidth);
                        /* Set the appropriate bit in the lookup table
                         * to 1 or 0 depending on whether the rule
                         * matches */
                        if(rule_matches(e_array_Bwidth,num_temp,q_temp,b_temp)){
                                BitTrue(&((*even_tables)[h.num][d][0]), w);
                        }
                }
        }
        return NULL;
}

/* Function to fill a single odd table (called by fill_tables threads)  */
void * fill_odd_table(void * args)
{
        /* Unpack args from struct pointer */
        policy * pol = ((thread_args*)args)->pol;
        table_dims * dims = ((thread_args*)args)->dims;
        uint64_t d = ((thread_args*)args)->table_num;
        uint8_t (* odd_tables)[dims->odd_h][dims->odd_d][dims->bytewidth] =
                /* Big ugly cast to eliminate pointer warnings */
                (uint8_t (*) [dims->odd_h][dims->odd_d][dims->bytewidth])
                ((thread_args*)args)->tables;

        Trace("Generating odd table %"PRIu64"\n", d);
        /* Precalculate odd array size */
        uint64_t o_array_Bwidth = ceil_div(dims->odd_s, 8);
        /* Offset to get to the beginning of the odd sections of the b and q
         * masks */
        uint64_t offset = dims->even_d * dims->even_s;
        for(uint64_t w = 0; w < pol->n; ++w){
                /* Create a temporary array to copy the relevant q_mask
                 * bits into */
                uint8_t q_temp[o_array_Bwidth];
                memset(q_temp, 0, o_array_Bwidth); /* zero out */
                /* Copy the relevant bits into the temp array */
                copy_section(pol->q_masks[w], q_temp,
                             offset + d*dims->odd_s, dims->odd_s);

                /* Create a temporary array to copy the relevant b_mask
                 * bits into */
                uint8_t b_temp[o_array_Bwidth];
                memset(b_temp, 0, o_array_Bwidth); /* zero out */
                /* Copy the relevant bits into the temp array */
                copy_section(pol->b_masks[w], b_temp, 
                             offset + d*dims->odd_s, dims->odd_s);
                for(union64 h = {.num = 0}; h.num < dims->odd_h; ++h.num){
                        uint8_t num_temp[o_array_Bwidth];
                        memset(num_temp, 0, o_array_Bwidth);
                        /* Copy even_s bits of the number into the
                           temporary matching variable*/
                        memcpy(num_temp, h.arr, o_array_Bwidth);
                        //copy_section(h.arr, num_temp,0,dims.odd_s);
                        /* Set the appropriate bit in the lookup table
                         * to 1 or 0 depending on whether the rule
                         * matches */
                        if(rule_matches(o_array_Bwidth,num_temp,q_temp,b_temp)){
                                BitTrue(&((*odd_tables)[h.num][d][0]), w);
                        }
                }
        }
        return NULL;
}

        
/* test whether a given byte array matches the b_array after being
 * masked by the q_array  */
bool rule_matches( uint64_t size, uint8_t input[size], const uint8_t q_mask[size], 
                  const uint8_t b_mask[size])
{
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
                  uint64_t startbit, uint64_t size)
{
        for (uint64_t i = startbit; i < startbit + size; i++){
                if(BitIsTrue(src_array, i)){
                        BitTrue(dst_array, i - startbit);
                }else{
                        BitFalse(dst_array, i - startbit);
                }
        }
} 
        
/* Creates a single table for rule matching */
uint8_t * create_single_table(policy pol, uint64_t width)
{
        uint64_t height = (uint64_t) exp2(pol.b);
        uint8_t (*table)[width] = calloc(height, width);
        /* For each possible input bitarray*/
        for(union64 i = {.num = 0}; i.num < height; i.num++){
                /* Check each rule to see which is the first match 
                 * j starts at 1, since rule 0 means "no match" */
                for(union64 j = {.num = 1}; j.num <= pol.n; j.num++){
                        /* Mask and compare with the current i */
                        if((i.num & (uint64_t)pol.q_masks[j.num - 1]) 
                           != (uint64_t)pol.b_masks[j.num - 1]){
                                Trace("Input %"PRIu64" (",i.num);
                                for(uint64_t k = pol.B/8; k != 0; k--){
                                        printbits(i.arr[k-1]);
                                        Trace(" ");
                                }
                                Trace(") matches rule %"PRIu64"\n", j.num);
                                /* Set the table row equal to the rule number
                                 * that matched. Note that this way of doing it
                                 * is dependent on a little-endian integer
                                 * representation. A portable implementation
                                 * will need to do something more complicated */
                                memcpy(table[i.num], j.arr, width);
                                break;
                        }
                        /* A convenient property here is that if the loop to
                         * match rules falls through without finding a match, it
                         * automatically matches rule 0, since the memory for
                         * the table was calloc'd  */
                }
        }
        return (uint8_t*) table;
}

/* Classify packets with a single table */
void read_input_and_classify_single(policy pol, 
                                    uint64_t width, 
                                    uint8_t (*table)[width])
{
        uint64_t packets_read = 0;
        union64 inpacket = {.num = 0}; /* Current input temp */
        union64 rule_matched = {.num = 0}; /* It is important this is zeroed as
                                            * we will be copying into its least
                                            * significant bytes only, relying on
                                            * the most significant bytes to
                                            * remain zero.*/
        while(fread(inpacket.arr, sizeof(uint8_t), pol.pl, stdin) == pol.pl){
                memcpy(rule_matched.arr, table[inpacket.num], width);
                Print("%"PRIu64"\n", rule_matched.num);
                packets_read++;
        }
        Trace("Packets read in: %"PRIu64"\n", packets_read);
}

/* Filters incoming packets and classifies them to stdout */
void read_input_and_classify(policy pol, table_dims dim, 
                     uint8_t even_tables[dim.even_h][dim.even_d][dim.bytewidth], 
                     uint8_t odd_tables[dim.odd_h][dim.odd_d][dim.bytewidth])
{
        
        uint64_t packets_read = 0; 
        /* Place to store the current input */
        uint8_t inpacket[pol.pl];

        /* Read in bytewidth bytes and end if less than that is read in */
        while(fread(inpacket, sizeof(uint8_t), pol.pl, stdin) == pol.pl){
                packets_read++;
                /* Create a temporary spot for reading in the inpacket */
                union64 even_index[dim.even_d];
                union64 odd_index[dim.odd_d];
                /* Zero out index arrays */
                memset(even_index, 0, dim.even_d * sizeof(union64));
                memset(odd_index, 0, dim.odd_d * sizeof(union64));
                
                /* Copy in sections of inpacket to even_index array*/
                for (uint64_t i = 0; i < dim.even_d; ++i){
                        copy_section(inpacket,
                                     even_index[i].arr, 
                                     i*dim.even_s, dim.even_s);
                }
                
                /* precompute bit offset of odd sections  */
                uint64_t offset = dim.even_d * dim.even_s;
                /* Copy in sections of inpacket to odd_index array*/
                for (uint64_t i = 0; i < dim.odd_d; ++i){
                        copy_section(inpacket,
                                     odd_index[i].arr,
                                     offset + i*dim.odd_s, dim.odd_s);
                }

                /* create a running total to decide which rule is satisfied */
                uint8_t bit_total[pol.N/8];
                /* Zero out bit_total */
                memset(bit_total, 0, pol.N/8);
                /* Copy the first bit array into the total Note that we must
                 * have at least one even section, its the odd sections that may
                 * not exist. So it is ok for the next for loop to start at 1
                 * instead of 0 */
                memcpy(bit_total, &even_tables[even_index[0].num][0][0], pol.N/8);

                /* Loop through the remaining even bit arrays and AND them with
                 * the total */
                for(uint64_t i = 1; i < dim.even_d;++i){
                        and_bitarray(&even_tables[even_index[i].num][i][0], 
                                     bit_total, pol.N/8);
                }

                 /* Loop through the remaining odd bit arrays and AND them with
                 * the total */
                for(uint64_t i = 0; i < dim.odd_d; ++i){
                        and_bitarray(&odd_tables[odd_index[i].num][i][0],
                                     bit_total, pol.N/8);
                }
                /* Loop through and determine the first bit that is set, if none
                 * is set, we say rule 0 is matched. */
                bool match = false;
                for(uint64_t i = 0; i < pol.N; ++i){
                        if(BitValue(bit_total,i) == true){
                                Print("%"PRIu64"\n",i+1);
                                //Trace("Packet %"PRIu64" ",packets_read);
                                //Trace(" matched rule %"PRIu64"\n",i+1);
                                match = true;
                                break;
                        }
                }
                if (!match) { 
                        Print("0\n");
                        //Trace("Packet %"PRIu64" ",packets_read);
                        //Trace(" matched rule 0\n");
                }
        }

        Trace("Packets read in: %"PRIu64"\n", packets_read);

}

/* AND two bit arrays together, the second argument holds the results */
static inline void and_bitarray(const uint8_t* new, uint8_t* total, uint64_t size)
{
        for (uint64_t i = 0; i < size; ++i){
                total[i] &= new[i];
        }
}

/* Rounds up the result of integer division */
static inline uint64_t ceil_div(uint64_t num, uint64_t denom)
{
        return (num + denom - 1) / denom;
}

/* Starts timing a section of code */
void start_timing(profile_t * time)
{
        gettimeofday(time, NULL);
}

/* Ends timing a section of code and returns the number of microseconds elapsed */
long end_timing(profile_t * time)
{
        long mtime, seconds, useconds;
        struct timeval end_time;
        gettimeofday(&end_time, NULL);
        
        seconds = end_time.tv_sec - time->tv_sec;
        useconds = end_time.tv_usec - time->tv_usec;
        mtime = seconds * 1000000 + useconds;
        return mtime;
}
