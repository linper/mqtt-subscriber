#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>
#include <stdbool.h>
#include "sqlite3.h"

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

extern struct topic_data topic_data;
extern struct connect_data connect_data;
extern struct client_data client_data;

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
	struct topic_data *tops;
	int n_tops;
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

//tests if all topics have given status
int test_all_t_status(struct client_data *client, int status);
//generates random string of given size 
char *rand_string(char *str, size_t size);

#endif