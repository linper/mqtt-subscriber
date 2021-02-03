#include "parser.h"

int parse_msg(void *obj, struct msg **msg_ptr)
{
	struct json_object *json;
	struct json_object *child;
	struct array_list *payload;
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
	if ((rc = json_object_object_get_ex(json, "sender", &child)) == 1){
		dt = json_object_get_string(child);
		if ((msg->sender = (char*)malloc(strlen(dt) + 1)) == NULL)
			goto err;
		strcpy(msg->sender, dt);
	}
	if ((msg->payload = new_glist(8)) == NULL)
		goto err;

	if ((rc = json_object_object_get_ex(json, "payload", &child)) != 1)
		goto fail;
	
	if ((payload = json_object_get_array(child)) == NULL)
		goto fail;
	//parses payload members
	for (int i = 0; i < array_list_length(payload); i++){
		if ((json = (struct json_object*)array_list_get_idx(payload, i)) == NULL)
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

		if (push_glist(msg->payload, mdt) != 0)
			goto err;
	}
	return SUB_SUC;
	fail: //probaly bad message format
		free_msg(msg);
		return SUB_FAIL;
	err: //out of memory
		log_err("MQTT subscriber failed");
		free_msg(msg);
		return SUB_GEN_ERR;
}

void format_out(char **out_ptr, struct msg *msg)
{
	char *out;
	out = *out_ptr;
	struct json_object *base_ob;
	struct json_object *ob;
	struct json_object *string;
	struct json_object *array;
	struct msg_dt *dt;
	base_ob = json_object_new_object();
	array = json_object_new_array();
	if (msg->sender){
		string = json_object_new_string(msg->sender);
		json_object_object_add(base_ob, "sender", string);
	}
	json_object_object_add(base_ob, "payload", array);
	size_t n = count_glist(msg->payload);
	//parses payload members
	for (size_t i = 0; i < n; i++){
		dt = (struct msg_dt*)get_glist(msg->payload, i);
		ob = json_object_new_object();
		string = json_object_new_string(dt->type);
		json_object_object_add(ob, "type", string);
		string = json_object_new_string(dt->data);
		json_object_object_add(ob, "data", string);
		json_object_array_add(array, ob);
	}
	strcpy(out, json_object_to_json_string(base_ob));
}
