#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <getopt.h>
#include "printing.h"
#include "util.h"
#include "regex.tab.c"

/* Globals for use in this file only */
static uint64_t mem_allowed = 0;
static char * pol_filename = NULL;
static char * input_filename = NULL;
static char * output_filename = NULL;

/* Prototypes */
static void parse_options(int argc, char ** argv); 
static void cleanup(void);
static void print_usage(char * argv[]);
static void fail(void);
static void succeed(void);


int main(int argc, char ** argv)
{
        parse_options(argc, argv);
        
        yyparse();
        
        succeed();
}

/* Parses command line options and checks for invalid arguments */
static void parse_options(int argc, char ** argv)
{
        extern char * optarg;

        static struct option lopts[] = 
                {
                        /* These options all take arguments*/
                        {"policy-file", required_argument,  NULL, 0},
                        {"input-file",  required_argument,  NULL, 0},
                        {"output-file", required_argument,  NULL, 0},
                        {"mem-allowed", required_argument,  NULL, 0},
                        {"help", no_argument, NULL, true},
                        {0, 0, 0, 0} /* This is needed as the terminating
                                      * element in the array */
                };

        int opt_index;          /* stores index of long option recognized */
        char c;                 /* stores  character of short option*/
        while((c = getopt_long(argc, argv, "p:i:o:m:h", 
                               lopts, &opt_index)) != -1){
                /* policy-file opt_index */
                if (c == 'p' || (c == 0 && opt_index == 0)){
                        pol_filename = malloc_strcpy(optarg);
                } 
                /* input-file option */
                else if (c == 'i' || (c == 0 && opt_index == 1)){
                        input_filename = malloc_strcpy(optarg);
                }
                /* output-file option */
                else if (c == 'o' || (c == 0 && opt_index == 2)){
                        output_filename = malloc_strcpy(optarg);
                }
                /* mem-allowed option */
                else if (c == 'm' || (c == 0 && opt_index == 3)){
                        mem_allowed = atol(optarg);
                }
                else if (c == 'h' || (c == 0 && opt_index == 4)){
                        print_usage(argv);
                        succeed();
                }
                /* Unrecognized option */
                else {
                        Error ("Ignoring unrecognized long option.\n");
                        fail();
                }
        }
        
        /* Now to open files and check for errors */
        if(pol_filename == NULL){
                Error("A policy file name must be specified with the -p option.\n");
                print_usage(argv);
                fail();
        }
        /* load the policy file */
        yyin = fopen(pol_filename, "r");
        if(yyin == NULL) {
                Error("Policy file '%s' could not be opened for reading\n"
                      , pol_filename);
                fail();
        }
        
        /* Check if memory specified was a valid amount. atol signals failure by
         * returning 0, so we don't know if it is 0 because the user specified 0
         * memory or because the -m option wasn't specified. We could set a flag
         * or something to differentiate these cases but it doesn't really
         * matter because either way it is an error. */
        if(mem_allowed <= 0){
                Error("Input a valid memory limit with the -m option.\n");
                print_usage(argv);
                fail();
        }
        
        /* change input if it was specified */
        if(input_filename != NULL){
                freopen(input_filename, "r", stdin);
                if(stdin == NULL){
                        Error("Input file '%s' could not be opened for reading\n"
                              , input_filename);
                        fail();
                }
        }
        
        /* change output if it was specified */
        if(output_filename == NULL){
                freopen(output_filename, "w", stdout);
                if(stdout == NULL){
                        Error("Output file '%s' could not be opened for writing\n"
                              , output_filename);
                        fail();
                }
        }
}

/* Convenience function to print out usage info */
static void print_usage(char * argv[])
{
        Error("Usage: %s\t[-m | --mem-allowed=MEM_ALLOWED]\n"
              "\t\t[-p | --policy-file=POLICY_FILE]\n"
              "\t\t[-i | --input-file=INPUT_FILE]\n"
              "\t\t[-o | --output-file=OUTPUT_FILE]\n"
              "\t\t[-h | --help]\n", argv[0]);
}

/* cleans up all resources etc. */
static void cleanup(void)
{
        if (pol_filename != NULL ) {free(pol_filename);}
        if (input_filename != NULL ){
                free(input_filename);
                fclose(stdin);
        }
        if (output_filename != NULL ){
                free(output_filename);
                fclose(stdout);
        }
}

static void fail( void )
{
        cleanup();
        exit(EXIT_FAILURE);
}

static void succeed( void )
{
        cleanup();
        exit(EXIT_SUCCESS);
}
