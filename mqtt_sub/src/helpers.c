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

struct topic_data *get_top_by_name(struct glist *tops, char *name)
{
	size_t n = count_glist(tops);
	for (size_t i = 0; i < n; i++){
		struct topic_data *top = (struct topic_data*)get_glist(tops, i);
		if (strcmp(top->name, name) == 0)
			return top;
	}
	return NULL;
}

void filter_msg(struct topic_data *top, struct msg *msg)
{
	if (top->constrain){
		char *field;
		char *type;
		bool found;
		int nf = (int)count_glist(top->fields);
		int nt = (int)count_glist(msg->body);
		for (int i = 0; i < nt; i++){
			type = ((struct msg_dt*)get_glist(msg->body, i))->type;
			found = false;
			for (int j = 0; j < nf; j++){
				field = (char*)get_glist(top->fields, j);
				if (strcmp(type, field) == 0){
					found = true;
					break;
				}
			}
			if (!found){
				delete_glist(msg->body, i);
				i--;
				nt--;
			}
		}
	}
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

