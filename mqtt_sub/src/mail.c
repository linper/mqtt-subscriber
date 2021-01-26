#include "mail.h"

static const char *rule_strings[] = {
	"equal to",
	"not equal to",
	"more than",
	"less than",
	"more or equal to",
	"less or equal to"
};

struct upload_status {
	int lines_read;
	struct glist *list;
};

size_t payload_source(void *ptr, size_t size, size_t nmemb, void *userp)
{
	struct upload_status *upload_ctx = (struct upload_status *)userp;
	const char *data;
	if((size == 0) || (nmemb == 0) || ((size*nmemb) < 1))
		return 0;
	data = (char*)get_glist(upload_ctx->list,upload_ctx->lines_read);
	if(data) {
		size_t len = strlen(data);
		memcpy(ptr, data, len);
		upload_ctx->lines_read++;
		return len;
	}
	return 0;
}

int build_message(struct glist *message, struct event_data *ev, \
						struct msg_dt *dt, char *topic)
{
	size_t buff_len = strlen(topic) | strlen(ev->r_email) | \
		strlen(ev->s_email) | sizeof(ev->target) | strlen(ev->field) | \
							strlen(dt->data) + 256;
	char buff[buff_len];
	//Header
	sprintf(buff, "Date: %s\r\n", "");
	if (push_glist2(message, buff, strlen(buff)+1) != 0)
		goto fail;
	sprintf(buff, "To: %s\r\n", ev->r_email);
	if (push_glist2(message, buff, strlen(buff)+1) != 0)
		goto fail;
	sprintf(buff, "From: %s\r\n", ev->s_email);
	if (push_glist2(message, buff, strlen(buff)+1) != 0)
		goto fail;
	sprintf(buff, "Message-ID: %s\r\n", "");
	if (push_glist2(message, buff, strlen(buff)+1) != 0)
		goto fail;
	sprintf(buff, "Subject: MQTT event%s\r\n", "");
	if (push_glist2(message, buff, strlen(buff)+1) != 0)
		goto fail;
	sprintf(buff, "\r\n", "");
	if (push_glist2(message, buff, strlen(buff)+1) != 0)
		goto fail;
	//Body
	sprintf(buff, "%s\r\n", "Event invoked");
	if (push_glist2(message, buff, strlen(buff)+1) != 0)
		goto fail;
	sprintf(buff, "For topic: %s\r\n", topic);
	if (push_glist2(message, buff, strlen(buff)+1) != 0)
		goto fail;
	sprintf(buff, "In field: %s\r\n", ev->field);
	if (push_glist2(message, buff, strlen(buff)+1) != 0)
		goto fail;
	sprintf(buff, "With rule: %s\r\n", rule_strings[ev->rule - 1]);
	if (push_glist2(message, buff, strlen(buff)+1) != 0)
		goto fail;
	switch (ev->type){
	case EV_DT_LNG:
		sprintf(buff, "For: %ld\r\n", ev->target.lng);
		break;
	case EV_DT_DBL:
		sprintf(buff, "For: %f\r\n", ev->target.dbl);
		break;
	case EV_DT_STR:
		sprintf(buff, "For: %s\r\n", ev->target.str);
		break;
	default:
		sprintf(buff, "For: %s\r\n", "unknown");
		break;
	}
	if (push_glist2(message, buff, strlen(buff)+1) != 0)
		goto fail;
	sprintf(buff, "At: %s\r\n", dt->data);
	if (push_glist2(message, buff, strlen(buff)+1) != 0)
		goto fail;
	if (push_glist(message, NULL) != 0)
		goto fail;
	
	return SUB_SUC;
	fail:
		return SUB_GEN_ERR;
}

int send_mail(struct event_data *event, struct msg_dt *dt, char *topic)
{
	CURL *curl;
	CURLcode res = CURLE_OK;
	struct curl_slist *recipients = NULL;
	struct upload_status upload_ctx;
	struct glist *message = new_glist(16);
	upload_ctx.list = message;
	upload_ctx.lines_read = 0;
	
	if(build_message(message, event, dt, topic) != SUB_SUC)
		goto ncurl;

	curl = curl_easy_init();
	if(curl) {
		curl_easy_setopt(curl, CURLOPT_USERNAME, event->s_email);
		curl_easy_setopt(curl, CURLOPT_PASSWORD, event->s_pwd);
		curl_easy_setopt(curl, CURLOPT_URL, event->mail_srv);
		curl_easy_setopt(curl, CURLOPT_MAIL_FROM, event->s_email);
		recipients = curl_slist_append(recipients, event->r_email);
		curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);
		curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
		curl_easy_setopt(curl, CURLOPT_READDATA, &upload_ctx);
		curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
		res = curl_easy_perform(curl);
		if(res != CURLE_OK){
			char *err = curl_easy_strerror(res);
			char buff[strlen(err) + 50];
			sprintf(buff, "MQTT subscriber failed to send e-mail: %s\n", err);
			log_err(buff);
			goto error;
		}
		curl_slist_free_all(recipients);
		curl_easy_cleanup(curl);
		free_glist(message);
		return SUB_SUC;
	}
	error:
		curl_slist_free_all(recipients);
		curl_easy_cleanup(curl);
		free_glist(message);
	ncurl:
		return SUB_GEN_ERR;

}
