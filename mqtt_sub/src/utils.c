#include "utils.h"

//==========================================================================
// FREEING MEMORY
//==========================================================================

void free_client(struct client_data *client)
{
	struct connect_data *con;
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
