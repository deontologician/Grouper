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

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>

int main(int argc, char ** argv)
{
        if(argc != 4){
                fprintf(stderr, "ERROR. Takes 3 arguments: bits, rules, filename\n");
                exit(EXIT_FAILURE);
        }
        long bits = atol(argv[1]);
        long rules = atol(argv[2]);
        char * filename = argv[3];
        if(bits <= 0){
                fprintf(stderr, "ERROR. bits must be > 0, got %ld\n", bits);
                exit(EXIT_FAILURE);
        }
        if(rules <= 0){
                fprintf(stderr, "ERROR. rules must be > 0, got %ld\n", rules);
                exit(EXIT_FAILURE);
        }
        if(strlen(filename) == 0){
                fprintf(stderr, "ERROR. filename must be nonzero length.\n");
                exit(EXIT_FAILURE);
        }
        struct timeval seed;
        gettimeofday(&seed, NULL);
        srand(seed.tv_sec * seed.tv_usec);
        
        FILE * outfile = fopen(filename, "w");
        fprintf(outfile, "%d\n", bits);
        int i,j;
        int random_rule;
        for(i = 0; i < rules; i ++){
                for(j = 0; j < bits; j++){
                        random_rule = (int)((3.0 * rand()) / (RAND_MAX + 1.0));
                        switch(random_rule){
                        case 0:
                                fputc('0', outfile);
                                break;
                        case 1:
                                fputc('1', outfile);
                                break;
                        case 2:
                                fputc('?', outfile);
                                break;
                        default:
                                fputc((char)48 + random_rule, outfile);
                        }
                }
                /* Add the newline */
                fputc('\n', outfile);
        }
        fclose(outfile);
        return EXIT_SUCCESS;
}
