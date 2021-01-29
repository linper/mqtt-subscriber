#include "helpers.h"

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

struct topic_data *get_top_by_id(struct glist *tops, int id)
{
	size_t n = count_glist(tops);
	for (size_t i = 0; i < n; i++){
		struct topic_data *top = (struct topic_data*)get_glist(tops, i);
		if (top->id == id)
			return top;
	}
	return NULL;
}

struct glist *get_tops(struct glist *tops, char* name)
{
	struct glist *matched;
	if ((matched = new_glist(4)) == NULL)
		return NULL;
	size_t n = count_glist(tops);
	struct topic_data *top;
	int rc;
	for (size_t i = 0; i < n; i++){
		top = (struct topic_data*)get_glist(tops, i);
		if ((rc = get_sing_top(top, name)) == SUB_SUC)
			push_glist(matched, top);
		else if (rc == SUB_GEN_ERR)
			log_err("MQTT subscriber internal error: 1");
	}
	return matched;
}

int get_sing_top(struct topic_data *top, char* name)
{
	struct glist *npath = build_name_path(name);
	if (!npath)
		return SUB_GEN_ERR;
	size_t tn = count_glist(top->name_path);
	size_t nn = count_glist(npath);
	if(nn > tn)
		goto fail;
	char *ttoken;
	char *ntoken;
	for (size_t i = 0; i < nn; i++){
		ttoken = (char*)get_glist(top->name_path, i);
		ntoken = (char*)get_glist(npath, i);
		if (strcmp(ttoken, "#") == 0){
			goto success;
		} else if (strcmp(ttoken, "+") == 0){
			continue;
		} else if (strcmp(ntoken, ttoken) != 0){
			goto fail;
		}
	}
	success:
		free_glist(npath);
		return SUB_SUC;
	fail:
		free_glist(npath);
		return SUB_FAIL;
}

struct glist *build_name_path(char *name)
{
	char buff[strlen(name) + 1];
	strcpy(buff, name);
	struct glist *path = new_glist(8);
	if (!path)
		return NULL;
	for (char *p = strtok(buff, "/"); p != NULL; p = strtok(NULL, "/")){
		if (p != "" && push_glist2(path, p, strlen(p)+1) != 0){
			free_glist(path);
			return NULL;
		}
	}
	return path;
}

struct msg *filter_msg(struct topic_data *top, struct msg *msg)
{
	char *field;
	char *type;
	bool found;
	int nf = (int)count_glist(top->fields);
	int nt = (int)count_glist(msg->payload);
	struct msg *clone_msg = (struct msg*)malloc(sizeof(struct msg));
	clone_msg->sender = msg->sender;
	if ((clone_msg->payload = clone_glist(msg->payload)) == NULL){
		free_glist(clone_msg);
		return NULL;
	}
	if (nf == 0)
		return clone_msg;
	for (size_t i = nt - 1; i > 0; i--){
		type = ((struct msg_dt*)get_glist(msg->payload, i))->type;
		found = false;
		for (int j = 0; j < nf; j++){
			field = (char*)get_glist(top->fields, j);
			if (strcmp(type, field) == 0){
				found = true;
				break;
			}
		}
		if (!found)
			forget_glist(clone_msg->payload, i);
	}
	return clone_msg;
}

int str_to_int(char* str, int *res)
{
	char *p;
	errno = 0;
	long l = strtol(str, &p, 10);
	if ((errno == ERANGE && (l == LONG_MAX || l == LONG_MIN)) || \
						(errno != 0 && l == 0))
		return SUB_GEN_ERR;
	*res = (int)l;
	return SUB_SUC;
}

int str_to_long(char* str, long *res)
{
	char *p;
	errno = 0;
	*res = strtol(str, &p, 10);
	if ((errno == ERANGE && (*res == LONG_MAX || *res == LONG_MIN)) || \
						(errno != 0 && *res == 0))
		return SUB_GEN_ERR;
	return SUB_SUC;
}

int str_to_double(char* str, double *res)
{
	char *p;
	errno = 0;
	*res = strtod(str, &p);
	if ((errno == ERANGE && (*res == HUGE_VALF || *res == HUGE_VALL)) || \
						(errno != 0 && *res == 0))
		return SUB_GEN_ERR;
	return SUB_SUC;
}
