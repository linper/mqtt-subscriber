#ifndef HELP_H
#define HELP_H

#include <stdlib.h>
#include <stdio.h>

#include "utils.h"

//tests if all topics have given status
int test_all_t_status(struct client_data *client, int status);
//generates random string of given size 
char *rand_string(char *str, size_t size);
//gets topic from its name
struct topic_data *get_top_by_name(struct glist *tops, char *name);
//deletes unallowed data fields from message
void filter_msg(struct topic_data *top, struct msg *msg);

#endif