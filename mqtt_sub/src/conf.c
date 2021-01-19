#include "conf.h"

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
							PATH_LEN)) != UCI_OK)
		goto fail;
	if (ptr.o != NULL)
		con->port = atol(ptr.o->v.string);
	//geting remote broker address
	if ((rc = get_conf_ptr(c, &ptr, path, "mqtt_sub.mqtt_sub.remote_addr", \
							PATH_LEN)) != UCI_OK)
		goto fail;
	if (ptr.o != NULL)
		con->host = strcpy(malloc(strlen(ptr.o->v.string)+1), \
							ptr.o->v.string);
	//geting remote username
	if ((rc = get_conf_ptr(c, &ptr, path, "mqtt_sub.mqtt_sub.username", \
							PATH_LEN)) != UCI_OK)
		goto fail;
	if (ptr.o != NULL)
		con->username = strcpy(malloc(strlen(ptr.o->v.string)+1), \
							ptr.o->v.string);
	//geting remote password
	if ((rc = get_conf_ptr(c, &ptr, path, "mqtt_sub.mqtt_sub.password", \
							PATH_LEN)) != UCI_OK)
		goto fail;
	if (ptr.o != NULL)
		con->password = strcpy(malloc(strlen(ptr.o->v.string)+1), \
							ptr.o->v.string);
	//geting keep alive value
	if ((rc = get_conf_ptr(c, &ptr, path, "mqtt_sub.mqtt_sub.keep_alive", \
							PATH_LEN)) != UCI_OK)
		goto fail;
	if (ptr.o != NULL)
		con->keep_alive = atol(ptr.o->v.string);
	else
		con->keep_alive = 60; //default 60 seconds
	//geting is_clean session value
	if ((rc = get_conf_ptr(c, &ptr, path, "mqtt_sub.mqtt_sub.is_clean", \
							PATH_LEN)) != UCI_OK)
		goto fail;
	if (ptr.o != NULL && strcmp("0", ptr.o->v.string) == 0)
		con->is_clean = false;
	else
		con->is_clean = true;
	if ((rc = get_conf_ptr(c, &ptr, path, "mqtt_sub.mqtt_sub.tls", \
							PATH_LEN)) != UCI_OK)
		goto fail;
	if (ptr.o != NULL && strcmp("1", ptr.o->v.string) == 0){
		con->use_tls = true;
		if ((rc = get_tls_conf(c, &ptr, con)) != UCI_OK)
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
							PATH_LEN)) != UCI_OK)
		goto fail;
	if (ptr->o != NULL && strcmp("1", ptr->o->v.string) == 0)
		con->tls_insecure = true;
	//geting cafile name
	if ((rc = get_conf_ptr(c, ptr, path, "mqtt_sub.mqtt_sub.cafile", \
							PATH_LEN)) != UCI_OK)
		goto fail;
	if (ptr->o != NULL && ptr->o->v.string != NULL)
		con->cafile = strcpy((char*)malloc(strlen(ptr->o->v.string) + \
							1), ptr->o->v.string);
	//geting certfile name
	if ((rc = get_conf_ptr(c, ptr, path, "mqtt_sub.mqtt_sub.certfile", \
							PATH_LEN)) != UCI_OK)
		goto fail;
	if (ptr->o != NULL && ptr->o->v.string != NULL)
		con->certfile = strcpy((char*)malloc(strlen(ptr->o->v.string) \
							+ 1), ptr->o->v.string);
	//geting keyfile name
	if ((rc = get_conf_ptr(c, ptr, path, "mqtt_sub.mqtt_sub.keyfile", \
							PATH_LEN)) != UCI_OK)
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

	c = uci_alloc_context();
	//getting number of topics
	strcpy(path, "mqtt_sub");
	if ((rc = uci_lookup_ptr(c, &ptr, path, true)) != UCI_OK)
		goto fail;
	client->n_tops = ptr.p->n_section - 1;

	curr_topic = (struct topic_data*)calloc(client->n_tops, \
						sizeof(struct topic_data));
	client->tops = curr_topic;
	for (int i = 0; i < client->n_tops; i++){
		//geting qos value
		memset(path, '\0', PATH_LEN);
		sprintf(path, "mqtt_sub.@topic[%d].qos", i);
		if ((rc = uci_lookup_ptr(c, &ptr, path, true)) != UCI_OK)
			goto fail;
		if (ptr.o == NULL)
			goto fail;
		curr_topic->qos = atol(ptr.o->v.string);
		//geting topic name value
		memset(path, '\0', PATH_LEN);
		sprintf(path, "mqtt_sub.@topic[%d].topic", i);
		if ((rc = uci_lookup_ptr(c, &ptr, path, true)) != UCI_OK && \
								ptr.o != NULL)
			goto fail;
		if (ptr.o == NULL || ptr.o->v.string == NULL)
			goto fail;
		curr_topic->name = (char*)malloc(strlen(ptr.o->v.string)+1);
		strcpy(curr_topic->name, ptr.o->v.string);
		//geting want_retained value
		memset(path, '\0', PATH_LEN);
		sprintf(path, "mqtt_sub.@topic[%d].want_retained", i);
		if ((rc = uci_lookup_ptr(c, &ptr, path, true)) != UCI_OK)
			goto fail;
		if (ptr.o != NULL && strcmp("1", ptr.o->v.string) == 0){
			curr_topic->want_retained = true;
		} else {
			curr_topic->want_retained = false;
		}
		curr_topic->mid = i+1+100;
		*curr_topic++;
	}

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
