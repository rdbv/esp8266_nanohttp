#include "nanohttp.h"

Nano_Http* _nhttp;

/* ========== Request parsing ========== */

// Stupid, but simple func
Request_Type parse_request_type(char* data) {
    if(data == NULL) return RQ_ERR;
    
    if(data[0] == 'G' && data[1] == 'E')  return GET;
    if(data[0] == 'P' && data[1] == 'O')  return POST;

    return RQ_ERR;
}

// Get URI
bool parse_uri(HTTP_Request* rq, char* data) {
    // Search / in request
    // HTTP/1.1 /testest \r\n
    char* begin = strchr(data, '/');

    if(!begin) return false;

    char* end = strchr(begin, ' ');

    if(!end)  return false;

    rq->uri_length  = end - begin;

    // Check is in range
    if(rq->uri_length >= URI_LENGTH) return false;

    os_bzero(rq->uri, URI_LENGTH);
    os_memcpy(rq->uri, begin, rq->uri_length);

    // Compute hash
    rq->uri_hash = fnv_hash(rq->uri, rq->uri_length);
    return true;
}

bool parse_request(HTTP_Request* request, char* data) {
    char *token;
    char *rest = data;

    // Method, URI, Version
    token = strtok_r(rest, "\r\n", &rest);
    request->type = parse_request_type(token);

    if(request->type == RQ_ERR) {
        LOG_NHTTP("Unknown request type\r\n");
        return false;
    }

    // parse uri
    if(!parse_uri(request, data)) {
        LOG_NHTTP("Failed to parse URI\r\n");
        return false;
    }

    return true;
}

/* ========== Processing code ========== */

URI_Entry* get_uri_entry(uint32_t hash) {
    for(unsigned int i=0;i<_nhttp->uri_count;++i)
        if(_nhttp->uri[i].uri_hash == hash)
            return &_nhttp->uri[i];
    return NULL;
}

uint32_t fnv_hash(const char* str, uint32_t length) {
    uint32_t h = 2166136261;
    uint32_t i=0;
    for(;i<length;++i)
        h = (h*16777619) ^ str[i];    
    return h;
}

bool process_request(char* data) {
    HTTP_Response* resp = &_nhttp->response;
    HTTP_Request* req = &_nhttp->request;

    LOG_NHTTP("=== REQUEST ===\r\n");

    if(!parse_request(req, data)) {
        os_printf("parsing failed\r\n");
        while(1){};
        //return false;
    }

    dump_request(req);

    if(req->type == POST) {
        LOG_NHTTP("POSCIK: %s\r\n", data);
        return;
    }

    URI_Entry* uri_entry = get_uri_entry(req->uri_hash);

    if(!uri_entry) {
        os_printf("NOT FOUND\r\n");
        return false;
    }
   
    resp->uri = uri_entry;

    if(uri_entry->callback != NULL)
        uri_entry->callback();

    os_printf("Resource size: %d %x\r\n", uri_entry->size, uri_entry->size);
    os_printf("Resource padding: %d\r\n", uri_entry->padding);

    resp->rtype = OK_;
    resp->ctype = text_html;
    resp->chunks_left = uri_entry->size / BUFFER_SIZE;

    if(resp->chunks_left) 
        resp->rest = uri_entry->size % BUFFER_SIZE;
    else
        resp->rest = uri_entry->size;

    resp->chunks_sent = 0;
    resp->send_zero = 0;

    int cnt = os_sprintf(_nhttp->data, "HTTP/1.1 %s\r\n", responses_str[resp->rtype]);
    cnt += os_sprintf(_nhttp->data+cnt, "Content-Type: %s\r\n", ctypes_str[resp->ctype]);
    cnt += os_sprintf(_nhttp->data+cnt, "Connection: Close\r\n");
    cnt += os_sprintf(_nhttp->data+cnt, "Transfer-Encoding: chunked\r\n\r\n");

    os_printf("Chunks: %d\r\nRest: %d\r\n", resp->chunks_left, resp->rest);

    _nhttp->processing = 1;

    LOG_NHTTP("=== EOF_REQ ===\r\n");
}


void cb_en0() {
    LOG_NHTTP("SIEMANKO!\r\n");
    gpio_output_set(BIT5, 0, BIT5, 0);
}
void cb_ds0() {
    LOG_NHTTP("NARAZIE\r\n");
    gpio_output_set(0, BIT5, BIT5, 0);
}

/* Utils */

bool init_nanohttp(Nano_Http* nhttp) {
    _nhttp = nhttp;
    _nhttp->esp_conn.type = ESPCONN_TCP;
    _nhttp->esp_conn.state = ESPCONN_NONE;
    _nhttp->esp_conn.proto.tcp = &nhttp->esp_tcp;
    _nhttp->esp_conn.proto.tcp->local_port = 80;
    _nhttp->processing = 0;
    espconn_regist_connectcb(&_nhttp->esp_conn, _client_connected);

    sint8 ret = espconn_accept(&_nhttp->esp_conn);
    //espconn_regist_time(&_nhttp->esp_conn, 25, 0);
    espconn_tcp_set_max_con_allow(&_nhttp->esp_conn, 1);
    os_printf("espconn_accept [%d] !!! \r\n", ret);

    read_pages_from_flash();
    add_callback("/test_page1", cb_en0);
    add_callback("/test_page2", cb_ds0);
}

void read_pages_from_flash() {
    if(spi_flash_read(FLASH_BASE, &_nhttp->uri_count, 4) != SPI_FLASH_RESULT_OK) {
        LOG_NHTTP("Failed to read page count\r\n");
        return;
    }    

    uint32_t flash_addr = FLASH_BASE + 4;
    uint32_t pad_siz = 0;
    for(unsigned int i=0;i<_nhttp->uri_count;++i, pad_siz=0) {
        if(spi_flash_read(flash_addr, &_nhttp->uri[i].uri_hash, 4) != SPI_FLASH_RESULT_OK) {
            LOG_NHTTP("Failed to read %d hash\r\n", i);
            return;
        }
        if(spi_flash_read(flash_addr+4, &_nhttp->uri[i].offset, 4) != SPI_FLASH_RESULT_OK) {
            LOG_NHTTP("Failed to read %d offset\r\n", i);
            return;
        }
        if(spi_flash_read(flash_addr+8, &pad_siz, 4) != SPI_FLASH_RESULT_OK) {
            LOG_NHTTP("Failed to read %d size\r\n");
            return;
        }
        _nhttp->uri[i].size = (pad_siz & 0xffffff00) >> 8;
        _nhttp->uri[i].padding = pad_siz & 0xff;
        _nhttp->uri[i].callback = NULL;
        flash_addr += 12;
    }
}

bool add_callback(const char* c, void (*callback)(void) ) {
    URI_Entry* uri = get_uri_entry(fnv_hash(c, strlen(c)));
    if(!uri) {
        LOG_NHTTP("DUPCIA\r\n");
        return false;
    }
    uri->callback = callback;
}

/* ========== INFO ========== */

void dump_request(HTTP_Request* rq) {
    os_printf("Type:%d\r\n", rq->type);
    os_printf("URI:%s\r\n", rq->uri);    
    os_printf("URI Len:%d\r\n", rq->uri_length);
    os_printf("hash:%x\r\n", rq->uri_hash);
    //os_printf("padding:%d\r\n", rq->
}

void dump_uri_entry(URI_Entry* en) {
    os_printf("======\r\n");
        os_printf("Hash:%x\r\n", en->uri_hash);
        os_printf("Offs:%d (%x)\r\n", en->offset, en->offset);
        os_printf("Len:%d\r\n", en->size);
    os_printf("==**==\r\n");
}

void dump_datahex(char* data, unsigned int len) {
    for(unsigned int i=0;i<len;++i) {
        os_printf("%x ", data[i]);        
        if((i+1) % 10 == 0)
            os_printf("\r\n");
    }
    os_printf("\r\n");
}

char* ip_as_str(struct espconn* conn)
{
    static char bff0[16];
    os_memset(bff0, 0, 16);
    os_sprintf(bff0, "%d.%d.%d.%d", 
            conn->proto.tcp->remote_ip[0],
            conn->proto.tcp->remote_ip[1],
            conn->proto.tcp->remote_ip[2],
            conn->proto.tcp->remote_ip[3]); 
    return bff0;
}


/* ========== ESP-stuff ========== */

void _client_connected(void* arg) {
    struct espconn *pesp_conn = arg;
    os_printf("Client connected... %s:%d %d\r\n", 
            ip_as_str(pesp_conn),
            pesp_conn->proto.tcp->remote_port,
            pesp_conn->link_cnt);


    espconn_regist_recvcb(pesp_conn, _client_recv);
    espconn_regist_reconcb(pesp_conn, _client_reconnected);
    espconn_regist_disconcb(pesp_conn, _client_disconnected);
    espconn_regist_sentcb(pesp_conn, _client_sent);
}

// Called when client disconnected, as name says :D
void  _client_disconnected(void* arg) {
    struct espconn* pesp_conn = arg;
    os_printf("Client disconnected... %s:%d %d\r\n", 
            ip_as_str(pesp_conn),
            pesp_conn->proto.tcp->remote_port,
            pesp_conn->link_cnt);
}

// Called when some error appears
void  _client_reconnected(void* arg, sint8 code) {
    os_printf("Oh no!\r\n");
}

// Called when ESP recieve data
void  _client_recv(void* arg, char* data, uint16_t len) {
    struct espconn* pesp_conn = arg;

    os_printf("Recieved (%s:%d %d): %s\r\n",
            ip_as_str(pesp_conn), 
            pesp_conn->proto.tcp->remote_port,
            pesp_conn->link_cnt,
            data);

    process_request(data);
    espconn_send(pesp_conn, _nhttp->data, os_strlen(_nhttp->data));
}

// Called after packet is sent
void  _client_sent(void* arg) {
    /* First packet sent */
    struct espconn* pesp_conn = arg;

    // Called after 0\r\n (last) packet sent
    if((_nhttp->response.chunks_left == 0 && _nhttp->response.rest == 0 &&
       _nhttp->response.send_zero == 0)) {
        _nhttp->processing = 0;
        LOG_NHTTP("ELO!\r\n");
        return;
    }

    // Sec-check   
    if(_nhttp->response.uri == NULL)
       return;

    int n = 0;
    int f = 0;
    uint8_t p = 0; 
    uint32_t s = 0;
   
    os_memset(&_nhttp->data, 0, BUFFER_SIZE);

    // There is remaining chunks to send
    // Chunk is 512
    if(_nhttp->response.chunks_left != 0) {

        // Prepare size
        os_memcpy(&_nhttp->data, "200;  \r\n", 8);
        _nhttp->data[520] = '\r';
        _nhttp->data[521] = '\n';

        // Read data to buffer right after size
        f = FLASH_BASE+_nhttp->response.uri->offset + (512 * _nhttp->response.chunks_sent);        
        if(spi_flash_read(f, (uint32_t*)&_nhttp->data[8], 512) != SPI_FLASH_RESULT_OK) {
            LOG_NHTTP("Flash read error block %d\r\n", _nhttp->response.chunks_left);
            return;
        }

        LOG_NHTTP("Sending 512 chunk (%x %d %d)...\r\n", f, f, f%4);
        _nhttp->response.chunks_left--;
        _nhttp->response.chunks_sent++;

        espconn_send(pesp_conn, _nhttp->data, 522);

        // No chunks, no rest - page size is n*512 size
        if(_nhttp->response.chunks_left == 0 && _nhttp->response.rest == 0)
            _nhttp->response.send_zero = 1;
    }
    // If page smaller than 512, send data as 1 chunk
    // Or if 0 chunks left, just send rest of data
    else if((_nhttp->response.uri->size < 512 && _nhttp->response.rest != 0) ||  
            (_nhttp->response.chunks_left == 0 && _nhttp->response.rest != 0) ) {

        // Read padding
        p = _nhttp->response.uri->padding;

        // Size of file - padding bytes        
        os_memcpy(&_nhttp->data, "      \r\n", 8);
        n = os_sprintf(&_nhttp->data, "%X;", _nhttp->response.rest - p);
        // \x00 from sprintf
        _nhttp->data[n] = ' ';

        // Read data...
        f = FLASH_BASE+_nhttp->response.uri->offset + (512 * _nhttp->response.chunks_sent);
        if(spi_flash_read(f, (uint32_t*)&_nhttp->data[8], _nhttp->response.rest) != SPI_FLASH_RESULT_OK) {
            LOG_NHTTP("Flash read error rest\r\n");
        }

        // End of response..
        _nhttp->data[_nhttp->response.rest-p+8] = '\r';
        _nhttp->data[_nhttp->response.rest-p+9] = '\n';

        LOG_NHTTP("Sending rest (%x %d)...\r\n", p, p);
        espconn_send(pesp_conn, _nhttp->data, _nhttp->response.rest + 8 + (2 - p));

        _nhttp->response.rest = 0;
        _nhttp->response.send_zero = 1;
    }
    // Last packet
    else if(_nhttp->response.send_zero) {

        LOG_NHTTP("Sending zero...\r\n");
        espconn_send(pesp_conn, "0\r\n\r\n", 5);
        _nhttp->response.send_zero = 0;
    }
}
