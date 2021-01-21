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
		free_glist(client->tops);
		free_glist(client->events);
	}
	free(client);
}

void free_top_cb(void *obj)
{
	struct topic_data *top = (struct topic_data*)obj;
	free(top->name);
	free_glist(top->fields);
	free_shallow_glist(top->events);
	free(top);
}

void free_ev_cb(void *obj)
{
	struct event_data *event = (struct event_data*)obj;
	free(event->field);
	if (event->type == EV_DT_LNG)
		free((int*)(event->type));
	else
		free((char*)(event->type));
	free(event);
}
