#include "event.h"

int handle_events(struct topic_data *top, struct glist *dt_list)
{
	int rc;
	struct glist *events = top->events;
	size_t n_ev = count_glist(events);
	size_t n_dt = count_glist(dt_list);
	struct event_data *e;
	struct msg_dt *dt;
	for (size_t i = 0; i < n_ev; i++){
		e = (struct event_data*)get_glist(events, i);
		for (size_t i = 0; i < n_dt; i++){
			dt = (struct msg_dt*)get_glist(dt_list, i);
			if (strcmp(e->field, dt->type) == 0 && \
					(rc = check_event(e, dt)) == SUB_SUC){
				printf("event: %s\n", e->field); //testing
				e->last_event = (long)time(NULL);
				if ((send_mail(e, dt, top->name)) != SUB_SUC)
					goto error;
			} else if (rc == SUB_GEN_ERR){
				goto error;
			}
		}
	}
	return SUB_SUC;
	error:
		return SUB_GEN_ERR;
}

int check_event(struct event_data *e, struct msg_dt *dt)
{
	long num_val;
	bool res;
	if ((long)time(NULL) - e->last_event < e->interval)
		return SUB_FAIL;
	if (e->type == EV_DT_LNG && str_to_long(dt->data, &num_val) != SUB_SUC)
		return SUB_FAIL;
	else if (e->type == EV_DT_DBL && str_to_double(dt->data, &num_val) != SUB_SUC)
		return SUB_FAIL;

	switch (e->rule){
	case EV_R_EQ:
		if (e->type != EV_DT_STR)
			res = num_val == e->target.lng;
		else
			res = (strcmp(dt->data, e->target.str) == 0);
		break;
	case EV_R_NEQ:
		if (e->type != EV_DT_STR)
			res = num_val != e->target.lng;
		else
			res = (strcmp(dt->data, e->target.str) != 0);
		break;
	case EV_R_MT:
		if (e->type != EV_DT_STR)
			res = num_val > e->target.lng;
		else
			res = (strcmp(dt->data, e->target.str) > 0);
		break;
	case EV_R_LT:
		if (e->type != EV_DT_STR)
			res = num_val < e->target.lng;
		else
			res = (strcmp(dt->data, e->target.str) < 0);
		break;
	case EV_R_ME:
		if (e->type != EV_DT_STR)
			res = num_val >= e->target.lng;
		else
			res = (strcmp(dt->data, e->target.str) >= 0);
		break;
	case EV_R_LE:
		if (e->type != EV_DT_STR)
			res = num_val <= e->target.lng;
		else
			res = (strcmp(dt->data, e->target.str) <= 0);
		break;
	}
	return !res;
}