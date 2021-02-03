#ifndef MQTT_SUB_H
#define MQTT_SUB_H

#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <syslog.h>
#include <signal.h>
#include <time.h>
// #include <pthread.h>

#include "mosquitto.h"
#include "sqlite3.h"

#include "utils.h"
#include "helpers.h"
#include "db.h"
#include "conf.h"
#include "parser.h"
#include "event.h"

void sigHandler(int signo);
//creates if not exist and writes .pid file
int write_pid(FILE *fp, int pid);
//verifies if proces specified by pid is mqtt_sum instance
int verify_pid(int pid);
//checks if other mqtt_sum instance is already running
int check_for_rival();

void on_message(struct mosquitto *mosq, void *obj, \
				const struct mosquitto_message *message);
void on_connect(struct mosquitto *mosq, void *obj, int rc);
void on_disconnect(struct mosquitto *mosq, void *obj, int rc);
void on_subscribe(struct mosquitto *mosq, void *obj, int mid, int qos_count, \
							const int *granted_qos);
//creates mosquitto client
int init_client(struct mosquitto **mosq_ptr, struct client_data *client);
//connects client to broker
//registers on_connect callback
int connect_to_broker(struct mosquitto *mosq, struct client_data *client);
//subscribes to all given topics
int subscribe_all(struct mosquitto *mosq, struct client_data *client);
//disconnects client fron broker
int disconnect_from_broker(struct mosquitto *mosq);

#endif