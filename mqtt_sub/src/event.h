#ifndef EVENT_H
#define EVENT_H

#include <stdlib.h>
#include <errno.h>
#include "utils.h"
#include "helpers.h"

//it's basiclly main() for event checking
int handle_events(struct glist *events, struct glist *dt_list);

//checks if single event occured
int check_event(struct event_data *e, struct msg_dt *dt);

#endif