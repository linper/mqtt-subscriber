#include "event.h"

int handle_events(struct glist *events, struct glist *dt_list)
{
	int rc;
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
				printf("event: %s\n", e->field);
				// if ((send_email()) != SUB_SUC)
				// 	goto err;
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
	char *s_val, p;
	long l_val;
	bool res;
	if (e->type == EV_DT_LNG && (l_val = strtol(dt->data, &p, 10)) == 0 && \
								errno == EINVAL)
		return SUB_FAIL;
	switch (e->rule){
	case EV_R_EQ:
		if (e->type == EV_DT_LNG)
			res = l_val == e->target.lng;
		else
			res = (strcmp(dt->data, e->target.str) == 0);
		break;
	case EV_R_NEQ:
		if (e->type == EV_DT_LNG)
			res = l_val != e->target.lng;
		else
			res = (strcmp(dt->data, e->target.str) != 0);
		break;
	case EV_R_MT:
		if (e->type == EV_DT_LNG)
			res = l_val > e->target.lng;
		else
			res = (strcmp(dt->data, e->target.str) > 0);
		break;
	case EV_R_LT:
		if (e->type == EV_DT_LNG)
			res = l_val < e->target.lng;
		else
			res = (strcmp(dt->data, e->target.str) < 0);
		break;
	case EV_R_ME:
		if (e->type == EV_DT_LNG)
			res = l_val >= e->target.lng;
		else
			res = (strcmp(dt->data, e->target.str) >= 0);
		break;
	case EV_R_LE:
		if (e->type == EV_DT_LNG)
			res = l_val <= e->target.lng;
		else
			res = (strcmp(dt->data, e->target.str) <= 0);
		break;
	}
	return !res;
}