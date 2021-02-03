#include "db.h"


int init_db(sqlite3 **db, const char *db_name)
{
	int rc;
	char *err;
	//opens DB
	if ((rc = sqlite3_open(db_name, db)) != SQLITE_OK)
		return SUB_GEN_ERR;
	//creates table of it's not present
	char *sql = "CREATE TABLE IF NOT EXISTS Logs(timestamp TEXT, topic \
							TEXT, message TEXT);";
	if ((rc = sqlite3_exec(*db, sql, 0, 0, &err)) != SQLITE_OK){
		char buff[strlen(err) + 50];
		sprintf(buff, "MQTT subscriber DB failed: %s", err);
		log_err(buff);
		sqlite3_free(err);
		return SUB_GEN_ERR;
	}
	return SUB_SUC;
}

int log_db(sqlite3 *db, char *topic, char *message)
{
	char buff[100 + strlen(topic) + strlen(message)];
	int rc;
	char *err;

	if ((rc = sprintf(buff, "INSERT INTO Logs VALUES(datetime('now'), \
			'%s', '%s');", topic, message)) < 0)
		return SUB_GEN_ERR;

	if ((rc = sqlite3_exec(db, buff, 0, 0, &err)) != SQLITE_OK){
		char buff[strlen(err) + 50];
		sprintf(buff, "MQTT subscriber DB failed: %s", err);
		log_err(buff);
		sqlite3_free(err);
		return SUB_GEN_ERR;
	}
	return SUB_SUC;
}
