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
				log_event_msg(top, e);
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
	long lnum_val;
	double dnum_val;
	bool res;
	//checking if passed enough time from last event
	if ((long)time(NULL) - e->last_event < e->interval)
		return SUB_FAIL;
	//converion checking
	if (e->type == EV_DT_LNG && str_to_long(dt->data, &lnum_val) != SUB_SUC)
		return SUB_FAIL;
	else if (e->type == EV_DT_DBL && str_to_double(dt->data, &dnum_val) != SUB_SUC)
		return SUB_FAIL;
	
	//comparing event target and received data by event rules and data type
	switch (e->rule){
	case EV_R_EQ:
		if (e->type == EV_DT_LNG)
			res = lnum_val == e->target.lng;
		else if (e->type == EV_DT_DBL)
			res = dnum_val == e->target.dbl;
		else
			res = (strcmp(dt->data, e->target.str) == 0);
		break;
	case EV_R_NEQ:
		if (e->type == EV_DT_LNG)
			res = lnum_val != e->target.lng;
		else if (e->type == EV_DT_DBL)
			res = dnum_val != e->target.dbl;
		else
			res = (strcmp(dt->data, e->target.str) != 0);
		break;
	case EV_R_MT:
		if (e->type == EV_DT_DBL)
			res = dnum_val > e->target.dbl;
		else
			res = lnum_val > e->target.lng;
		break;
	case EV_R_LT:
		if (e->type == EV_DT_DBL)
			res = dnum_val < e->target.dbl;
		else
			res = lnum_val < e->target.lng;
		break;
	case EV_R_ME:
		if (e->type == EV_DT_DBL)
			res = dnum_val >= e->target.dbl;
		else
			res = lnum_val >= e->target.lng;
		break;
	case EV_R_LE:
		if (e->type == EV_DT_DBL)
			res = dnum_val <= e->target.dbl;
		else
			res = lnum_val <= e->target.lng;
		break;
	default:
		res = false;
		break;
	}
	return res == true ? SUB_SUC : SUB_FAIL;
}

void log_event_msg(struct topic_data *top, struct event_data *e)
{
	char buff[1024];
	char *buff_pt = buff;
	int rc = 0;
	if ((rc = sprintf(buff_pt, "MQTT subscriber event occured: for topic: %s;", \
						top->name)) <= 0)
		goto error;
	buff_pt = buff_pt + rc;
	char *rec;
	for (size_t i = 0; i < count_glist(e->receivers); i++){
		rec = (char*)get_glist(e->receivers, i);
		if ((rc = sprintf(buff_pt, " email send to: %s;", rec)) <= 0)
			goto error;
		buff_pt = buff_pt + rc;
	}
		log_info(buff);
	return;
	error:
		log_info("MQTT subscriber event occured: message too long...");

}