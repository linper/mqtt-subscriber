#ifndef EVENT_H
#define EVENT_H

#include <stdlib.h>
#include <time.h>
#include "utils.h"
#include "helpers.h"
#include "mail.h"

//it's basiclly main() for event checking
//dt_list is payload member of msg struct
//rec_top is topic name retreived from retreived message (not top->name)!!!
int handle_events(struct topic_data *top, struct glist *dt_list, char *rec_top);

//checks if single event occured
int check_event(struct event_data *e, struct msg_dt *dt);

#endif