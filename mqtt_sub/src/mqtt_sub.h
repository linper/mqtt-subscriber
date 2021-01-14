#ifndef MQTT_SUB_H
#define MQTT_SUB_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <syslog.h>
#include <signal.h>
#include <unistd.h>
#include <uci.h>
#include <libubox/blobmsg_json.h>
#include <libubox/blobmsg.h>
#include "libubus.h"
#include "mosquitto.h"
#include "sqlite3.h"

struct topic_data;
struct connect_data;
struct client_data;

void sigHandler(int signo);

//initializes database and creates nessary tables
int init_db(sqlite3 **db);
//logs message, message type and topic to database
int log_db(sqlite3 *db, int *n_msg, char *type, char *topic, char *message);
//reads connection data from config file
int get_con_conf(struct client_data *client);
//reads last will data from config file
int get_will_conf(struct uci_context* c, struct uci_ptr *ptr, struct libmosquitto_will *will);
//reads topics' data from config file
int get_top_conf(struct client_data *client);
//creates mosquitto client
int init_client(struct mosquitto **mosq_ptr, struct client_data *client);

void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message);
void on_connect(struct mosquitto *mosq, void *obj, int rc);
void on_disconnect(struct mosquitto *mosq, void *obj, int rc);
void on_subscribe(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos);
void on_unsubscribe(struct mosquitto *mosq, void *obj, int mid);

//connects client to broker
//registers on_connect, on_disconnect callbacks
int connect_to_broker(struct mosquitto *mosq, struct connect_data *con);
//subscribes to all given topics
//registers on_subscribe, on_unsubscribe, on_message callbacs
int subscribe_all(struct mosquitto *mosq, struct client_data *client);
//unsubscribes from topics
int pre_disconnect(struct mosquitto *mosq, struct client_data *client);
//disconnects client fron broker
int disconnect_from_broker(struct mosquitto *mosq);

void free_topics(struct topic_data *topic, int len);
void free_con_data(struct connect_data *con);
void free_clientstruct (struct client_data *client);

int get_conf_ptr(struct uci_context *c, struct uci_ptr *ptr, char *buff, const char *path, const int path_len);
//tests if all topics have given status
int test_all_t_status(struct client_data *client, int status);
//generates random string of given size 
static char *rand_string(char *str, size_t size);

#endif