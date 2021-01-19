#ifndef DB_H
#define DB_H

#include "stdio.h"
#include "string.h"
#include "sqlite3.h"
#include "utils.h"


extern int init_db(sqlite3 **db, const char *db_name);
extern int log_db(sqlite3 *db, int *n_msg, char *topic, char *message);

#endif