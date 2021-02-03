#ifndef PARSER_H
#define PARSER_H

#include <stdlib.h>
#include <json-c/json.h>
#include "utils.h"
#include "glist.h"

/*
structure of message
{
  "sender": <sender name>, // this is opional
  "payload": [
    {
      "type": "<name of datafield-1>",
      "data": "<data of datafield-1>"
    },
    {
      "type": "<name of datafield-2>",
      "data": "<data of datafield-2>"
    },
    .
    .
    .
    {
      "type": "<name of datafield-n>",
      "data": "<data of datafield-n>"
    }
  ]
}
e.g.:
{"sender":"sender-1","payload":[{"type":"a","data":"def"},{"type":"b","data":"14.55"}]}

NOTE:
    All elements has to be of string type!!!
    Messages that won't comply with this formatting, still will be printed, but they
    won't be able to trigger events.

Example of succesfull parsing (message in output section):
{ "sender": "sender-1", "payload": [ { "type": "a", "data": "def" }, { "type": "b", "data": "14.55" } ] } 
NOTE:
    Notice whitespaces!!!
*/

//parses json string to msg struct
int parse_msg(void *obj, struct msg **msg_ptr);
//parses msg struct to json string
void format_out(char **out_ptr, struct msg *msg);

#endif