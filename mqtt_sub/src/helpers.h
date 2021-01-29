#ifndef HELP_H
#define HELP_H

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include "utils.h"

//tests if all topics have given status
int test_all_t_status(struct client_data *client, int status);
//generates random string of given size 
char *rand_string(char *str, size_t size);
//gets topic from its name
struct topic_data *get_top_by_id(struct glist *tops, int id);
//gets all matching topics
//name must be exact
struct glist *get_tops(struct glist *tops, char* name);
//matches single topic
//name must be exact
int get_sing_top(struct topic_data *top, char* name);
//deletes unallowed data fields from message
struct msg *filter_msg(struct topic_data *top, struct msg *msg);
struct glist *build_name_path(char *name);

int str_to_int(char* str, int *res);
int str_to_long(char* str, long *res);
int str_to_double(char* str, double *res);

#endif