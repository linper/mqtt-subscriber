#include "parser.h"

enum {
	SND_MSG,
	BODY_MSG,
	__MSG_MAX
};

enum {
	TYPE_DT,
	DATA_DT,
	__DT_MAX
};

static const struct blobmsg_policy msg_policy[__MSG_MAX] = {
	[SND_MSG] = { .name = "sender", .type = BLOBMSG_TYPE_STRING },
	[BODY_MSG] = { .name = "body", .type = BLOBMSG_TYPE_ARRAY },
};

static const struct blobmsg_policy dt_policy[__DT_MAX] = {
	[TYPE_DT] = { .name = "type", .type = BLOBMSG_TYPE_STRING },
	[DATA_DT] = { .name = "data", .type = BLOBMSG_TYPE_STRING },
};

int parse_msg(void *obj, struct msg **msg_ptr)
{
	struct blob_attr *mb[__MSG_MAX];
	struct blob_attr *dtb[__DT_MAX];
	struct msg *msg;
	struct blob_attr *cur;
	static struct blob_buf b;
	int rem;
	char *dt;
	struct msg_dt *mdt;

	char *msg_str = (char*)obj;
	if ((msg = (struct msg*)calloc(1, sizeof(struct msg))) == NULL)
		goto err;
	*msg_ptr = msg;
	blob_buf_init(&b, 0);
	if (!blobmsg_add_json_from_string(&b, msg_str))
		goto fail;
	blobmsg_parse(msg_policy, __MSG_MAX, mb, blob_data(b.head), blob_len(b.head));
	if (!mb[SND_MSG]) {
		goto fail;
	} else {
		char *dt = blobmsg_get_string(mb[SND_MSG]);
		if ((msg->sender = (char*)malloc(strlen(dt) + 1)) == NULL)
			goto err;
		strcpy(msg->sender, dt);
	}
	if (!mb[BODY_MSG])
		goto fail;
	if ((msg->body = new_glist(8)) == NULL)
		goto err;
	blobmsg_for_each_attr(cur, mb[BODY_MSG], rem) {
		blobmsg_parse(dt_policy, __DT_MAX, dtb, blobmsg_data(cur), \
							blobmsg_data_len(cur));
		if (!dtb[TYPE_DT] || !dtb[DATA_DT])
			goto fail;
		if ((mdt = (struct msg_dt*)calloc(1, sizeof(struct msg_dt))) == NULL)
			goto err;
		if ((dt = (char*)malloc(strlen(blobmsg_get_string(dtb[TYPE_DT])) + 1)) == NULL)
			goto err;
		strcpy(dt, blobmsg_get_string(dtb[TYPE_DT]));
		mdt->type = dt;
		if ((dt = (char*)malloc(strlen(blobmsg_get_string(dtb[DATA_DT])) + 1)) == NULL)
			goto err;
		strcpy(dt, blobmsg_get_string(dtb[DATA_DT]));
		mdt->data = dt;
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

