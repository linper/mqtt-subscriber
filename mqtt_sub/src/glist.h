#ifndef GLIST_H
#define GLIST_H

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <string.h>

//generic dynamic list
struct glist;

struct glist *new_glist(int cap);
//creates shallow clone
struct glist *clone_glist(struct glist *lst);
void clear_glist(struct glist *lst);
//just sets count to 0
void clear_shallow_glist(struct glist *lst);
void free_glist(struct glist *lst);
void free_shallow_glist(struct glist *lst);
//appends value at the end of list 
int push_glist(struct glist *lst, void *value);
//copies len bytes from value and appends it at the end of list 
int push_glist2(struct glist *lst, void *value, size_t len);
//inserts value at specified index 
void *get_glist(struct glist *lst, int index);
//same deleting element without freeing it
int forget_glist(struct glist *lst, int index);
size_t count_glist(struct glist *lst);
//sets callback for every element for free_glist
//cb(void *<element to free>)
void set_free_cb_glist(struct glist *lst, void (*cb)(void*));

//private

//expands inner array
int __extend_glist(struct glist *lst);
//shrinks inner array
int __shrink_glist(struct glist *lst);
//allows negative indexing
int __convert_index_glist(struct glist *lst, int *index);

#endif