/* Contains utility functions that may be needed by more than one file */
#pragma once

#include <string.h>

/* Mallocs a new string the same size as source, copies the source string to it,
 * then returns the pointer to it */
char * malloc_strcpy(char * source)
{ 
        size_t length = strlen(source) + 1; /* add 1 for nul character */
        char * dest = malloc(sizeof(char) * length);
        strncpy(source, dest, length);

        return dest;
}
