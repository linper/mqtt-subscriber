#ifndef CONF_H
#define CONF_H

#include <stdlib.h>
#include <string.h>
#include <uci.h>
#include "sqlite3.h"
#include "utils.h"
#include "helpers.h"
#include "glist.h"

//reads all configurations
int get_conf(struct client_data *client);
//reads topics' data from config file
int get_top_conf(struct client_data *client);
//reads connection data from config file
int get_con_conf(struct client_data *client);
//reads tls data from config file
int get_tls_conf(struct uci_context* c, struct uci_ptr *ptr, \
						struct connect_data *con);
//reads event data from config file
int get_ev_conf(struct client_data *client);
int get_conf_ptr(struct uci_context *c, struct uci_ptr *ptr, char *buff, \
					const char *path, const int path_len);
int link_ev(struct client_data *client);

#endif