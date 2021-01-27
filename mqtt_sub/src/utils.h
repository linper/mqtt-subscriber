#ifndef M_UTILS_H
#define M_UTILS_H

#include <stdlib.h>
#include <stdbool.h>
#include <syslog.h>
#include "sqlite3.h"
#include "glist.h"

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

struct msg_dt{
	char *type;
	char *data;
};

struct msg{
	char *sender;
	struct glist *body;
};

struct event_data{
	int t_id;
	char *field;
	enum ev_dt type;
	enum ev_rules rule;
	union{
		long lng;
		double dbl;
		char *str;
	} target;
	char *s_email;
	char *s_pwd;
	char *r_email;
	char *mail_srv;
	bool en_interv;
	bool en_count;
	long interval;
	long last_ev_t;
	int count;

};

struct topic_data{
	char *name;
	int id;
	int qos;
	bool want_retained;
	bool constrain;
	struct glist *fields;
	enum t_status status;
	int mid;
	struct glist *events;
};

struct connect_data{
	char *host;
	int port;
	char *username;
	char *password;
	int keep_alive;
	bool is_clean;
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
	int n_msg;
};


#ifdef DEBUG
	#define log_err(err) \
	fprintf(stderr, "ERROR: %s\n", err)
#else
	#define log_err(err) \
	syslog(LOG_ERR, "%s\n", err)
#endif

void free_client (struct client_data *client);
void free_msg(struct msg *msg);
void free_top_cb(void *obj);
void free_ev_cb(void *obj);
void free_msg_dt_cb(void *obj);

#endif