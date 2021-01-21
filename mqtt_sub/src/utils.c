#include "utils.h"

int test_all_t_status(struct client_data *client, int status)
{
	struct topic_data *top;
	size_t n = count_glist(client->tops);
	if (client != NULL){
		for (size_t i = 0; i < n; i++){
			top = get_glist(client->tops, i);
			if (top->status != status)
				return SUB_FAIL;
		}
		return SUB_SUC;
	}
	return SUB_GEN_ERR;
}

char *rand_string(char *str, size_t size)
{
	const char charset[] = \
	"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789,.-#'?!";
	if (size) {
		--size;
		for (int n = 0; n < size; n++) {
			int key = rand() % (int) (sizeof charset - 1);
			str[n] = charset[key];
		}
		str[size] = '\0';
	}
	return str;
}

//==========================================================================
// FREEING MEMORY
//==========================================================================

void free_client(struct client_data *client)
{
	struct connect_data *con;
	struct topic_data *top;
	struct topic_data *event;
	if (client != NULL){
		if ((con = client->con) != NULL){
			free(con->username);
			free(con->password);
			free(con->host);
			free(con->cafile);
			free(con->certfile);
			free(con->keyfile);
			free(con);
		}
		set_free_cb_glist(client->tops, &free_top_cb);
		free_glist(client->tops);
		set_free_cb_glist(client->events, &free_ev_cb);
		free_glist(client->events);
	}
	free(client);
}

void free_msg(struct msg *msg)
{
	if (msg){
		set_free_cb_glist(msg->body, &free_msg_dt_cb);
		free(msg->sender);
		free_glist(msg->body);
		free(msg);
	}
}

void free_top_cb(void *obj)
{
	if (obj){
		struct topic_data *top = (struct topic_data*)obj;
		free(top->name);
		free_glist(top->fields);
		free_shallow_glist(top->events);
		free(top);
	}
}

void free_msg_dt_cb(void *obj)
{
	if (obj){
		struct msg_dt *mdt = (struct msg_dt*)obj;
		free(mdt->type);
		free(mdt->data);
	}
}

void free_ev_cb(void *obj)
{
	if (obj){
		struct event_data *event = (struct event_data*)obj;
		free(event->field);
		free(event->target);
		free(event);

	}
}
