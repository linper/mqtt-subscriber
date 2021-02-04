#ifndef M_UTILS_H
#define M_UTILS_H

#include <stdlib.h>
#include <stdbool.h>
#include <syslog.h>
#include <stdint.h>
#include "sqlite3.h"
#include "glist.h"


enum reload_status{
	REL_ABSENT,
	REL_FULL,
	REL_EV,
};

//topic subscription status
enum t_status{
	T_UNSUB,
	T_SUB,
};

enum ret_codes{
	SUB_GEN_ERR = -1,
	SUB_SUC = 0,
	SUB_FAIL = 1,
};

enum ev_rules{
	EV_R_EQ = 1,
	EV_R_NEQ = 2,
	EV_R_MT = 3,
	EV_R_LT = 4,
	EV_R_ME = 5,
	EV_R_LE = 6,
};

//event target datatype
enum ev_dt{
	EV_DT_LNG,
	EV_DT_DBL,
	EV_DT_STR,
};

extern struct topic_data topic_data;
extern struct event_data event_data;
extern struct connect_data connect_data;
extern struct client_data client_data;
extern struct msg msg;
extern struct msg_dt msg_dt;

//member of message's payload
struct msg_dt{
	char *type;
	char *data;
};

//message structure
struct msg{ 
	char *sender;
	struct glist *payload; //contains *msg_dt
};

struct event_data{
	int t_id;
	char *field; //target field name
	enum ev_dt type;
	enum ev_rules rule;
	union{ //target datatype
		long lng;
		double dbl;
		char *str;
	} target;
	//sender enail info
	char *sender_email;
	char *username;
	char *password;
	char *smtp_ip;
	int smtp_port;
	//glist of reciever emails
	struct glist *receivers;
	// minimum interval at witch event can be invoked
	long interval;
	//last event occurence
	long last_event;
};

struct topic_data{
	char *name;
	//name divided into parts e.g. aaa/bbb/ccc
	struct glist *name_path;
	int id;
	int qos;
	//if not NULL, contains allowed message fields
	struct glist *fields;
	enum t_status status;
	int mid;//message id, needed when subscribing
	//asociaated events
	struct glist *events;
};

struct connect_data{
	char *host;
	int port;
	char *username;
	char *password;
	int keep_alive;
	bool use_tls;
	bool tls_insecure;
	char *cafile;
	char *keyfile;
	char *certfile;
};

struct client_data{
	struct connect_data *con;
	struct glist *tops;
	struct glist *events;
	sqlite3 *db;
};

//defines where to forward errors
#ifdef DEBUG
	#define log_err(err) \
	fprintf(stderr, "ERROR: %s\n", err)
#else
	#define log_err(err) \
	syslog(LOG_ERR, "%s\n", err)
#endif

//frees whole client
void free_client (struct client_data *client);
//free function for msg struct
void free_msg(struct msg *msg);
//free callback function for topics
void free_top_cb(void *obj);
//free callback function for events
void free_ev_cb(void *obj);
//free callback function for msg_dt structs
void free_msg_dt_cb(void *obj);

#endif