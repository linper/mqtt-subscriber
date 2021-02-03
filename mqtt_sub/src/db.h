#ifndef DB_H
#define DB_H

#include <stdio.h>
#include <string.h>
#include "sqlite3.h"
#include "utils.h"

//opens database
int init_db(sqlite3 **db, const char *db_name);
//logs messages to database
int log_db(sqlite3 *db, char *topic, char *message);

#endif