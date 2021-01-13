#include "mqtt_sub.h"

const char* DB = "/usr/share/mqtt_sub/mqtt_sub.db";
volatile int interupt = 0;

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

typedef struct topic_data{
    char* name;
    int qos;
    bool want_retained;
    enum t_status status;
    int mid;
} topic_data;

typedef struct connect_data{
    char* host;
    int port;
    char* username;
    char* password;
    struct libmosquitto_will* will;
    struct libmosquitto_tls* tls;
    int keep_alive;
    bool is_clean;
    bool use_tls;
    bool has_will;
} connenct_data;

typedef struct client_data{
    connect_data* con;
    topic_data* tops;
    int n_tops;
    sqlite3* db;
    int n_msg;
} client_data;

void sigHandler(int signo){
    interupt = INT_PRE_DISC;
}

//==========================================================================
// DATABASE
//==========================================================================

int init_db(sqlite3* db){
    int rc;
    char* err;

    if((rc = sqlite3_open(DB, &db)) != SQLITE_OK) {
        sqlite3_close(db);
        return SUB_GEN_ERR;
    }

    char* sql = "DROP TABLE IF EXISTS logs;"
                "CREATE TABLE Logs(id INT, timestamp TEXT, type TEXT, topic TEXT, message TEXT);";

    if((rc = sqlite3_exec(db, sql, 0, 0, &err)) != SQLITE_OK){
        printf("error: %s\n", err);
        sqlite3_free(err);
        sqlite3_close(db);
        return SUB_GEN_ERR;
    }
    sqlite3_close(db);
    return SUB_SUC;
}

int log_db(sqlite3* db, int* n_msg, char* type, char* topic, char* message){
    char buff[100+strlen(type)+strlen(topic)+strlen(message)];
    int rc;
    char* err;

    if((rc = sprintf(buff, "INSERT INTO Logs VALUES(%d, datetime('now'), '%s', '%s', '%s');", *n_msg, type, topic, message)) < 0) return SUB_GEN_ERR;

    if((rc = sqlite3_open(DB, &db)) != SQLITE_OK) {
        sqlite3_close(db);
        return SUB_GEN_ERR;
    }

    if((rc = sqlite3_exec(db, buff, 0, 0, &err)) != SQLITE_OK){
        printf("error: %s\n", err);
        sqlite3_free(err);
        sqlite3_close(db);
        return SUB_GEN_ERR;
    }
    (*n_msg)++;
    sqlite3_close(db);
    return SUB_SUC;
}

//==========================================================================
// READING CONFIGURATIONS AND INIT
//==========================================================================

int get_con_conf(client_data* client){
    connect_data* con = (connect_data*)calloc(1, sizeof(connect_data));
    client->con = con;
    struct uci_context *c;
    struct uci_ptr ptr;
    int rc;
    const size_t PATH_LEN = 256;
    char path[PATH_LEN];

    c = uci_alloc_context();
    // *con = *(connect_data*)calloc(1, sizeof(connect_data));
    //getting remote port
    strcpy(path, "mqtt_sub.mqtt_sub.remote_port");
    if((rc = uci_lookup_ptr(c, &ptr, path, true)) != UCI_OK){goto fail;}
    if(ptr.o != NULL){
        con->port = atoi(ptr.o->v.string);
    }
    //geting remote broker address
    memset(path, '\0', PATH_LEN);
    strcpy(path, "mqtt_sub.mqtt_sub.remote_addr");
    if((rc = uci_lookup_ptr(c, &ptr, path, true)) != UCI_OK){goto fail;}
    if(ptr.o != NULL){
        con->host = strcpy(malloc(strlen(ptr.o->v.string)), ptr.o->v.string);
    }
    //geting remote username
    memset(path, '\0', PATH_LEN);
    strcpy(path, "mqtt_sub.mqtt_sub.username");
    if((rc = uci_lookup_ptr(c, &ptr, path, true)) != UCI_OK){goto fail;}
    if(ptr.o != NULL){
        con->username = strcpy(malloc(strlen(ptr.o->v.string)), ptr.o->v.string);
    }
    //geting remote password
    memset(path, '\0', PATH_LEN);
    strcpy(path, "mqtt_sub.mqtt_sub.password");
    if((rc = uci_lookup_ptr(c, &ptr, path, true)) != UCI_OK){goto fail;}
    if(ptr.o != NULL){
        con->password = strcpy(malloc(strlen(ptr.o->v.string)), ptr.o->v.string);
    }
    //geting keep alive value
    memset(path, '\0', PATH_LEN);
    strcpy(path, "mqtt_sub.mqtt_sub.keep_alive");
    if((rc = uci_lookup_ptr(c, &ptr, path, true)) != UCI_OK){goto fail;}
    if(ptr.o != NULL){
        con->keep_alive = atoi(ptr.o->v.string);
    }else{
        con->keep_alive = 60; //default 60 seconds
    }
    //geting is_clean session value
    memset(path, '\0', PATH_LEN);
    strcpy(path, "mqtt_sub.mqtt_sub.is_clean");
    if((rc = uci_lookup_ptr(c, &ptr, path, true)) != UCI_OK){goto fail;}
    if(ptr.o != NULL && strcmp("0", ptr.o->v.string) == 0){
        con->is_clean = false;
    }else{
        con->is_clean = true;
    }
    //geting has_will value
    memset(path, '\0', PATH_LEN);
    strcpy(path, "mqtt_sub.mqtt_sub.has_will");
    if((rc = uci_lookup_ptr(c, &ptr, path, true)) != UCI_OK){goto fail;}
    if(ptr.o != NULL && strcmp("1", ptr.o->v.string) == 0){
        con->has_will = true;
//==========================================================================
// TODO: get will conf
//==========================================================================
    }
    //geting use_tls value
    memset(path, '\0', PATH_LEN);
    strcpy(path, "mqtt_sub.mqtt_sub.use_tls");
    if((rc = uci_lookup_ptr(c, &ptr, path, true)) != UCI_OK){goto fail;}
    if(ptr.o != NULL && strcmp("1", ptr.o->v.string) == 0){
        con->use_tls = true;
//==========================================================================
// TODO: get tls conf
//==========================================================================

    }

    uci_free_context(c);
    return SUB_SUC;

    fail:
        uci_free_context(c);
        return SUB_GEN_ERR;
}

int get_top_conf(client_data* client){
    topic_data* curr_topic;
    struct uci_context *c;
    struct uci_ptr ptr;
    int rc;
    const size_t PATH_LEN = 256;
    char path[PATH_LEN];

    c = uci_alloc_context();
    //getting number of topics
    strcpy(path, "mqtt_sub");
    if((rc = uci_lookup_ptr(c, &ptr, path, true)) != UCI_OK){goto fail;}
    client->n_tops = ptr.p->n_section - 1;

    curr_topic = (topic_data*)calloc(client->n_tops, sizeof(topic_data));
    client->tops = curr_topic;
    for (int i = 0; i < client->n_tops; i++){
        //geting qos value
        memset(path, '\0', PATH_LEN);
        sprintf(path, "mqtt_sub.@topic[%d].qos", i);
        if((rc = uci_lookup_ptr(c, &ptr, path, true)) != UCI_OK){goto fail;}
        if(ptr.o == NULL){goto fail;}
        curr_topic->qos = atoi(ptr.o->v.string); //on failure qos becomes 0
        //geting topic name value
        memset(path, '\0', PATH_LEN);
        sprintf(path, "mqtt_sub.@topic[%d].topic", i);
        if((rc = uci_lookup_ptr(c, &ptr, path, true)) != UCI_OK && ptr.o != NULL){goto fail;}
        if(ptr.o == NULL || ptr.o->v.string == NULL){goto fail;}
        curr_topic->name = (char*)malloc(strlen(ptr.o->v.string)+1);
        strcpy(curr_topic->name, ptr.o->v.string);
        //geting want_retained value
        memset(path, '\0', PATH_LEN);
        sprintf(path, "mqtt_sub.@topic[%d].want_retained", i);
        if((rc = uci_lookup_ptr(c, &ptr, path, true)) != UCI_OK){goto fail;}
        if(ptr.o != NULL && strcmp("1", ptr.o->v.string) == 0){
            curr_topic->want_retained = true;
        }else{
            curr_topic->want_retained = false;
        }
        curr_topic->mid = i+1+100;
        *curr_topic++;
    }

    uci_free_context(c);
    return SUB_SUC;

    fail:
        uci_free_context(c);
        return SUB_GEN_ERR;
}

int init_client(struct mosquitto** mosq_ptr, client_data* client){
    int rc = 0;
    struct mosquitto* mosq;
    char* id = NULL;
    connect_data* con = client->con;

    if(!con->is_clean){
        id = rand_string(id, 31);
    }
    if((mosq = mosquitto_new(id, con->is_clean, client)) == NULL){
        return SUB_GEN_ERR;
    }
    *mosq_ptr = mosq;
    if(con->username != NULL && con->password != NULL){
        if((rc = mosquitto_username_pw_set(mosq, con->username, con->password)) != EXIT_SUCCESS){
            return SUB_GEN_ERR;
        }
    }
    //==========================================================================
    // TODO: configure will
    //==========================================================================
    
    if(con->has_will){
        // if((rc = int mosquitto_will_set(mosq, con->username, con->password)) != EXIT_SUCCESS){
        //     return SUB_GEN_ERR;
        // }
    }
    //==========================================================================
    // TODO: configure tls
    //==========================================================================
    if(con->use_tls){
        // if((rc = int mosquitto_tls_set(mosq, con->username, con->password)) != EXIT_SUCCESS){
        //     return SUB_GEN_ERR;
        // }
    }

    return SUB_SUC;
}

//==========================================================================
// CALLBACKS
//==========================================================================

void on_message(struct mosquitto* mosq, void* obj, const struct mosquitto_message* message){
    client_data* client = (client_data*)obj;
    if(log_db(client->db, &(client->n_msg), "message", message->topic, (char*)message->payload));
    printf("message received\ntopic: %s\npayload: %s\n\n", message->topic, (char*)message->payload);
}

void on_connect(struct mosquitto* mosq, void* obj, int rc){
    switch (rc){
        case 1:
            printf("%s\n", "connection refused (unacceptable protocol version)");
            break;
        case 2:
            printf("%s\n", "connection refused (identifier rejected)");
            break;
        case 3:
            printf("%s\n", "connection refused (broker unavailable)");
            break;
        default:
            printf("%s: %d\n", "connected", rc);
            break;
    }
}

void on_disconnect(struct mosquitto* mosq, void* obj, int rc){
    switch (rc){
        case 0:
            printf("client disconnected normaly");
            break;
        printf("unexpected disconnect with rc: %d\n", rc);
            break;
    }
    interupt = INT_DONE_DISC;
}

void on_subscribe(struct mosquitto* mosq, void* obj, int mid, int qos_count, const int* granted_qos){
    client_data* client = (client_data*)obj;
    topic_data* top;
    for (int i = 0; i < client->n_tops; i++){
        top = client->tops+i;
        if(top->mid == mid){
            top->status = T_SUB;
            printf("subscribed: %s\n", top->name);
            break;
        }
    }
}

void on_unsubscribe(struct mosquitto* mosq, void* obj, int mid){
    client_data* client = (client_data*)obj;
    topic_data* top;
    for (int i = 0; i < client->n_tops; i++){
        top = client->tops+i;
        if(top->mid == mid){
            top->status = T_UNSUB;
            printf("unsubscribed: %s\n", top->name);
            break;
        }
    }
}

void on_log(struct mosquitto *mosq, void *obj, int level, const char *str){
    switch (level){
    case (MOSQ_LOG_INFO):
        printf("INFO %s", str);
        break;
    case (MOSQ_LOG_NOTICE):
        printf("NOTICE %s", str);
        break;
    case (MOSQ_LOG_WARNING):
        printf("WARNING %s", str);
        break;
    case (MOSQ_LOG_ERR):
        printf("ERROR: %s", str);
        break;
    case (MOSQ_LOG_DEBUG):
        printf("DEBUG: %s", str);
        break;
    default:
        printf("OTHER: %s", str);
        break;
    }
}

//==========================================================================
// CONNENTION HANDLING
//==========================================================================

int connect_to_broker(struct mosquitto* mosq, connect_data* con){
    int rc;
    mosquitto_connect_callback_set(mosq, &on_connect);
    mosquitto_disconnect_callback_set(mosq, &on_disconnect);
    if((rc = mosquitto_connect(mosq, con->host, con->port, con->keep_alive)) != MOSQ_ERR_SUCCESS){return SUB_GEN_ERR;}
    // if((rc = mosquitto_loop(mosq, -1, 1)) != MOSQ_ERR_SUCCESS){return SUB_GEN_ERR;}
    return SUB_SUC;
}

int subscribe_all(struct mosquitto* mosq, client_data* client){
    int rc;
    if(client->n_tops){
        topic_data* top;
        mosquitto_message_callback_set(mosq, &on_message);
        mosquitto_subscribe_callback_set(mosq, &on_subscribe);
        mosquitto_unsubscribe_callback_set(mosq, &on_unsubscribe);
        for (int i = 0; i < client->n_tops; i++){
            top = client->tops+i;
            if((rc = mosquitto_subscribe(mosq, &(top->mid), top->name, top->qos)) != SUB_SUC){
                printf("Error: MQTT subscriber failed to subscribe to topic: %s, %d", top->name, rc);
            }
        }
    }

    return SUB_SUC;
}

int pre_disconnect(struct mosquitto* mosq, client_data* client){
    int rc;
    topic_data* top;
    for (int i = 0; i < client->n_tops; i++){
        top = client->tops+i;
        if((rc = mosquitto_unsubscribe(mosq, &(top->mid), top->name)) != SUB_SUC){
            printf("Error: MQTT subscriber failed to unsubscribe from topic: %s, %d", top->name, rc);
        }
    }
    interupt = INT_WAIT_UNSUB;
    return SUB_SUC;
}

int disconnect_from_broker(struct mosquitto* mosq){
    int rc;
    if((rc = mosquitto_disconnect(mosq)) != SUB_SUC){
        printf("Error: MQTT subscriber failed to disconnect: %d", rc);
    }
    return SUB_SUC;
}


void main_loop(struct mosquitto* mosq, client_data* client){
    int rc;
    while(true){
        switch (interupt){
            case INT_PRE_DISC: //unsubscribes topics and disconnenct cliet
                interupt = INT_ABSENT;
                if((rc = pre_disconnect(mosq, client)) != SUB_SUC){
                    printf("Error: MQTT subscriber failed to disconnect");
                }
                break;
            case INT_WAIT_UNSUB: //waits while client unsubscribes all topics
                if((rc = test_all_t_status(client, T_UNSUB)) < SUB_SUC){
                    printf("Error: MQTT subscriber failed to disconnect");
                }else if(rc == SUB_SUC){
                    interupt = INT_READY_DISC;
                }else{
                    if((rc = mosquitto_loop(mosq, -1, 1)) != SUB_SUC){
                        printf("Error: failed to loop: %d\n", rc);
                    }
                    //==========================================================================
                    // TODO: handle failed unsubscription attenpts
                    //==========================================================================
                }
                break;
            case INT_READY_DISC: //unsubscribes topics and disconnenct cliet
                interupt = INT_ABSENT;
                if((rc = disconnect_from_broker(mosq)) != SUB_SUC){
                    printf("Error: MQTT subscriber failed to disconnect");
                }
                break;
            case INT_DONE_DISC: //goes to exit freeing part
                goto exit;
                break;
            default:
                if((rc = mosquitto_loop(mosq, -1, 1)) != SUB_SUC){
                    printf("Error: failed to loop: %d\n", rc);
                }
                break;
        }
    }
    exit:
        return;
}

//==========================================================================
// FREEING MEMORY
//==========================================================================

void free_topics(topic_data* topics, int len){
    if(topics != NULL){
        for (int i = 0; i < len; i++){
            free((topics+i)->name);
        }
        free(topics);
    }
}

void free_con_data(connect_data* con){
    if(con != NULL){
        free(con->username);
        free(con->password);
        free(con->host);
        if(con->will != NULL){
            free(con->will->topic);
            free(con->will->payload);
            free(con->will);
        }
        if(con->tls != NULL){
            free(con->tls->cafile);
            free(con->tls->capath);
            free(con->tls->certfile);
            free(con->tls->keyfile);
            free(con->tls->ciphers);
            free(con->tls->tls_version);
            free(con->tls);
        }
    }
    free(con);
}

void free_client(client_data* client){
    if(client != NULL){
        free_con_data(client->con);
        free_topics(client->tops, client->n_tops);
    }
    free(client);
}

//==========================================================================
// HELPERS
//==========================================================================

int test_all_t_status(client_data* client, int status){
    topic_data* top;
    if(client != NULL){
        for (int i = 0; i < client->n_tops; i++){
            top = client->tops+i;
            if(top->status != status)return SUB_FAIL;
        }
        return SUB_SUC;
    }
    return SUB_GEN_ERR;
}

static char *rand_string(char *str, size_t size){
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789,.-#'?!";
    if (size) {
        --size;
        for (size_t n = 0; n < size; n++) {
            int key = rand() % (int) (sizeof charset - 1);
            str[n] = charset[key];
        }
        str[size] = '\0';
    }
    return str;
}

//==========================================================================
// MAIN
//==========================================================================

int main(int argc, char* argv[]){
    int rc, erc;
    time_t t;
    struct mosquitto* mosq;
    client_data* client;
    sqlite3* db;

    srand((unsigned) time(&t));
    signal(SIGINT, sigHandler);
    signal(SIGTERM, sigHandler);
    openlog(NULL, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
    
    if((rc = init_db(db)) != SUB_SUC) goto nmosq_exit;
    

    if((client = (client_data*)calloc(1, sizeof(client_data))) == NULL)goto nmosq_exit;
    if((rc = get_con_conf(client)) != SUB_SUC) goto nmosq_exit;
    if((rc = get_top_conf(client)) != SUB_SUC) goto nmosq_exit;

    client->db = db; //need to fix this
    
    mosquitto_lib_init();
    
    if((rc = init_client(&mosq, client)) != SUB_SUC) goto ncon_exit;
    // mosquitto_log_callback_set(mosq, &on_log);
    if((rc = connect_to_broker(mosq, client->con)) != SUB_SUC) goto ncon_exit;
    if((rc = subscribe_all(mosq, client)) != SUB_SUC) interupt = INT_PRE_DISC;
    
    main_loop(mosq, client);

    con_exit: //connected clent exit
        
    ncon_exit: //when client was never connected or already disconnected
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
    nmosq_exit: //when mosq lib was never initialized or already freed
        free_client(client);
        // syslog(LOG_ERR, "Error: MQTT subscriber failed");
        closelog();
        return rc == 0 ? 0 : -1;
}