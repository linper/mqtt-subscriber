
#include "mqtt_sub.h"

enum inter_status{
	INT_ABSENT,
	INT_READY_DISC,
	INT_DONE_DISC,
};

volatile int interupt = 0;
int reload = REL_ABSENT;
long sub_lmodt = 0;
long top_lmodt = 0;
long ev_lmodt = 0;


void sigHandler(int signo)
{
	interupt = INT_READY_DISC;
}

void sigReloder(int signo)
{
	long sub_t = get_last_mod("/etc/config/mqtt_sub");
	long top_t = get_last_mod("/etc/config/mqtt_topics");
	long ev_t = get_last_mod("/etc/config/mqtt_events");
	//if connection or topics' config file was changed, whole client reload in needed
	if (sub_t > sub_lmodt || top_t > top_lmodt){
		top_lmodt = top_t;
		sub_lmodt = sub_t;
		reload = REL_FULL;
		interupt = INT_READY_DISC;
	//if only events' config was changed, only events has to be reloaded
	} else if (ev_t > top_lmodt){
		ev_lmodt = ev_t;
		reload = REL_EV;
	}
}

int write_pid(FILE *fp, int pid)
{
	if ((fp = fopen("/var/run/mqtt_sub.pid", "w+")) == NULL)
		return SUB_GEN_ERR;
	if (fprintf(fp, "%d\n", pid) < 0){
		fclose(fp);
		return SUB_GEN_ERR;
	}
	fclose(fp);
	return SUB_SUC;
}

int verify_pid(int pid)
{
	char name[1024];
	if(name){
		sprintf(name, "/proc/%d/cmdline",pid);
		FILE* f = fopen(name,"r");
		if(f){
			size_t size;
			size = fread(name, 1, 1024, f);
			fclose(f);
			int len = (int)strlen(name);
			//sets first pointer to last eigth byte
			if(size > 0 && strcmp(name + len - 8 , "mqtt_sub") == 0)
				return SUB_SUC;
			else
				return SUB_FAIL;
		}
	}
	return SUB_GEN_ERR;
}

int check_for_rival()
{
	int pid = (int)getpid();
	FILE *fp;
	size_t kill_count = 0;
	int rc = 0;
	fp = fopen("/var/run/mqtt_sub.pid", "r");
	if (!fp){ //file does not exists
		if (write_pid(fp, pid) != SUB_SUC)
			goto error;
	} else { //file exists
		char buff[10];
		if (fgets(buff, sizeof(buff), fp) == EOF)
			goto ferror;
		fclose(fp);
		int old_pid = 0;
		if ((rc = str_to_int(buff, &old_pid)) != SUB_SUC)
			goto error;
		 //rival exists
		if (pid == old_pid)
			return SUB_SUC;
		rc = SUB_FAIL;
		if (kill(old_pid, 0) == 0 && (rc = verify_pid(old_pid)) == SUB_SUC)
			return SUB_FAIL;
		else if (rc == SUB_GEN_ERR)
			goto error;
		//file is empty or bad format
		else if (write_pid(fp, pid) != SUB_SUC)
				goto error;
	}
		return SUB_SUC;
	fail:
		log_err("MQTT subscriber failed, another instance is running");
		return SUB_FAIL;
	ferror:
		fclose(fp);
	error:
		log_err("MQTT subscriber failed check for another instances");
		return SUB_GEN_ERR;
}

//==========================================================================
// CALLBACKS
//==========================================================================

void on_message(struct mosquitto *mosq, void *obj, \
					const struct mosquitto_message *message)
{
	int rc;
	char out_str[strlen(message->payload)*2+10];
	char *out_str_ptr = out_str;
	struct msg *base_msg;
	struct msg *msg;
	struct topic_data *top;
	struct client_data *client = (struct client_data*)obj;
	//gets all matching topics
	struct glist *tops = get_tops(client->tops, message->topic);
	if (count_glist(tops) == 0){
		free_shallow_glist(tops);
		return;
	}
	//parsed jsom string into msg struct
	if ((rc = parse_msg(message->payload, &base_msg)) != SUB_SUC){
		if (rc == SUB_GEN_ERR)
			goto err_msg;
		else
			return;
	}
	//interates through matched topics
	for (size_t i = 0; i < count_glist(tops); i++){
		top = (struct topic_data*)get_glist(tops, i);
		//creates message clones for each topic needs
		if ((msg = filter_msg(top, base_msg)) == NULL)
			goto error;
		//parses msg struct back to json string
		format_out(&out_str_ptr, msg);
		//checks if events occured and executes them
		if (handle_events(top, msg->payload, message->topic) != SUB_SUC)
			goto error;
		//logs messages to DB
		if (log_db(client->db, message->topic, out_str) != SUB_SUC)
			goto error;
		free_shallow_glist(msg->payload);
		free(msg);
	}
	free_shallow_glist(tops);
	return;
	error:
		if(msg){
			free_shallow_glist(msg->payload);
			free(msg->sender);
			free(msg);
		}
		free_msg(base_msg);
	err_msg:
		free_shallow_glist(tops);
		interupt = INT_READY_DISC;
		return;
		
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
		interupt = INT_READY_DISC;
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
	for (size_t i = 0; i < n; i++){//getting subscribed topic
		top = get_glist(client->tops, i);
		if (top->mid == mid){
			top->status = T_SUB;
			break;
		}
	}
}

//==========================================================================
// CONNENTION HANDLING
//==========================================================================

int init_client(struct mosquitto **mosq_ptr, struct client_data *client)
{
	struct mosquitto *mosq;
	char *id = NULL;
	char id_buff[30];
	char *err = NULL;
	struct connect_data *con = client->con;

	if (!con->is_clean)
		id = rand_string(id_buff, sizeof(id_buff));
	if ((mosq = mosquitto_new(id, con->is_clean, client)) == NULL)
		goto error;

	*mosq_ptr = mosq;
	if (con->username != NULL && con->password != NULL)
		if ( mosquitto_username_pw_set(mosq, con->username,\
		con->password) != EXIT_SUCCESS)
			goto error;
	
	if (con->use_tls){
		if(con->tls_insecure && mosquitto_tls_insecure_set(mosq, \
						true) != MOSQ_ERR_SUCCESS)
			goto error;
		if (mosquitto_tls_set(mosq, con->cafile, NULL, \
		con->certfile, con->keyfile, NULL) != EXIT_SUCCESS){
			err = "MQTT subscriber TLS error";
			goto error;
		}
	}

	return SUB_SUC;
	error:
		if (err)
			log_err(err);
		else
			log_err("MQTT subscriber client init error");
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
	time_t t = 0;
	time_t now = 0;
	while(true){
		switch (interupt){
		case INT_READY_DISC: //disconnenct client
			interupt = INT_ABSENT;
			if ((rc = disconnect_from_broker(mosq)) != SUB_SUC)
				interupt = INT_DONE_DISC;
			break;
		case INT_DONE_DISC: //goes to exit freeing part
			goto exit;
			break;
		default: //if no actions needed, loops this part
			if (reload == REL_EV && reconf(client) != SUB_SUC){ //checks if event reloding is needed
				log_err("MQTT subscriber failed to reload events");
				interupt = INT_READY_DISC;
			}
			reload = REL_ABSENT;
			if ((rc = mosquitto_loop(mosq, -1, 1)) != SUB_SUC){
				char buff[50];
				sprintf(buff, "MQTT subscriber failed to loop: %d", rc);
				log_err(buff);
				interupt = INT_READY_DISC;
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
	signal(SIGHUP, sigReloder);
	openlog(NULL, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
	
	//checks if another mqtt_sum instance is already running
	if ((rc = check_for_rival()) != SUB_SUC)
			goto log_exit;

	if ((rc = init_db(&db, "/usr/share/mqtt_sub/mqtt_sub.db")) != SUB_SUC)
		goto db_exit;
	reload: //reload jump point
	if ((client = (struct client_data*)calloc(1, \
					sizeof(struct client_data))) == NULL)
		goto nmosq_exit;
	//setts gets last modification times of config files
	sub_lmodt = get_last_mod("/etc/config/mqtt_sub");
	top_lmodt = get_last_mod("/etc/config/mqtt_topics");
	ev_lmodt = get_last_mod("/etc/config/mqtt_events");
	//getting configuration
	if ((rc = get_conf(client)) != SUB_SUC)
		goto nmosq_exit;
	
	client->db = db;
	
	mosquitto_lib_init();
	//configures client
	if ((rc = init_client(&mosq, client)) != SUB_SUC)
		goto ncon_exit;
	//connects client to broker
	if ((rc = connect_to_broker(mosq, client)) != SUB_SUC)
		goto ncon_exit;
	//subscribes to topics retreived form mqtt_topics config
	if ((rc = subscribe_all(mosq, client)) != SUB_SUC)
		interupt = INT_READY_DISC;
	
	main_loop(mosq, client);

	ncon_exit: //when client was never connected or already disconnected
		mosquitto_destroy(mosq);
		mosquitto_lib_cleanup();
	nmosq_exit: //when mosq lib was never initialized or already freed
		free_client(client);
		if (reload == REL_FULL){ //reloading whole client connection
			interupt = INT_ABSENT;
			reload = REL_ABSENT;
			goto reload;
		}
	db_exit:
		sqlite3_close(db);
	log_exit:
		closelog();
		return rc == 0 ? 0 : -1;
}