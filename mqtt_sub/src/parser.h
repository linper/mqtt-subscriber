#ifndef PARSER_H
#define PARSER_H

#include <stdlib.h>
#include <libubox/blobmsg_json.h>
#include <libubox/blobmsg.h>
#include "utils.h"
#include "glist.h"


int parse_msg(void *obj, struct msg **msg_ptr);

#endif