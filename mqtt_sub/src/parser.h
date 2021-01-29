#ifndef PARSER_H
#define PARSER_H

#include <stdlib.h>
#include <json-c/json.h>
#include "utils.h"
#include "glist.h"


int parse_msg(void *obj, struct msg **msg_ptr);
void format_out(char **out_ptr, struct msg *msg);

#endif