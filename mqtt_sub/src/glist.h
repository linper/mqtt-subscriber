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
int push_glist(struct glist *lst, void *value);
int put_glist(struct glist *lst, void *value, size_t index);
void *pop_glist(struct glist *lst);
void *get_glist(struct glist *lst, size_t index);
void *remove_glist(struct glist *lst, size_t index);
int delete_glist(struct glist *lst, size_t index);
size_t count_glist(struct glist *lst);
void **get_array_glist(struct glist *lst);
void set_free_cb_glist(struct glist *lst, void (*cb)(void*));

int __extend_glist(struct glist *lst);
int __shrink_glist(struct glist *lst);

#endif