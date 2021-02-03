#ifndef PARSER_H
#define PARSER_H

#include <stdlib.h>
#include <json-c/json.h>
#include "utils.h"
#include "glist.h"

//parses json string to msg struct
int parse_msg(void *obj, struct msg **msg_ptr);
//parses msg struct to json string
void format_out(char **out_ptr, struct msg *msg);

#endif