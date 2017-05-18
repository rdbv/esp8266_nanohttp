#ifndef NANOHTTP_H
#define NANOHTTP_H

#include <string.h>
#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "user_interface.h"
#include "espconn.h"

#define BUFFER_SIZE 2048

/* Max length for URI */
#define URI_LENGTH 64
/* Max pages active */
#define MAX_PAGES 8
/* Flash base for pages code */
#define FLASH_BASE 0x7e000


/* Request type, support only for needed */
typedef enum {
    GET, 
    POST,
    RQ_ERR
} Request_Type;

LOCAL char* responses_str[] = {
    "200 OK",
    "404 Not Found",
    "500 Internal Server Error"
};

typedef enum {
    OK_,
    NOT_FOUND,
    RS_ERR
} Response_Type;

LOCAL char* ctypes_str[] = {
    "text/html"
};

typedef enum {
    text_html,
} Content_Type;



/* Just request */
typedef struct {
    Request_Type type;
    char uri[URI_LENGTH];
    uint32_t uri_length;
    uint32_t uri_hash;
} HTTP_Request;

// URI Entry
typedef struct {
    uint32_t uri_hash;
    uint32_t offset;
    uint32_t size;
    uint32_t padding;
    void (*callback)(void);
} URI_Entry;

/* Response */
typedef struct {
    Response_Type rtype;
    Content_Type ctype;
    URI_Entry* uri;
    uint32_t chunks_left;
    uint32_t chunks_sent;
    uint32_t rest;
    uint8_t  send_zero;
} HTTP_Response;


typedef struct {
    URI_Entry uri[MAX_PAGES];
    uint32_t  uri_count;
    HTTP_Request request;
    HTTP_Response response;
    uint8_t data[BUFFER_SIZE];
    uint8_t processing;
    struct espconn esp_conn;
    esp_tcp esp_tcp;
} Nano_Http;


/* Parsing */
Request_Type ICACHE_FLASH_ATTR parse_request_type(char*);
bool         ICACHE_FLASH_ATTR parse_uri(HTTP_Request*, char*);
bool         ICACHE_FLASH_ATTR parse_request(HTTP_Request*, char*);

/* Processing */
URI_Entry*   ICACHE_FLASH_ATTR get_uri_entry(uint32_t);
uint32_t     ICACHE_FLASH_ATTR fnv_hash(const char*, uint32_t);
uint32_t     ICACHE_FLASH_ATTR build_response(HTTP_Response*);
bool         ICACHE_FLASH_ATTR process_request(char*);

/* Utils */
bool         ICACHE_FLASH_ATTR init_nanohttp(Nano_Http*);
void         ICACHE_FLASH_ATTR read_pages_from_flash();
bool         ICACHE_FLASH_ATTR add_callback(const char*, void (*callback)(void) );

/* Logging */
void         ICACHE_FLASH_ATTR dump_request(HTTP_Request*);
void         ICACHE_FLASH_ATTR dump_uri_entry(URI_Entry*);
void         ICACHE_FLASH_ATTR dump_datahex(char*, unsigned int);
void         ICACHE_FLASH_ATTR assert(bool, const char*);
char*        ICACHE_FLASH_ATTR ip_as_str(struct espconn*);

/* ESP8266-stuff */

void         ICACHE_FLASH_ATTR _client_connected(void*);
void         ICACHE_FLASH_ATTR _client_disconnected(void*);
void         ICACHE_FLASH_ATTR _client_reconnected(void*, sint8);
void         ICACHE_FLASH_ATTR _client_recv(void*, char*, uint16_t);
void         ICACHE_FLASH_ATTR _client_sent(void*);

#define LOG_NHTTP(...) os_printf("[NHTTP] ");os_printf(__VA_ARGS__)

#endif
