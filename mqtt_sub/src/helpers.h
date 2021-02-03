#ifndef HELP_H
#define HELP_H

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include "utils.h"

//generates random string of given size 
char *rand_string(char *str, size_t size);
//gets topic from its name
struct topic_data *get_top_by_id(struct glist *tops, int id);
//gets all matching topics
//name must be exact
struct glist *get_tops(struct glist *tops, char* name);
//matches single topic
//name must be exact
//returns wether succeded
int get_sing_top(struct topic_data *top, char* name);
//creates shallow clone of msg and
//deletes unallowed data fields from clone
//return clone
struct msg *filter_msg(struct topic_data *top, struct msg *msg);
//builds name path from topic's name
struct glist *build_name_path(char *name);
//gets stat last modification time of file
long get_last_mod(const char *path);

//conversions

int str_to_int(char* str, int *res);
int str_to_long(char* str, long *res);
int str_to_double(char* str, double *res);

#endif