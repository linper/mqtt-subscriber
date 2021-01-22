#include "parser.h"

int parse_msg(void *obj, struct msg **msg_ptr)
{
	struct json_object *json;
	struct json_object *child;
	struct array_list *body;
	struct msg *msg;
	char *dt;
	struct msg_dt *mdt;
	int rc, len;

	char *msg_str = (char*)obj;
	if ((msg = (struct msg*)calloc(1, sizeof(struct msg))) == NULL)
		goto err;
	*msg_ptr = msg;
	if ((json = json_tokener_parse(msg_str)) == NULL)
		goto fail;
	if ((rc = json_object_object_get_ex(json, "sender", &child)) != 1)
		goto fail;
	dt = json_object_get_string(child);
	if ((msg->sender = (char*)malloc(strlen(dt) + 1)) == NULL)
		goto err;
	strcpy(msg->sender, dt);

	if ((msg->body = new_glist(8)) == NULL)
		goto err;

	if ((rc = json_object_object_get_ex(json, "body", &child)) == NULL)
		goto fail;
	
	if ((body = json_object_get_array(child)) == NULL)
		goto fail;
	for (int i = 0; i < array_list_length(body); i++){
		if ((json = (struct json_object*)array_list_get_idx(body, i)) == NULL)
			goto fail;

		if ((mdt = (struct msg_dt*)calloc(1, sizeof(struct msg_dt))) == NULL)
			goto err;
		if ((rc = json_object_object_get_ex(json, "type", &child)) != 1)
			goto fail;
		if ((len = json_object_get_string_len(child)) == 0)
			goto fail;
		if ((mdt->type = (char*)malloc(len + 1)) == NULL)
			goto err;
		strcpy(mdt->type, json_object_get_string(child));

		if ((rc = json_object_object_get_ex(json, "data", &child)) != 1)
			goto fail;
		if ((len = json_object_get_string_len(child)) == 0)
			goto fail;
		if ((mdt->data = (char*)malloc(len + 1)) == NULL)
			goto err;
		strcpy(mdt->data, json_object_get_string(child));
		

		if (push_glist(msg->body, mdt) != 0)
			goto err;
	}
	suc:
		return SUB_SUC;
	fail:
		free_msg(msg);
		return SUB_FAIL;
	err:
		log_err("MQTT subscriber failed");
		free_msg(msg);
		return SUB_GEN_ERR;
}

