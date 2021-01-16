#include "mqtt_sub.h"

volatile int interupt = 0;

enum ret_codes{
	SUB_GEN_ERR = -1,
	SUB_SUC = 0,
	SUB_FAIL = 1,
};

enum t_status{
	T_UNSUB,
	T_SUB,
};

enum inter_status{
	INT_ABSENT,
	INT_PRE_DISC,
	INT_WAIT_UNSUB,
	INT_READY_DISC,
	INT_DONE_DISC,
};

struct topic_data{
	char *name;
	int qos;
	bool want_retained;
	enum t_status status;
	int mid;
};

struct connect_data{
	char *host;
	int port;
	char *username;
	char *password;
	struct libmosquitto_will *will;
	int keep_alive;
	bool is_clean;
	bool use_tls;
	bool tls_insecure;
	char *cafile;
	char *keyfile;
	char *certfile;
	bool has_will;
};

struct client_data{
	struct connect_data *con;
	struct topic_data *tops;
	int n_tops;
	sqlite3 *db;
	int n_msg;
};

void sigHandler(int signo){
	interupt = INT_PRE_DISC;
}

//==========================================================================
// DATABASE
//==========================================================================

int init_db(sqlite3 **db, const char *db_name)
{
	int rc;
	char *err;

	if ((rc = sqlite3_open(db_name, db)) != SQLITE_OK)
		return SUB_GEN_ERR;
	char *sql = "DROP TABLE IF EXISTS logs;"
		"CREATE TABLE Logs(id INT, timestamp TEXT, type TEXT, topic \
							TEXT, message TEXT);";

	if ((rc = sqlite3_exec(*db, sql, 0, 0, &err)) != SQLITE_OK){
		printf("error: %s\n", err);
		sqlite3_free(err);
		return SUB_GEN_ERR;
	}
	return SUB_SUC;
}

int log_db(sqlite3 *db, int *n_msg, char *type, char *topic, char *message)
{
	char buff[100 + strlen(type) + strlen(topic) + strlen(message)];
	int rc;
	char *err;

	if ((rc = sprintf(buff, "INSERT INTO Logs VALUES(%d, datetime('now'), \
			'%s', '%s', '%s');", *n_msg, type, topic, message)) < 0)
		return SUB_GEN_ERR;

	if ((rc = sqlite3_exec(db, buff, 0, 0, &err)) != SQLITE_OK){
		printf("error: %s\n", err);
		sqlite3_free(err);
		return SUB_GEN_ERR;
	}
	(*n_msg)++;
	return SUB_SUC;
}

//==========================================================================
// READING CONFIGURATIONS AND INIT
//==========================================================================

int get_con_conf(struct client_data *client)
{
	struct connect_data *con = (struct connect_data*)calloc(1, sizeof(struct connect_data));
	client->con = con;
	struct uci_context *c;
	struct uci_ptr ptr;
	int rc;
	const size_t PATH_LEN = 256;
	char path[PATH_LEN];

	c = uci_alloc_context();
	//getting remote port
	if ((rc = get_conf_ptr(c, &ptr, path, "mqtt_sub.mqtt_sub.remote_port", \
							PATH_LEN)) != UCI_OK)
		goto fail;
	if (ptr.o != NULL)
		con->port = atoi(ptr.o->v.string);
	//geting remote broker address
	if ((rc = get_conf_ptr(c, &ptr, path, "mqtt_sub.mqtt_sub.remote_addr", \
							PATH_LEN)) != UCI_OK)
		goto fail;
	if (ptr.o != NULL)
		con->host = strcpy(malloc(strlen(ptr.o->v.string)+1), \
							ptr.o->v.string);
	//geting remote username
	if ((rc = get_conf_ptr(c, &ptr, path, "mqtt_sub.mqtt_sub.username", \
							PATH_LEN)) != UCI_OK)
		goto fail;
	if (ptr.o != NULL)
		con->username = strcpy(malloc(strlen(ptr.o->v.string)+1), \
							ptr.o->v.string);
	//geting remote password
	if ((rc = get_conf_ptr(c, &ptr, path, "mqtt_sub.mqtt_sub.password", \
							PATH_LEN)) != UCI_OK)
		goto fail;
	if (ptr.o != NULL)
		con->password = strcpy(malloc(strlen(ptr.o->v.string)+1), \
							ptr.o->v.string);
	//geting keep alive value
	if ((rc = get_conf_ptr(c, &ptr, path, "mqtt_sub.mqtt_sub.keep_alive", \
							PATH_LEN)) != UCI_OK)
		goto fail;
	if (ptr.o != NULL)
		con->keep_alive = atoi(ptr.o->v.string);
	else
		con->keep_alive = 60; //default 60 seconds
	//geting is_clean session value
	if ((rc = get_conf_ptr(c, &ptr, path, "mqtt_sub.mqtt_sub.is_clean", \
							PATH_LEN)) != UCI_OK)
		goto fail;
	if (ptr.o != NULL && strcmp("0", ptr.o->v.string) == 0)
		con->is_clean = false;
	else
		con->is_clean = true;
	//geting has_will value
	if ((rc = get_conf_ptr(c, &ptr, path, "mqtt_sub.mqtt_sub.has_will", \
							PATH_LEN)) != UCI_OK)
		goto fail;
	if (ptr.o != NULL && strcmp("1", ptr.o->v.string) == 0){
		con->has_will = true;
		struct libmosquitto_will *will;
		if ((will = (struct libmosquitto_will*)calloc(1, \
		sizeof(struct libmosquitto_will))) == NULL)
			goto fail;
		con->will = will;
		if ((rc = get_will_conf(c, &ptr, will)) != UCI_OK)
			goto fail;
	}
	//geting use_tls value
	if ((rc = get_conf_ptr(c, &ptr, path, "mqtt_sub.mqtt_sub.tls", \
							PATH_LEN)) != UCI_OK)
		goto fail;
	if (ptr.o != NULL && strcmp("1", ptr.o->v.string) == 0){
		con->use_tls = true;
		if ((rc = get_tls_conf(c, &ptr, con)) != UCI_OK)
			goto fail;
	}

	uci_free_context(c);
	return SUB_SUC;

	fail:
		uci_free_context(c);
		return SUB_GEN_ERR;
}

int get_will_conf(struct uci_context* c, struct uci_ptr *ptr, \
					struct libmosquitto_will *will)
{
	int rc;
	const size_t PATH_LEN = 256;
	char path[PATH_LEN];

	//geting last will topic name
	if ((rc = get_conf_ptr(c, ptr, path, "mqtt_sub.mqtt_sub.will_topic", \
							PATH_LEN)) != UCI_OK)
		goto fail;
	if (ptr->o == NULL || ptr->o->v.string == NULL)
		goto fail;
	will->topic = strcpy((char*)malloc(strlen(ptr->o->v.string)+1), \
							ptr->o->v.string);
	//geting last will topic message
	if ((rc = get_conf_ptr(c, ptr, path, "mqtt_sub.mqtt_sub.will_message", \
							PATH_LEN)) != UCI_OK)
		goto fail;
	if (ptr->o != NULL){
		char *message = strcpy(malloc(strlen(ptr->o->v.string)+1), \
							ptr->o->v.string);
		will->payload = (void*)message;
		will->payloadlen = strlen(message);
	}
	//geting will qos value
	if ((rc = get_conf_ptr(c, ptr, path, "mqtt_sub.mqtt_sub.will_qos", \
							PATH_LEN)) != UCI_OK)
		goto fail;
	if (ptr->o == NULL)
		goto fail;
	will->qos = atoi(ptr->o->v.string); //on failure qos becomes 0
	//geting will retain value
	if ((rc = get_conf_ptr(c, ptr, path, "mqtt_sub.mqtt_sub.will_retain", \
							PATH_LEN)) != UCI_OK)
		goto fail;
	if (ptr->o != NULL && strcmp("1", ptr->o->v.string) == 0)
		will->retain = true;
		
	return SUB_SUC;
	fail:
		return SUB_GEN_ERR;
}


int get_tls_conf(struct uci_context* c, struct uci_ptr *ptr, \
						struct connect_data *con)
{
	int rc;
	// bool device_files = false;
	const size_t PATH_LEN = 256;
	char path[PATH_LEN];
	// struct libmosquitto_tls *tls = con->tls;

	//geting tls_insecure value
	if ((rc = get_conf_ptr(c, ptr, path, "mqtt_sub.mqtt_sub.tls_insecure", \
							PATH_LEN)) != UCI_OK)
		goto fail;
	if (ptr->o != NULL && strcmp("1", ptr->o->v.string) == 0)
		con->tls_insecure = true;
	// //geting _device_files value
	// if ((rc = get_conf_ptr(c, ptr, path, \
	// 		"mqtt_sub.mqtt_sub._device_files", PATH_LEN)) != UCI_OK)
	// 	goto fail;
	// if (ptr->o != NULL && strcmp("1", ptr->o->v.string) == 0)
	// 	device_files = true;
		
	//geting cafile name
	if ((rc = get_conf_ptr(c, ptr, path, "mqtt_sub.mqtt_sub.cafile", \
							PATH_LEN)) != UCI_OK)
		goto fail;
	if (ptr->o != NULL && ptr->o->v.string != NULL)
		con->cafile = strcpy((char*)malloc(strlen(ptr->o->v.string) + \
							1), ptr->o->v.string);
	//geting certfile name
	if ((rc = get_conf_ptr(c, ptr, path, "mqtt_sub.mqtt_sub.certfile", \
							PATH_LEN)) != UCI_OK)
		goto fail;
	if (ptr->o != NULL && ptr->o->v.string != NULL)
		con->certfile = strcpy((char*)malloc(strlen(ptr->o->v.string) \
							+ 1), ptr->o->v.string);
	//geting keyfile name
	if ((rc = get_conf_ptr(c, ptr, path, "mqtt_sub.mqtt_sub.keyfile", \
							PATH_LEN)) != UCI_OK)
		goto fail;
	if (ptr->o != NULL && ptr->o->v.string != NULL)
		con->keyfile = strcpy((char*)malloc(strlen(ptr->o->v.string) + \
							1), ptr->o->v.string);
		
	return SUB_SUC;
	fail:
		return SUB_GEN_ERR;
}

int get_top_conf(struct client_data *client)
{
	struct topic_data *curr_topic;
	struct uci_context *c;
	struct uci_ptr ptr;
	int rc;
	const size_t PATH_LEN = 256;
	char path[PATH_LEN];

	c = uci_alloc_context();
	//getting number of topics
	strcpy(path, "mqtt_sub");
	if ((rc = uci_lookup_ptr(c, &ptr, path, true)) != UCI_OK) goto fail;
	client->n_tops = ptr.p->n_section - 1;

	curr_topic = (struct topic_data*)calloc(client->n_tops, \
						sizeof(struct topic_data));
	client->tops = curr_topic;
	for (int i = 0; i < client->n_tops; i++){
		//geting qos value
		memset(path, '\0', PATH_LEN);
		sprintf(path, "mqtt_sub.@topic[%d].qos", i);
		if ((rc = uci_lookup_ptr(c, &ptr, path, true)) != UCI_OK)
			goto fail;
		if (ptr.o == NULL)
			goto fail;
		curr_topic->qos = atoi(ptr.o->v.string);
		//geting topic name value
		memset(path, '\0', PATH_LEN);
		sprintf(path, "mqtt_sub.@topic[%d].topic", i);
		if ((rc = uci_lookup_ptr(c, &ptr, path, true)) != UCI_OK && \
								ptr.o != NULL)
			goto fail;
		if (ptr.o == NULL || ptr.o->v.string == NULL)
			goto fail;
		curr_topic->name = (char*)malloc(strlen(ptr.o->v.string)+1);
		strcpy(curr_topic->name, ptr.o->v.string);
		//geting want_retained value
		memset(path, '\0', PATH_LEN);
		sprintf(path, "mqtt_sub.@topic[%d].want_retained", i);
		if ((rc = uci_lookup_ptr(c, &ptr, path, true)) != UCI_OK)
			goto fail;
		if (ptr.o != NULL && strcmp("1", ptr.o->v.string) == 0){
			curr_topic->want_retained = true;
		} else {
			curr_topic->want_retained = false;
		}
		curr_topic->mid = i+1+100;
		*curr_topic++;
	}

	uci_free_context(c);
	return SUB_SUC;

	fail:
		uci_free_context(c);
		return SUB_GEN_ERR;
}

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
	
	if (con->has_will)
		if ((rc = mosquitto_will_set(mosq, con->will->topic, \
		con->will->payloadlen, con->will->payload, con->will->qos, \
					con->will->retain)) != EXIT_SUCCESS)
			return SUB_GEN_ERR;
	if (con->use_tls){
		if(con->tls_insecure && (rc = mosquitto_tls_insecure_set(mosq, \
						true)) != MOSQ_ERR_SUCCESS)
			return SUB_GEN_ERR;
		if ((rc = mosquitto_tls_set(mosq, con->cafile, NULL, \
		con->certfile, con->keyfile, &pw_callback)) != EXIT_SUCCESS){
			log_db(client->db, &(client->n_msg), "error", "-", \
								"Tls error");
			return SUB_GEN_ERR;
		}
	}

	return SUB_SUC;
}

//==========================================================================
// CALLBACKS
//==========================================================================

void on_message(struct mosquitto *mosq, void *obj, \
					const struct mosquitto_message *message)
{
	struct client_data *client = (struct client_data*)obj;
	if (log_db(client->db, &(client->n_msg), "message", message->topic, \
	(char*)message->payload) != SUB_SUC)
		interupt = INT_PRE_DISC;
	printf("message received\ntopic: %s\npayload: %s\n\n", message->topic,\
		(char*)message->payload);
}

void on_connect(struct mosquitto *mosq, void *obj, int rc)
{
	char buff[100];
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
	default:
		strcpy(buff, "client connected");
		break;
	}
	if (log_db(client->db, &(client->n_msg), "info", "-", buff) != SUB_SUC)
		interupt = INT_PRE_DISC;

}

void on_disconnect(struct mosquitto *mosq, void *obj, int rc)
{
	char buff[100];
	struct client_data *client = (struct client_data*)obj;
	switch (rc){
	case 0:
		strcpy(buff, "client disconnected");
		break;
		strcpy(buff, "unexpected disconnect");
		break;
	}
	log_db(client->db, &(client->n_msg), "info", "-", buff);
	interupt = INT_DONE_DISC;
}

void on_subscribe(struct mosquitto *mosq, void *obj, int mid, \
					int qos_count, const int *granted_qos)
{
	struct client_data *client = (struct client_data*)obj;
	struct topic_data *top;
	for (int i = 0; i < client->n_tops; i++){
		top = client->tops+i;
		if (top->mid == mid){
			top->status = T_SUB;
			printf("subscribed: %s\n", top->name);
			if (log_db(client->db, &(client->n_msg), "info", \
					top->name, "subscribed") != SUB_SUC)
				interupt = INT_PRE_DISC;
			break;
		}
	}
}

void on_unsubscribe(struct mosquitto *mosq, void *obj, int mid)
{
	struct client_data *client = (struct client_data*)obj;
	struct topic_data *top;
	for (int i = 0; i < client->n_tops; i++){
		top = client->tops+i;
		if (top->mid == mid){
			top->status = T_UNSUB;
			printf("unsubscribed: %s\n", top->name);
			if (log_db(client->db, &(client->n_msg), "info", \
					top->name, "unsubscribed") != SUB_SUC)
				interupt = INT_PRE_DISC;
			break;
		}
	}
}

void on_log(struct mosquitto *mosq, void *obj, int level, const char *str)
{
	switch (level){
	case (MOSQ_LOG_INFO):
		printf("INFO %s", str);
		break;
	case (MOSQ_LOG_NOTICE):
		printf("NOTICE %s", str);
		break;
	case (MOSQ_LOG_WARNING):
		printf("WARNING %s", str);
		break;
	case (MOSQ_LOG_ERR):
		printf("ERROR: %s", str);
		break;
	case (MOSQ_LOG_DEBUG):
		printf("DEBUG: %s", str);
		break;
	default:
		printf("OTHER: %s", str);
		break;
	}
}

int pw_callback(char *buf, int size, int rwflag, void *userdata){
	printf("%s\n", "pw_callback\n");
	struct client_data *client = (struct client_data*)userdata;
	strcpy(buf, client->con->password);
}

//==========================================================================
// CONNENTION HANDLING
//==========================================================================

int connect_to_broker(struct mosquitto *mosq, struct client_data *client)
{
	int rc;
	mosquitto_connect_callback_set(mosq, &on_connect);
	mosquitto_disconnect_callback_set(mosq, &on_disconnect);
	if ((rc = mosquitto_connect(mosq, client->con->host, \
	client->con->port, client->con->keep_alive)) != MOSQ_ERR_SUCCESS){
		char buff[30];
		sprintf(buff, "connection failed: %d", rc);
		printf("%s\n", buff);
		log_db(client->db, &(client->n_msg), "error", "-", buff);
		return SUB_GEN_ERR;
	} 
	return SUB_SUC;
}

int subscribe_all(struct mosquitto *mosq, struct client_data *client)
{
	int rc;
	if (client->n_tops){
		struct topic_data *top;
		mosquitto_message_callback_set(mosq, &on_message);
		mosquitto_subscribe_callback_set(mosq, &on_subscribe);
		mosquitto_unsubscribe_callback_set(mosq, &on_unsubscribe);
		for (int i = 0; i < client->n_tops; i++){
			top = client->tops+i;
			if ((rc = mosquitto_subscribe(mosq, &(top->mid), \
			top->name, top->qos)) != SUB_SUC){
				printf("Error: MQTT subscriber failed to subscribe to topic: \
				%s, %d\n", top->name, rc);
			}
		}
	}

	return SUB_SUC;
}

int pre_disconnect(struct mosquitto *mosq, struct client_data *client)
{
	int rc;
	struct topic_data *top;
	for (int i = 0; i < client->n_tops; i++){
		top = client->tops+i;
		if ((rc = mosquitto_unsubscribe(mosq, &(top->mid), \
							top->name)) != SUB_SUC){
			printf("Error: MQTT subscriber failed to unsubscribe from topic: \
			%s, %d", top->name, rc);
		}
	}
	interupt = INT_WAIT_UNSUB;
	return SUB_SUC;
}

int disconnect_from_broker(struct mosquitto *mosq)
{
	int rc;
	if ((rc = mosquitto_disconnect(mosq)) != SUB_SUC)
		printf("Error: MQTT subscriber failed to disconnect: %d", rc);
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
				printf("Error: MQTT subscriber failed to disconnect\n");
			break;
		case INT_WAIT_UNSUB: //waits while client unsubscribes all topics
			if ((rc = test_all_t_status(client, T_UNSUB)) < SUB_SUC){
				printf("Error: MQTT subscriber failed to disconnect\n");
			} else if (rc == SUB_SUC){
				interupt = INT_READY_DISC;
			} else {
				if ((rc = mosquitto_loop(mosq, -1, 1)) != SUB_SUC){
					printf("Error: failed to loop: %d\n", rc);
					log_db(client->db, &(client->n_msg), 
					"error", "-", "failed to unsubscribe");
					interupt = INT_READY_DISC;
				}
			}
			break;
		case INT_READY_DISC: //disconnenct client
			interupt = INT_ABSENT;
			if ((rc = disconnect_from_broker(mosq)) != SUB_SUC){
				printf("Error: MQTT subscriber failed to disconnect\n");
				interupt = INT_DONE_DISC;
			}
			break;
		case INT_DONE_DISC: //goes to exit freeing part
			goto exit;
			break;
		default:
			if ((rc = mosquitto_loop(mosq, -1, 1)) != SUB_SUC){
				printf("Error: failed to loop: %d\n", rc);
				interupt = INT_PRE_DISC;
			}
			break;
		}
	}
	exit:
		return;
}

//==========================================================================
// FREEING MEMORY
//==========================================================================

void free_topics(struct topic_data *topics, int len)
{
	if (topics != NULL){
		for (int i = 0; i < len; i++)
			free((topics+i)->name);
		free(topics);
	}
}

void free_con_data(struct connect_data *con)
{
	if (con != NULL){
		free(con->username);
		free(con->password);
		free(con->host);
		free(con->cafile);
		free(con->certfile);
		free(con->keyfile);
		if (con->will != NULL){
			free(con->will->topic);
			free(con->will->payload);
			free(con->will);
		}
		// if (con->tls != NULL){
		// 	free(con->tls->cafile);
		// 	free(con->tls->capath);
		// 	free(con->tls->certfile);
		// 	free(con->tls->keyfile);
		// 	free(con->tls->ciphers);
		// 	free(con->tls->tls_version);
		// 	free(con->tls);
		// }
	}
	free(con);
}

void free_client(struct client_data *client)
{
	if (client != NULL){
		free_con_data(client->con);
		free_topics(client->tops, client->n_tops);
	}
	free(client);
}

//==========================================================================
// HELPERS
//==========================================================================

int get_conf_ptr(struct uci_context *c, struct uci_ptr *ptr, char \
				*buff, const char *path, const int path_len)
{
	memset(buff, '\0', path_len);
	strcpy(buff, path);
	if (uci_lookup_ptr(c, ptr, buff, true) != UCI_OK)
		return SUB_GEN_ERR;
	return SUB_SUC;
}

int test_all_t_status(struct client_data *client, int status)
{
	struct topic_data *top;
	if (client != NULL){
		for (int i = 0; i < client->n_tops; i++){
			top = client->tops+i;
			if (top->status != status)
				return SUB_FAIL;
		}
		return SUB_SUC;
	}
	return SUB_GEN_ERR;
}

static char *rand_string(char *str, size_t size)
{
	const char charset[] = \
	"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789,.-#'?!";
	if (size) {
		--size;
		for (size_t n = 0; n < size; n++) {
			int key = rand() % (int) (sizeof charset - 1);
			str[n] = charset[key];
		}
		str[size] = '\0';
	}
	return str;
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
	if ((rc = get_con_conf(client)) != SUB_SUC)
		goto nmosq_exit;
	if ((rc = get_top_conf(client)) != SUB_SUC)
		goto nmosq_exit;

	client->db = db; //need to fix this
	
	mosquitto_lib_init();
	
	if ((rc = init_client(&mosq, client)) != SUB_SUC)
		goto ncon_exit;
	// mosquitto_log_callback_set(mosq, &on_log);
	if ((rc = connect_to_broker(mosq, client)) != SUB_SUC)
		goto ncon_exit;
	if ((rc = subscribe_all(mosq, client)) != SUB_SUC)
		interupt = INT_PRE_DISC;
	
	main_loop(mosq, client);

	con_exit: //connected clent exit
		
	ncon_exit: //when client was never connected or already disconnected
		mosquitto_destroy(mosq);
		mosquitto_lib_cleanup();
	nmosq_exit: //when mosq lib was never initialized or already freed
		free_client(client);
		// syslog(LOG_ERR, "Error: MQTT subscriber failed");
	db_exit:
		sqlite3_close(db);
		closelog();
		return rc == 0 ? 0 : -1;
}