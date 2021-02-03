#include "conf.h"


int get_conf(struct client_data *client)
{
	int rc;
	//reads connection confifuration form mqtt_sub
	if ((rc = get_con_conf(client)) == SUB_GEN_ERR)
		goto error;
	else if (rc == SUB_FAIL)
		return SUB_GEN_ERR;
	if ((client->tops = new_glist(0)) == NULL)
		goto error;
	//sets callback
	set_free_cb_glist(client->tops, &free_top_cb);
	//reads topics' confifuration form mqtt_topics
	if (get_top_conf(client->tops) != SUB_SUC)
		goto error;
	if ((client->events = new_glist(0)) == NULL)
		goto error;
	set_free_cb_glist(client->events, &free_ev_cb);
	//reads events' confifuration form mqtt_events
	if (get_ev_conf(client->events, client->tops) != SUB_SUC)
		goto error;
	//links topics and events
	if (link_ev(client->tops, client->events) != SUB_SUC)
		goto error;
	return SUB_SUC;
	error:
		log_err("MQTT subscriber bad configurations");
		return SUB_GEN_ERR;
}

int reconf(struct client_data *client)
{
	struct glist *tops = client->tops;
	struct glist *events = client->events;
	clear_glist(events); //clears previous events
	//reads events' confifuration form mqtt_events
	if (get_ev_conf(events, tops) != SUB_SUC)
		goto error;
	//links topics and events
	if (link_ev(tops, events) != SUB_SUC)
		goto error;
	return SUB_SUC;
	error:
		log_err("MQTT subscriber bad configurations");
		return SUB_GEN_ERR;
}

int get_con_conf(struct client_data *client)
{
	struct connect_data *con = (struct connect_data*)calloc(1, \
						sizeof(struct connect_data));
	if (!con)
		return SUB_GEN_ERR;
	client->con = con;
	struct uci_context *c;
	struct uci_ptr ptr;
	int rc;
	const size_t PATH_LEN = 256;
	char path[PATH_LEN];
	void *m;

	c = uci_alloc_context();
	if ((rc = get_conf_ptr(c, &ptr, path, "mqtt_sub.mqtt_sub.enabled", \
		PATH_LEN)) != SUB_SUC || ptr.o == NULL || strcmp("1", \
							ptr.o->v.string) != 0)
		goto not_enabled;
	//getting remote port
	if ((rc = get_conf_ptr(c, &ptr, path, "mqtt_sub.mqtt_sub.remote_port", \
					PATH_LEN)) != SUB_SUC || ptr.o == NULL)
		goto fail;
	if(str_to_int(ptr.o->v.string, &con->port) != SUB_SUC)
		goto fail;
	//geting remote broker address
	if ((rc = get_conf_ptr(c, &ptr, path, "mqtt_sub.mqtt_sub.remote_addr", \
					PATH_LEN)) != SUB_SUC || ptr.o == NULL)
		goto fail;
	if ((m = malloc(strlen(ptr.o->v.string) + 1)) == NULL)
		goto fail;
	con->host = strcpy(m, ptr.o->v.string);
	//geting remote username
	if ((rc = get_conf_ptr(c, &ptr, path, "mqtt_sub.mqtt_sub.username", \
					PATH_LEN)) == SUB_SUC && ptr.o != NULL){
		if ((m = malloc(strlen(ptr.o->v.string) + 1)) == NULL)
			goto fail;
		con->username = strcpy(m, ptr.o->v.string);
	}
	//geting remote password
	if ((rc = get_conf_ptr(c, &ptr, path, "mqtt_sub.mqtt_sub.password", \
					PATH_LEN)) == SUB_SUC && ptr.o != NULL){
		if ((m = malloc(strlen(ptr.o->v.string) + 1)) == NULL)
			goto fail;
		con->password = strcpy(m, ptr.o->v.string);
	}
	//geting keep alive value
	if ((rc = get_conf_ptr(c, &ptr, path, "mqtt_sub.mqtt_sub.keep_alive", \
					PATH_LEN)) != SUB_SUC || ptr.o == NULL)
	con->keep_alive = 60; //default 60 seconds
	else if(str_to_int(ptr.o->v.string, &con->keep_alive) != SUB_SUC)
		con->keep_alive = 60; //default 60 seconds

	//geting is_clean session value
	if ((rc = get_conf_ptr(c, &ptr, path, "mqtt_sub.mqtt_sub.is_clean", \
		PATH_LEN)) == SUB_SUC && ptr.o != NULL && strcmp("1", \
							ptr.o->v.string) == 0)
		con->is_clean = true;
	if ((rc = get_conf_ptr(c, &ptr, path, "mqtt_sub.mqtt_sub.tls", \
		PATH_LEN)) == SUB_SUC && ptr.o != NULL && strcmp("1", \
							ptr.o->v.string) == 0){
			con->use_tls = true;
			if ((rc = get_tls_conf(c, &ptr, con)) != SUB_SUC)
				goto fail;
		}
	uci_free_context(c);
	return SUB_SUC;
	not_enabled:
		uci_free_context(c);
		return SUB_FAIL;
	fail:
		uci_free_context(c);
		return SUB_GEN_ERR;
}

int get_tls_conf(struct uci_context* c, struct uci_ptr *ptr, \
						struct connect_data *con)
{
	int rc;
	const size_t PATH_LEN = 256;
	char path[PATH_LEN];
	void *m;

	//geting tls_insecure value
	if ((rc = get_conf_ptr(c, ptr, path, "mqtt_sub.mqtt_sub.tls_insecure", \
		PATH_LEN)) == SUB_SUC && ptr->o != NULL && strcmp("1", \
							ptr->o->v.string) == 0)
		con->tls_insecure = true;
	//geting cafile name
	if ((rc = get_conf_ptr(c, ptr, path, "mqtt_sub.mqtt_sub.cafile", \
					PATH_LEN)) != SUB_SUC || ptr->o == NULL)
		goto fail;
	if ((con->cafile = malloc(strlen(ptr->o->v.string) + 1)) == NULL)
		goto fail;
	if ((strcpy(con->cafile, ptr->o->v.string) == NULL))
		goto fail;
	//geting certfile name
	if ((rc = get_conf_ptr(c, ptr, path, "mqtt_sub.mqtt_sub.certfile", \
					PATH_LEN)) == SUB_SUC && ptr->o != NULL){
		if ((m = malloc(strlen(ptr->o->v.string) + 1)) == NULL)
			goto fail;
		if ((con->certfile = strcpy(m, ptr->o->v.string)) == NULL)
			goto fail;
	}
	//geting keyfile name
	if ((rc = get_conf_ptr(c, ptr, path, "mqtt_sub.mqtt_sub.keyfile", \
					PATH_LEN)) == SUB_SUC && ptr->o != NULL){
		if ((m = malloc(strlen(ptr->o->v.string) + 1)) == NULL)
			goto fail;
		if ((con->keyfile = strcpy(m, ptr->o->v.string)) == NULL)
			goto fail;
	}
		
	return SUB_SUC;
	fail:
		return SUB_GEN_ERR;
}

int get_top_conf(struct glist *tops)
{
	struct topic_data *curr_topic;
	struct uci_context *c;
	struct uci_ptr ptr;
	int rc;
	const size_t PATH_LEN = 256;
	char path[PATH_LEN];
	int i = 0;
	void *m;

	c = uci_alloc_context();
	strcpy(path, "mqtt_topics");
	if ((rc = uci_lookup_ptr(c, &ptr, path, true)) != UCI_OK || ptr.p == NULL)
		goto fail;
	
	while(true) {
		sprintf(path, "mqtt_topics.@topic[%d]", i);
		if ((rc = uci_lookup_ptr(c, &ptr, path, true)) != UCI_OK || \
								ptr.s == NULL)
			goto success;
		//geting enabled value
		sprintf(path, "mqtt_topics.@topic[%d].enabled", i);
		if ((rc = uci_lookup_ptr(c, &ptr, path, true)) != UCI_OK || \
			ptr.o == NULL || strcmp(ptr.o->v.string, "0") == 0){
			i++;
			continue;
		}
		if ((curr_topic = (struct topic_data*)calloc(1, \
					sizeof(struct topic_data))) == NULL)
			goto fail;
		if ((curr_topic->events = new_glist(8)) == NULL)
			goto fail;
		
		//geting id value
		sprintf(path, "mqtt_topics.@topic[%d].id", i);
		if ((rc = uci_lookup_ptr(c, &ptr, path, true)) != UCI_OK || \
								ptr.o == NULL)
			goto fail;
		if(str_to_int(ptr.o->v.string, &curr_topic->id) != SUB_SUC)
			goto fail;
		//geting topic name value
		sprintf(path, "mqtt_topics.@topic[%d].topic", i);
		if ((rc = uci_lookup_ptr(c, &ptr, path, true)) != UCI_OK || \
								ptr.o == NULL)
			goto fail;
		if ((curr_topic->name = malloc(strlen(ptr.o->v.string) + 1)) == NULL)
			goto fail;
		strcpy(curr_topic->name, ptr.o->v.string);
		if ((curr_topic->name_path = build_name_path(ptr.o->v.string)) == NULL)
			goto fail;
		//geting allowed types value
		sprintf(path, "mqtt_topics.@topic[%d].type", i);
		if ((rc = uci_lookup_ptr(c, &ptr, path, true)) == \
					UCI_OK && ptr.o != NULL){
			if((curr_topic->fields = new_glist(0)) == NULL)
				goto fail;
			struct uci_element *e;
			uci_foreach_element(&ptr.o->v.list, e) {
				if (push_glist2(curr_topic->fields, e->name, \
						strlen(e->name) + 1) != 0)
					goto fail;
			}
		}
		if (push_glist(tops, curr_topic) != 0)
			goto fail;
		i++;
	}
	
	success:
		uci_free_context(c);
		return SUB_SUC;
	fail:
		uci_free_context(c);
		return SUB_GEN_ERR;
}

int get_ev_conf(struct glist *events, struct glist *tops)
{
	struct event_data *curr_ev;
	struct uci_context *c;
	struct uci_ptr ptr;
	int rc;
	const size_t PATH_LEN = 256;
	char path[PATH_LEN];
	int i = 0;

	c = uci_alloc_context();
	strcpy(path, "mqtt_events");
	if ((rc = uci_lookup_ptr(c, &ptr, path, true)) != UCI_OK || ptr.p == NULL)
		goto fail;
	
	while(true){
		sprintf(path, "mqtt_events.@event[%d]", i);
		if ((rc = uci_lookup_ptr(c, &ptr, path, true)) != UCI_OK || \
								ptr.s == NULL)
			goto success;
		
		//geting enabled value
		sprintf(path, "mqtt_events.@event[%d].enabled", i);
		if ((rc = uci_lookup_ptr(c, &ptr, path, true)) != UCI_OK || \
			ptr.o == NULL || strcmp("1", ptr.o->v.string) != 0){
			i++;
			continue;
		}
		//geting event t_id value
		int t_id = 0;
		sprintf(path, "mqtt_events.@event[%d].t_id", i);
		if ((rc = uci_lookup_ptr(c, &ptr, path, true)) != UCI_OK || \
			ptr.o == NULL || str_to_int(ptr.o->v.string, \
							&t_id) != SUB_SUC){
			goto fail;
		} else if (get_top_by_id(tops, t_id) != NULL){
			if ((curr_ev = (struct event_data*)calloc(1, \
					sizeof(struct event_data))) == NULL)
				goto fail;
			curr_ev->t_id = t_id;
		} else {
			i++;
			continue;
		}
		sprintf(path, "mqtt_events.@event[%d].datatype", i);
		if ((rc = uci_lookup_ptr(c, &ptr, path, true)) != UCI_OK || \
								ptr.o == NULL)
			goto fail;
		if(str_to_int(ptr.o->v.string, &curr_ev->type) != SUB_SUC)
			goto fail;
		//geting rule value
		sprintf(path, "mqtt_events.@event[%d].rule", i);
		if ((rc = uci_lookup_ptr(c, &ptr, path, true)) != UCI_OK || \
								ptr.o == NULL)
			goto fail;
		if(str_to_int(ptr.o->v.string, &curr_ev->rule) != SUB_SUC)
			goto fail;
		// geting event name value
		sprintf(path, "mqtt_events.@event[%d].field", i);
		if ((rc = uci_lookup_ptr(c, &ptr, path, true)) != UCI_OK || \
								ptr.o == NULL)
			goto fail;
		if ((curr_ev->field = (char*)malloc(strlen(ptr.o->v.string) + \
								1)) == NULL)
			goto fail;
		strcpy(curr_ev->field, ptr.o->v.string);
		//geting event target value
		sprintf(path, "mqtt_events.@event[%d].target", i);
		if ((rc = uci_lookup_ptr(c, &ptr, path, true)) != UCI_OK || \
								ptr.o == NULL)
			goto fail;
		switch (curr_ev->type)
		{
		case EV_DT_LNG:
			if (str_to_long(ptr.o->v.string, &curr_ev->target.lng) \
								!= SUB_SUC)
				goto fail;
			break;
		case EV_DT_DBL:
			if (str_to_double(ptr.o->v.string, &curr_ev->target.dbl) \
								!= SUB_SUC)
				goto fail;
			break;
		default:
			if ((curr_ev->target.str = (char*)malloc(strlen(\
						ptr.o->v.string) + 1)) == NULL)
				goto fail;
			strcpy(curr_ev->target.str, ptr.o->v.string);
			break;
		}
		// geting event interval config
		sprintf(path, "mqtt_events.@event[%d].interval", i);
		if ((rc = uci_lookup_ptr(c, &ptr, path, true)) != UCI_OK || \
			ptr.o == NULL || str_to_long(ptr.o->v.string, \
			&curr_ev->interval) != SUB_SUC || \
							curr_ev->interval < 10)
			curr_ev->interval = 10;
		// geting sender username value
		sprintf(path, "mqtt_events.@event[%d].username", i);
		if ((rc = uci_lookup_ptr(c, &ptr, path, true)) != UCI_OK || \
								ptr.o == NULL)
			goto fail;
		if ((curr_ev->username = (char*)malloc(strlen(\
						ptr.o->v.string) + 1)) == NULL)
			goto fail;
		strcpy(curr_ev->username, ptr.o->v.string);
		// geting sender email value
		sprintf(path, "mqtt_events.@event[%d].sender_email", i);
		if ((rc = uci_lookup_ptr(c, &ptr, path, true)) != UCI_OK || \
								ptr.o == NULL)
			goto fail;
		if ((curr_ev->sender_email = (char*)malloc(strlen(\
						ptr.o->v.string) + 1)) == NULL)
			goto fail;
		strcpy(curr_ev->sender_email, ptr.o->v.string);
		// geting sender email value
		sprintf(path, "mqtt_events.@event[%d].password", i);
		if ((rc = uci_lookup_ptr(c, &ptr, path, true)) != UCI_OK || \
								ptr.o == NULL)
			goto fail;
		if ((curr_ev->password = (char*)malloc(strlen(\
						ptr.o->v.string) + 1)) == NULL)
			goto fail;
		strcpy(curr_ev->password, ptr.o->v.string);
		// geting mail server ip value
		sprintf(path, "mqtt_events.@event[%d].smtp_ip", i);
		if ((rc = uci_lookup_ptr(c, &ptr, path, true)) != UCI_OK || \
								ptr.o == NULL)
			goto fail;
		if ((curr_ev->smtp_ip = (char*)malloc(strlen(\
						ptr.o->v.string) + 1)) == NULL)
			goto fail;
		strcpy(curr_ev->smtp_ip, ptr.o->v.string);
		// geting mail server ip value
		sprintf(path, "mqtt_events.@event[%d].smtp_port", i);
		if ((rc = uci_lookup_ptr(c, &ptr, path, true)) != UCI_OK || \
			ptr.o == NULL || str_to_int(ptr.o->v.string, \
						&curr_ev->smtp_port) != SUB_SUC)
			goto fail;

		// geting mail receivers
		if((curr_ev->receivers = new_glist(4)) == NULL)
			goto fail;
		sprintf(path, "mqtt_events.@event[%d].recip", i);
		if ((rc = uci_lookup_ptr(c, &ptr, path, true)) != \
							UCI_OK || ptr.o == NULL)
			goto fail;
		struct uci_element *e;
		uci_foreach_element(&ptr.o->v.list, e) {
			if (push_glist2(curr_ev->receivers, e->name, \
						strlen(e->name) + 1) != 0)
				goto fail;
		}

		if (push_glist(events, curr_ev) != 0)
			goto fail;
		i++;
	}
	success:
		uci_free_context(c);
		return SUB_SUC;
	fail:
		uci_free_context(c);
		return SUB_GEN_ERR;
}

int get_conf_ptr(struct uci_context *c, struct uci_ptr *ptr, char \
				*buff, const char *path, const int path_len)
{
	memset(buff, '\0', path_len);
	strcpy(buff, path);
	if (uci_lookup_ptr(c, ptr, buff, true) != UCI_OK)
		return SUB_GEN_ERR;
	return SUB_SUC;
}

int link_ev(struct glist *tops, struct glist *evs)
{
	struct event_data *ev;
	struct topic_data *top;
	size_t n_evs = count_glist(evs);
	size_t n_tops = count_glist(tops);
	for (size_t j = 0; j < n_tops; j++){
		top = get_glist(tops, j);
		clear_shallow_glist(top->events); //clears previous events if they exists
		for (size_t i = 0; i < n_evs; i++){
			ev = get_glist(evs, i);
			//topics related by id and t_id. e.g. primary and foriegn key, like in DB
			if (top->id == ev->t_id){
				//when event is found it's linked to topic's event glist
				if (push_glist(top->events, ev) != 0)
					return SUB_GEN_ERR;
				break;
			}
		}
	}
	return SUB_SUC;
}
