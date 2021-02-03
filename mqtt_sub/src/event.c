#include "event.h"

int handle_events(struct topic_data *top, struct glist *dt_list, char *rec_top)
{
	int rc;
	struct glist *events = top->events;
	size_t n_ev = count_glist(events);
	size_t n_dt = count_glist(dt_list);
	struct event_data *e;
	struct msg_dt *dt;
	//iterating topic's events
	for (size_t i = 0; i < n_ev; i++){
		e = (struct event_data*)get_glist(events, i);
		//iterating payload
		for (size_t i = 0; i < n_dt; i++){
			dt = (struct msg_dt*)get_glist(dt_list, i);
			//searching for event target field and payload field match
			if (strcmp(e->field, dt->type) == 0 && \
					(rc = check_event(e, dt)) == SUB_SUC){
				//updates last event invokation timestamp
				e->last_event = (long)time(NULL);
				if ((send_mail(e, dt, rec_top)) != SUB_SUC)
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
	//checking if passed enough time from last event
	if ((long)time(NULL) - e->last_event < e->interval)
		return SUB_FAIL;
	//converion checking
	if (e->type == EV_DT_LNG && str_to_long(dt->data, &num_val) != SUB_SUC)
		return SUB_FAIL;
	else if (e->type == EV_DT_DBL && str_to_double(dt->data, &num_val) != SUB_SUC)
		return SUB_FAIL;
	
	//comparing event target and received data by event rules and data type
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
		res = num_val > e->target.lng;
		break;
	case EV_R_LT:
		res = num_val < e->target.lng;
		break;
	case EV_R_ME:
		res = num_val >= e->target.lng;
		break;
	case EV_R_LE:
			res = num_val <= e->target.lng;
		break;
	default:
		res = 0;
		break;
	}
	return !res;
}
