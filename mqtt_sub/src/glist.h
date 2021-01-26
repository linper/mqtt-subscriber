#ifndef GLIST_H
#define GLIST_H

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <string.h>

struct glist;

struct glist *new_glist(int cap);
void clear_glist(struct glist *lst);
void free_glist(struct glist *lst);
void free_shallow_glist(struct glist *lst);
//appends value at the end of list 
int push_glist(struct glist *lst, void *value);
//copies len bytes from value and appends it at the end of list 
int push_glist2(struct glist *lst, void *value, size_t len);
//inserts value at specified index 
int insert_glist(struct glist *lst, void *value, int index);
//copies len bytes from value and inserts it at specified index 
int insert_glist2(struct glist *lst, void *value, size_t len, int index);
//swaps stored at specified index with given value
void *pop_glist(struct glist *lst);
void *get_glist(struct glist *lst, int index);
void *remove_glist(struct glist *lst, int index);
int delete_glist(struct glist *lst, int index);
size_t count_glist(struct glist *lst);
void **get_array_glist(struct glist *lst);
void set_free_cb_glist(struct glist *lst, void (*cb)(void*));

//private
int __extend_glist(struct glist *lst);
int __shrink_glist(struct glist *lst);
int __convert_index_glist(struct glist *lst, int *index);

#endif