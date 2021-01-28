#ifndef MAIL_H
#define MAIL_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

#include <curl/curl.h>

#include "glist.h"
#include "utils.h"

size_t payload_source(void *ptr, size_t size, size_t nmemb, void *userp);
int build_message(struct glist *message, struct event_data *ev, \
							struct msg_dt *dt, char *topic);
int send_mail(struct event_data *event, struct msg_dt *dt, char *topic);

#endif