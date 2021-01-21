
#include "mqtt_sub.h"

enum inter_status{
	INT_ABSENT,
	INT_PRE_DISC,
	INT_WAIT_UNSUB,
	INT_READY_DISC,
	INT_DONE_DISC,
};

volatile int interupt = 0;


void sigHandler(int signo){
	interupt = INT_PRE_DISC;
}

//==========================================================================
// CALLBACKS
//==========================================================================

void on_message(struct mosquitto *mosq, void *obj, \
					const struct mosquitto_message *message)
{
	int rc;
	struct msg *msg;
	if ((rc = parse_msg(message->payload, &msg)) != SUB_SUC){
		if (rc == SUB_GEN_ERR)
			interupt = INT_PRE_DISC;
		else
			return;
	}
	size_t n = count_glist(msg->body);
	struct client_data *client = (struct client_data*)obj;
	struct msg_dt *mdt;
	for (size_t i = 0; i < n; i++){
		mdt = get_glist(msg->body, i);
		char buff[strlen(mdt->type) + strlen(mdt->data) + 3];
		sprintf(buff, "%s: %s", mdt->type, mdt->data);
		if (log_db(client->db, &(client->n_msg), message->topic, buff) \
								!= SUB_SUC){
			interupt = INT_PRE_DISC;
			return;
		}
	}
}

void on_connect(struct mosquitto *mosq, void *obj, int rc)
{
	char buff[60];
	struct client_data *client = (struct client_data*)obj;
	switch (rc){
	case 1:
		strcpy(buff, "connection refused (unacceptable protocol version)");
		break;
	case 2:
		strcpy(buff, "connection refused (identifier rejected)");
		break;
	case 3:
		strcpy(buff, "connection refused (broker unavailable)");
		break;
	}
	if (rc != 0){
		log_err(buff);
		interupt = INT_PRE_DISC;
	}
}

void on_disconnect(struct mosquitto *mosq, void *obj, int rc)
{
	struct client_data *client = (struct client_data*)obj;
	if (rc != 0)
		log_err("MQTT subscriber disconnected unexpectedly\n");
	interupt = INT_DONE_DISC;
}

void on_subscribe(struct mosquitto *mosq, void *obj, int mid, \
					int qos_count, const int *granted_qos)
{
	struct client_data *client = (struct client_data*)obj;
	struct topic_data *top;
	size_t n = count_glist(client->tops);
	for (size_t i = 0; i < n; i++){
		top = get_glist(client->tops, i);
		if (top->mid == mid){
			top->status = T_SUB;
			break;
		}
	}
}

void on_unsubscribe(struct mosquitto *mosq, void *obj, int mid)
{
	struct client_data *client = (struct client_data*)obj;
	struct topic_data *top;
	size_t n = count_glist(client->tops);
	for (size_t i = 0; i < n; i++){
		top = get_glist(client->tops, i);
		if (top->mid == mid){
			top->status = T_UNSUB;
			break;
		}
	}
}

//==========================================================================
// CONNENTION HANDLING
//==========================================================================

int init_client(struct mosquitto **mosq_ptr, struct client_data *client)
{
	int rc = 0;
	struct mosquitto *mosq;
	char *id = NULL;
	struct connect_data *con = client->con;

	if (!con->is_clean)
		id = rand_string(id, 31);
	if ((mosq = mosquitto_new(id, con->is_clean, client)) == NULL)
		return SUB_GEN_ERR;

	*mosq_ptr = mosq;
	if (con->username != NULL && con->password != NULL)
		if ((rc = mosquitto_username_pw_set(mosq, con->username,\
		con->password)) != EXIT_SUCCESS)
			return SUB_GEN_ERR;
	
	if (con->use_tls){
		if(con->tls_insecure && (rc = mosquitto_tls_insecure_set(mosq, \
						true)) != MOSQ_ERR_SUCCESS)
			return SUB_GEN_ERR;
		if ((rc = mosquitto_tls_set(mosq, con->cafile, NULL, \
		con->certfile, con->keyfile, NULL)) != EXIT_SUCCESS){
			log_err("MQTT subscriber TLS error");
			return SUB_GEN_ERR;
		}
	}

	return SUB_SUC;
}

int connect_to_broker(struct mosquitto *mosq, struct client_data *client)
{
	int rc;
	mosquitto_connect_callback_set(mosq, &on_connect);
	mosquitto_disconnect_callback_set(mosq, &on_disconnect);
	if ((rc = mosquitto_connect(mosq, client->con->host, \
	client->con->port, client->con->keep_alive)) != MOSQ_ERR_SUCCESS){
		char buff[50];
		sprintf(buff, "MQTT subscriber connection failed: %d", rc);
		log_err(buff);
		return SUB_GEN_ERR;
	} 
	return SUB_SUC;
}

int subscribe_all(struct mosquitto *mosq, struct client_data *client)
{
	int rc;
	size_t n = count_glist(client->tops);
	if (n){
		struct topic_data *top;
		mosquitto_message_callback_set(mosq, &on_message);
		mosquitto_subscribe_callback_set(mosq, &on_subscribe);
		mosquitto_unsubscribe_callback_set(mosq, &on_unsubscribe);
		for (size_t i = 0; i < n; i++){
			top = get_glist(client->tops, i);
			if ((rc = mosquitto_subscribe(mosq, &(top->mid), \
			top->name, top->qos)) != SUB_SUC){
				char buff[strlen(top->name)+100];
				sprintf(buff, "MQTT subscriber failed to subscribe to topic: \
				%s, %d", top->name, rc);
				log_err(buff);
			}
		}
	}

	return SUB_SUC;
}

int pre_disconnect(struct mosquitto *mosq, struct client_data *client)
{
	int rc;
	struct topic_data *top;
	size_t n = count_glist(client->tops);
	for (size_t i = 0; i < n; i++){
		top = get_glist(client->tops, i);
		if ((rc = mosquitto_unsubscribe(mosq, &(top->mid), \
							top->name)) != SUB_SUC){
			char buff[strlen(top->name)+100];
			sprintf(buff, "MQTT subscriber failed to unsubscribe from topic: \
			%s, %d", top->name, rc);
			log_err(buff);
			return SUB_GEN_ERR;
		}
	}
	interupt = INT_WAIT_UNSUB;
	return SUB_SUC;
}

int disconnect_from_broker(struct mosquitto *mosq)
{
	int rc;
	if ((rc = mosquitto_disconnect(mosq)) != SUB_SUC){
		char buff[50];
		sprintf(buff, "MQTT subscriber failed to disconnect: %d", rc);
		log_err(buff);
		return SUB_GEN_ERR;
	}
	return SUB_SUC;
}

void main_loop(struct mosquitto *mosq, struct client_data *client)
{
	int rc;
	while(true){
		switch (interupt){
		case INT_PRE_DISC: //unsubscribes topics and disconnenct client
			interupt = INT_ABSENT;
			if ((rc = pre_disconnect(mosq, client)) != SUB_SUC)
				interupt = INT_READY_DISC;
			break;
		case INT_WAIT_UNSUB: //waits while client unsubscribes all topics
			if ((rc = test_all_t_status(client, T_UNSUB)) <= SUB_SUC){
				interupt = INT_READY_DISC;
			} else if ((rc = mosquitto_loop(mosq, -1, 1)) != SUB_SUC) {
				char buff[50];
				sprintf(buff, "MQTT subscriber failed to loop: %d", rc);
				log_err(buff);
				interupt = INT_READY_DISC;
			}
			break;
		case INT_READY_DISC: //disconnenct client
			interupt = INT_ABSENT;
			if ((rc = disconnect_from_broker(mosq)) != SUB_SUC)
				interupt = INT_DONE_DISC;
			break;
		case INT_DONE_DISC: //goes to exit freeing part
			goto exit;
			break;
		default:
			if ((rc = mosquitto_loop(mosq, -1, 1)) != SUB_SUC){
				char buff[50];
				sprintf(buff, "MQTT subscriber failed to loop: %d", rc);
				log_err(buff);
				interupt = INT_PRE_DISC;
			}
			break;
		}
	}
	exit:
		return;
}

//==========================================================================
// MAIN
//==========================================================================

int main(int argc, char *argv[])
{
	int rc, erc;
	time_t t;
	struct mosquitto *mosq;
	struct client_data *client;
	sqlite3 *db;

	srand((unsigned) time(&t));
	signal(SIGINT, sigHandler);
	signal(SIGTERM, sigHandler);
	openlog(NULL, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
	
	if ((rc = init_db(&db, "/usr/share/mqtt_sub/mqtt_sub.db")) != SUB_SUC)
		goto db_exit;
	if ((client = (struct client_data*)calloc(1, \
					sizeof(struct client_data))) == NULL)
		goto nmosq_exit;
	if ((rc = get_conf(client)) != SUB_SUC)
		goto nmosq_exit;

	client->db = db; //need to fix this
	
	mosquitto_lib_init();
	
	if ((rc = init_client(&mosq, client)) != SUB_SUC)
		goto ncon_exit;
	if ((rc = connect_to_broker(mosq, client)) != SUB_SUC)
		goto ncon_exit;
	if ((rc = subscribe_all(mosq, client)) != SUB_SUC)
		interupt = INT_PRE_DISC;
	
	main_loop(mosq, client);

	ncon_exit: //when client was never connected or already disconnected
		mosquitto_destroy(mosq);
		mosquitto_lib_cleanup();
	nmosq_exit: //when mosq lib was never initialized or already freed
		free_client(client);
	db_exit:
		sqlite3_close(db);
		closelog();
		return rc == 0 ? 0 : -1;
}