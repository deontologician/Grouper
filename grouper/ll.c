#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "printing.h"
#include "assert.h"

/* Node in a list */
typedef struct LISTNODE {
        uint32_t val;
        struct LISTNODE * next;
} listnode;

/* list data structure */
typedef struct {
        listnode * first;
        listnode * last;
} list;

/******************************** Prototypes ***********************************/
list * new_list( void );
static listnode * new_listnode( uint32_t val, const listnode * next );
bool empty( const list * l);

/******************************** Definitions **********************************/

/* Returns a new empty generic list container */
list * 
__attribute__ ((malloc, warn_unused_result))
new_list( void ) 
{
        list * new = malloc(sizeof(*new));
        if(new == NULL){
                fprintf(stderr, "Malloc failed.");
                abort();
        }
        new->first = NULL;
        new->last = NULL;
        return new;
}

/* Returns a new node with the given value and next node */
static listnode * 
__attribute__ ((malloc, warn_unused_result))
new_listnode( uint32_t val, const listnode * next )
{
        listnode * ln = malloc(sizeof(*ln));
        if(ln == NULL){
                fprintf(stderr,"Malloc failed.\n");
                abort();
        }
        ln->val = val;
        ln->next = (listnode *) next;
        return ln;
}

/* Returns whether the list is empty */
bool
empty( const list * l) 
{
        return (l->first == NULL);
}

/* Prepend a number to a list */
void
prepend( list * inlist, uint32_t val)
{
        listnode * tmp = new_listnode( val, inlist->first );
        if(empty(inlist)){
                inlist->last = tmp;
        }
        inlist->first = tmp;
}

/* Appends a number to a list */
void
append( list * inlist, uint32_t val)
{
        listnode * tmp = new_listnode( val, NULL );
        if(empty(inlist)){
                inlist->first = tmp;
        }else{
                inlist->last->next = tmp;
        }
        inlist->last = tmp;
}

/* Inserts a value into the list in its proper place if the list is sorted. If
 * unsorted, it inserts it somewhere that will at least not unsort the list
 * further */
void
insert( list * inlist, uint32_t val )
{
        if(empty(inlist)){
                append(inlist, val);
                return;
        }
        listnode * tmp = new_listnode(val, NULL);
        listnode * iter = inlist->first;
        listnode * trailing = NULL;
        while(true){
                if(iter == NULL){
                        /* We've gone through the entire list without a
                         * match and fell off the end, so tmp will
                         * become last */
                        trailing->next = tmp;
                        inlist->last = tmp;
                        break;
                }
                if(iter->val >= val){
                        /* We've found the place to insert tmp */
                        tmp->next = iter;
                        if(trailing == NULL){
                                /* trailing is only NULL when iter is
                                 * the first item in the list, so we
                                 * must update first to be tmp */
                                inlist->first = tmp;
                        }else{
                                trailing->next = tmp;
                        }
                        break;
                }
                trailing = iter;
                iter = iter->next;
        }
}

/* Return a pointer to a copy of the given list */
list *
__attribute__ ((malloc, warn_unused_result))
copylist(const list * inlist )
{
        list * retval = new_list();
        listnode * iter = inlist->first;
        listnode * last_copied = NULL;
        while(iter != NULL){
                listnode * tmp = new_listnode(iter->val, NULL);
                if(iter == inlist->first){
                        retval->first = tmp;
                }
                if(iter == inlist->last){
                        retval->last = tmp;
                }
                if(last_copied != NULL){
                        last_copied->next = tmp;
                }
                last_copied = tmp;
                iter = iter->next;
        }
        return retval;
}

/* Clean up and deallocate a list, returns NULL for convenience. It should be
 * called like this: 
 * my_list = delete_list(my_list);
 */
list * 
__attribute__ ((warn_unused_result))
delete_list( list * L )
{
        listnode * tmp = L->first;
        listnode * next = L->first->next;
        while(tmp != NULL){
                free(tmp);
                tmp = next;
                if(next != NULL){
                        next = next->next;
                }
        }
        free(L);
        return NULL;
}

/* Returns the greatest element in the list */
static uint32_t
__attribute__ ((warn_unused_result))
list_max(list * L)
{
        return L->last->val;
}

/* Returns the least element in the list */
static inline uint32_t
__attribute__ ((warn_unused_result))
list_min(list * L)
{
        return L->first->val;
}

/* Frees all items from the given node on, the given node becomes the
 * last node*/
inline void
truncate(list * L, listnode * last)
{
        listnode * iter = last->next;
        while(iter != NULL){
                listnode * tmp = iter->next;
                free(iter);
                iter = tmp;
        }
        last->next = NULL;
        L->last = last;
}

/* Perform the intersection of two lists, total becomes the result */
void
conjunction(list * total, const list * cmp)
{
        /* If either list is empty, the intersection is empty */
        if(empty(total)){
                return;
        }
        if(empty(cmp)){
                total = delete_list(total);
                total = new_list();
                return;
        }
        listnode * iter_t = total->first; /* iterator over total */
        listnode * iter_t_trail = NULL;   /* points to the previous valid
                                           * node */
        listnode * iter_c = cmp->first;   /* iterator over cmp */
        while(true){
                assert(iter_c != NULL);
                assert(iter_t != NULL);
                while(iter_c->val < iter_t->val){
                        if(iter_c->next == NULL){
                                /* cmp is over, so truncate the rest of total
                                 * and return*/
                                truncate(total, iter_t_trail);
                                return;
                        }
                        iter_c = iter_c->next;
                }
                assert(iter_c->val >= iter_t->val);
                while(iter_c->val == iter_t->val){
                        if(iter_c->next == NULL){
                                /* cmp is over, so truncate the rest of total
                                 * and return  */
                                truncate(total, iter_t);
                                return;
                        }
                        if(iter_t->next == NULL){
                                /* total is over */
                                return;
                        }
                        /* This item is in the final list, so move everything
                         * forward one list item */
                        iter_c = iter_c->next;
                        iter_t_trail = iter_t;
                        iter_t = iter_t->next;
                }
                assert(iter_c->val > iter_t->val);
                while(iter_t->val < iter_c->val){
                        /* iter_t is not in the intersection, so we must
                         * gracefully dispose of it */
                        if(iter_t->next == NULL){
                                /* total is over, so free this item, and set the
                                 * trail as the last element */
                                free(iter_t);
                                iter_t_trail->next = NULL;
                                total->last = iter_t_trail;
                                return;
                        }
                        /* Stitch iter_t out of total */
                        if(iter_t_trail == NULL){
                                /* There was no previously valid node */
                                assert(iter_t == total->first);

                                total->first = iter_t->next;
                                free(iter_t);
                                iter_t = total->first;
                        }else{
                                /* We need to patch up the previously valid node
                                 * to point to the next *possibly* valid node */
                                iter_t_trail->next = iter_t->next;
                                free(iter_t);
                                iter_t = iter_t_trail->next;
                        }
                }
                
        }
        
}
