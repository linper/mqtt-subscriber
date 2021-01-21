#include "conf.h"

int get_conf(struct client_data *client){
	if (get_con_conf(client) != SUB_SUC)
		return SUB_GEN_ERR;
	if (get_top_conf(client) != SUB_SUC)
		return SUB_GEN_ERR;
	if (get_ev_conf(client) != SUB_SUC)
		return SUB_GEN_ERR;
	if (link_ev(client) != SUB_SUC)
		return SUB_GEN_ERR;
	return SUB_SUC;
}

int get_con_conf(struct client_data *client)
{
	struct connect_data *con = (struct connect_data*)calloc(1, \
						sizeof(struct connect_data));
	client->con = con;
	struct uci_context *c;
	struct uci_ptr ptr;
	int rc;
	const size_t PATH_LEN = 256;
	char path[PATH_LEN];

	c = uci_alloc_context();
	//getting remote port
	if ((rc = get_conf_ptr(c, &ptr, path, "mqtt_sub.mqtt_sub.remote_port", \
							PATH_LEN)) != SUB_SUC)
		goto fail;
	if (ptr.o != NULL)
		con->port = atol(ptr.o->v.string);
	//geting remote broker address
	if ((rc = get_conf_ptr(c, &ptr, path, "mqtt_sub.mqtt_sub.remote_addr", \
							PATH_LEN)) != SUB_SUC)
		goto fail;
	if (ptr.o != NULL)
		con->host = strcpy(malloc(strlen(ptr.o->v.string)+1), \
							ptr.o->v.string);
	//geting remote username
	if ((rc = get_conf_ptr(c, &ptr, path, "mqtt_sub.mqtt_sub.username", \
							PATH_LEN)) != SUB_SUC)
		goto fail;
	if (ptr.o != NULL)
		con->username = strcpy(malloc(strlen(ptr.o->v.string)+1), \
							ptr.o->v.string);
	//geting remote password
	if ((rc = get_conf_ptr(c, &ptr, path, "mqtt_sub.mqtt_sub.password", \
							PATH_LEN)) != SUB_SUC)
		goto fail;
	if (ptr.o != NULL)
		con->password = strcpy(malloc(strlen(ptr.o->v.string)+1), \
							ptr.o->v.string);
	//geting keep alive value
	if ((rc = get_conf_ptr(c, &ptr, path, "mqtt_sub.mqtt_sub.keep_alive", \
							PATH_LEN)) != SUB_SUC)
		goto fail;
	if (ptr.o != NULL)
		con->keep_alive = atol(ptr.o->v.string);
	else
		con->keep_alive = 60; //default 60 seconds
	//geting is_clean session value
	if ((rc = get_conf_ptr(c, &ptr, path, "mqtt_sub.mqtt_sub.is_clean", \
							PATH_LEN)) != SUB_SUC)
		goto fail;
	if (ptr.o != NULL && strcmp("0", ptr.o->v.string) == 0)
		con->is_clean = false;
	else
		con->is_clean = true;
	if ((rc = get_conf_ptr(c, &ptr, path, "mqtt_sub.mqtt_sub.tls", \
							PATH_LEN)) != SUB_SUC)
		goto fail;
	if (ptr.o != NULL && strcmp("1", ptr.o->v.string) == 0){
		con->use_tls = true;
		if ((rc = get_tls_conf(c, &ptr, con)) != SUB_SUC)
			goto fail;
	}

	uci_free_context(c);
	return SUB_SUC;

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

	//geting tls_insecure value
	if ((rc = get_conf_ptr(c, ptr, path, "mqtt_sub.mqtt_sub.tls_insecure", \
							PATH_LEN)) != SUB_SUC)
		goto fail;
	if (ptr->o != NULL && strcmp("1", ptr->o->v.string) == 0)
		con->tls_insecure = true;
	//geting cafile name
	if ((rc = get_conf_ptr(c, ptr, path, "mqtt_sub.mqtt_sub.cafile", \
							PATH_LEN)) != SUB_SUC)
		goto fail;
	if (ptr->o != NULL && ptr->o->v.string != NULL)
		con->cafile = strcpy((char*)malloc(strlen(ptr->o->v.string) + \
							1), ptr->o->v.string);
	//geting certfile name
	if ((rc = get_conf_ptr(c, ptr, path, "mqtt_sub.mqtt_sub.certfile", \
							PATH_LEN)) != SUB_SUC)
		goto fail;
	if (ptr->o != NULL && ptr->o->v.string != NULL)
		con->certfile = strcpy((char*)malloc(strlen(ptr->o->v.string) \
							+ 1), ptr->o->v.string);
	//geting keyfile name
	if ((rc = get_conf_ptr(c, ptr, path, "mqtt_sub.mqtt_sub.keyfile", \
							PATH_LEN)) != SUB_SUC)
		goto fail;
	if (ptr->o != NULL && ptr->o->v.string != NULL)
		con->keyfile = strcpy((char*)malloc(strlen(ptr->o->v.string) + \
							1), ptr->o->v.string);
		
	return SUB_SUC;
	fail:
		return SUB_GEN_ERR;
}

int get_top_conf(struct client_data *client)
{
	struct topic_data *curr_topic;
	struct uci_context *c;
	struct uci_ptr ptr;
	int rc;
	const size_t PATH_LEN = 256;
	char path[PATH_LEN];
	int i = 0;

	c = uci_alloc_context();
	strcpy(path, "mqtt_sub");
	if ((rc = uci_lookup_ptr(c, &ptr, path, true)) != UCI_OK)
		goto fail;
	if ((client->tops = new_glist(0)) == NULL)
		goto fail;
	set_free_cb_glist(client->tops, &free_top_cb);
	while(true) {
		sprintf(path, "mqtt_sub.@topic[%d]", i);
		if ((rc = uci_lookup_ptr(c, &ptr, path, true)) != UCI_OK || \
								ptr.s == NULL)
			goto success;

		//geting enabled value
		sprintf(path, "mqtt_sub.@topic[%d].enabled", i);
		if ((rc = uci_lookup_ptr(c, &ptr, path, true)) != UCI_OK)
			goto fail;
		if (ptr.o == NULL || atol(ptr.o->v.string) != 1){
			i++;
			continue;
		}

		curr_topic = (struct topic_data*)calloc(1, \
						sizeof(struct topic_data));
		if (push_glist(client->tops, curr_topic) != 0)
			goto fail;
		if ((curr_topic->events = new_glist(8)) == NULL)
			goto fail;
			
		//geting id value
		sprintf(path, "mqtt_sub.@topic[%d].id", i);
		if ((rc = uci_lookup_ptr(c, &ptr, path, true)) != UCI_OK || \
								ptr.o == NULL)
		if (ptr.o == NULL)
			goto fail;
		curr_topic->id = atol(ptr.o->v.string);
		//geting topic name value
		sprintf(path, "mqtt_sub.@topic[%d].topic", i);
		if ((rc = uci_lookup_ptr(c, &ptr, path, true)) != UCI_OK && \
				ptr.o == NULL || ptr.o->v.string == NULL)
			goto fail;
		curr_topic->name = (char*)malloc(strlen(ptr.o->v.string)+1);
		strcpy(curr_topic->name, ptr.o->v.string);
		//geting want_retained value
		sprintf(path, "mqtt_sub.@topic[%d].want_retained", i);
		if ((rc = uci_lookup_ptr(c, &ptr, path, true)) != UCI_OK)
			goto fail;
		if (ptr.o != NULL && strcmp("1", ptr.o->v.string) == 0)
			curr_topic->want_retained = true;
		else 
			curr_topic->want_retained = false;
		//geting constrain value
		sprintf(path, "mqtt_sub.@topic[%d].constrain", i);
		if ((rc = uci_lookup_ptr(c, &ptr, path, true)) != UCI_OK)
			goto fail;
		if (ptr.o != NULL && strcmp("1", ptr.o->v.string) == 0){
			curr_topic->constrain = true;
			if((curr_topic->fields = new_glist(0)) == NULL)
				goto fail;
			sprintf(path, "mqtt_sub.@topic[%d].type", i);
			if ((rc = uci_lookup_ptr(c, &ptr, path, true)) != UCI_OK)
				goto fail;
			struct uci_element *e;
			uci_foreach_element(&ptr.o->v.list, e) {
				char *c  = (char*)malloc(strlen(ptr.o->v.string)+1);
				strcpy(c, ptr.o->v.string);
				push_glist(curr_topic->fields, c);
			}

		} else {
			curr_topic->constrain = false;
		}
		i++;
	}
	
	success:
		uci_free_context(c);
		return SUB_SUC;
	fail:
		uci_free_context(c);
		return SUB_GEN_ERR;
}

int get_ev_conf(struct client_data *client)
{
	struct event_data *curr_ev;
	struct uci_context *c;
	struct uci_ptr ptr;
	int rc;
	const size_t PATH_LEN = 256;
	char path[PATH_LEN];
	int i = 0;

	c = uci_alloc_context();
	strcpy(path, "mqtt_sub");
	if ((rc = uci_lookup_ptr(c, &ptr, path, true)) != UCI_OK)
		goto fail;
	if ((client->events = new_glist(0)) == NULL)
		goto fail;
	set_free_cb_glist(client->events, &free_ev_cb);
	while(true){
		sprintf(path, "mqtt_sub.@event[%d]", i);
		if ((rc = uci_lookup_ptr(c, &ptr, path, true)) != UCI_OK || \
								ptr.s == NULL)
			goto success;
		
		//geting enabled value
		sprintf(path, "mqtt_sub.@event[%d].enabled", i);
		if ((rc = uci_lookup_ptr(c, &ptr, path, true)) != UCI_OK)
			goto fail;
		if (ptr.o == NULL || atol(ptr.o->v.string) != 1){
			i++;
			continue;
		}

		curr_ev = (struct event_data*)calloc(1, \
						sizeof(struct event_data));
		if (push_glist(client->events, curr_ev) != 0)
			goto fail;

		//geting event t_id value
		sprintf(path, "mqtt_sub.@event[%d].t_id", i);
		if ((rc = uci_lookup_ptr(c, &ptr, path, true)) != UCI_OK)
			goto fail;
		if (ptr.o == NULL)
			goto fail;
		curr_ev->t_id = atol(ptr.o->v.string);
		//geting t_id value
		sprintf(path, "mqtt_sub.@event[%d].datatype", i);
		if ((rc = uci_lookup_ptr(c, &ptr, path, true)) != UCI_OK)
			goto fail;
		if (ptr.o == NULL)
			goto fail;
		curr_ev->type = atol(ptr.o->v.string);
		//geting rule value
		sprintf(path, "mqtt_sub.@event[%d].rule", i);
		if ((rc = uci_lookup_ptr(c, &ptr, path, true)) != UCI_OK)
			goto fail;
		if (ptr.o == NULL)
			goto fail;
		curr_ev->rule = atol(ptr.o->v.string);
		// geting event name value
		sprintf(path, "mqtt_sub.@event[%d].field", i);
		if ((rc = uci_lookup_ptr(c, &ptr, path, true)) != UCI_OK && \
								ptr.o != NULL)
			goto fail;
		if (ptr.o == NULL || ptr.o->v.string == NULL)
			goto fail;
		curr_ev->field = (char*)malloc(strlen(ptr.o->v.string)+1);
		strcpy(curr_ev->field, ptr.o->v.string);
		//geting event target value
		sprintf(path, "mqtt_sub.@event[%d].target", i);
		if ((rc = uci_lookup_ptr(c, &ptr, path, true)) != UCI_OK && \
								ptr.o != NULL)
			goto fail;
		if (ptr.o == NULL || ptr.o->v.string == NULL)
			goto fail;
			
		if (curr_ev->type == EV_DT_LNG){
			curr_ev->target = (long*)malloc(sizeof(long));
			*(long*)(curr_ev->target) = atol(ptr.o->v.string);
		} else {
			curr_ev->target = (char*)malloc(strlen(ptr.o->v.string)+1);
			strcpy(curr_ev->target, ptr.o->v.string);
		}
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

int link_ev(struct client_data *client)
{
	struct glist *evs = client->events;
	struct glist *tops = client->tops;
	struct event_data *ev;
	struct topic_data *top;
	size_t n_evs = count_glist(evs);
	size_t n_tops = count_glist(tops);
	for (size_t i = 0; i < n_evs; i++){
		ev = get_glist(evs, i);
		for (size_t i = 0; i < n_tops; i++){
			top = get_glist(tops, i);
			if (top->id == ev->t_id){
				if (push_glist(top->events, ev) != 0)
					return SUB_GEN_ERR;
				break;
			}
		}
	}
	return SUB_SUC;
}
