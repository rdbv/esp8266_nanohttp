#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "user_interface.h"
#include "espconn.h"
#include "nanohttp.h"


LOCAL Nano_Http nhttp;

/*
    gpio_output_set(BIT5, 0, BIT5, 0);
    gpio_output_set(0, BIT5, BIT5, 0);
*/
/*
#define SERVER_LOCAL_PORT 80

LOCAL struct espconn esp_conn;
LOCAL esp_tcp esptcp;

LOCAL Nano_Http nhttp;

LOCAL char* ICACHE_FLASH_ATTR ip_as_str(struct espconn* conn) 
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

LOCAL void ICACHE_FLASH_ATTR
tcp_server_sent_cb(void *arg) 
{
    struct espconn *pespconn = arg;
    os_printf("TCP sent cb %s:%d \r\n", ip_as_str(pespconn), pespconn->proto.tcp->remote_port);
    espconn_send(pespconn, "POWTURKA\r\n", 10);
}

// cbrecv
LOCAL void ICACHE_FLASH_ATTR
tcp_server_recv_cb(void *arg, char *pusrdata, unsigned short length)
{
    uint32_t tlen;
    struct espconn *pespconn = arg;
    //process_request(&nhttp, (char*) pusrdata, &tlen);
    //espconn_send(pespconn, get_response(), tlen);
    espconn_send(pespconn, "SIEMA\r\n", 7);
}

LOCAL void ICACHE_FLASH_ATTR
tcp_server_discon_cb(void *arg) 
{
    struct espconn* pesp_conn = arg;
    os_printf("Client disconnected... %s:%d\r\n", ip_as_str(pesp_conn), 
            pesp_conn->proto.tcp->remote_port);
}

LOCAL void ICACHE_FLASH_ATTR
tcp_server_recon_cb(void *arg, sint8 err)
{
    os_printf("reconnect callback, err code %d !!! \r\n", err);
}

LOCAL void ICACHE_FLASH_ATTR
tcp_server_listen(void *arg)
{
    struct espconn *pesp_conn = arg;
    os_printf("Client connected... %s:%d %d\r\n", 
            ip_as_str(pesp_conn),
            pesp_conn->proto.tcp->remote_port,
            pesp_conn->link_cnt);


    espconn_regist_recvcb(pesp_conn, tcp_server_recv_cb);
    espconn_regist_reconcb(pesp_conn, tcp_server_recon_cb);
    espconn_regist_disconcb(pesp_conn, tcp_server_discon_cb);
    
    espconn_regist_sentcb(pesp_conn, tcp_server_sent_cb);
}

void ICACHE_FLASH_ATTR
user_tcpserver_init(uint32 port)
{
    //os_delay_us(2000000);

    //init_nanohttp(&nhttp);    
    esp_conn.type = ESPCONN_TCP;
    esp_conn.state = ESPCONN_NONE;
    esp_conn.proto.tcp = &esptcp;
    esp_conn.proto.tcp->local_port = port;
    espconn_regist_connectcb(&esp_conn, tcp_server_listen);

    sint8 ret = espconn_accept(&esp_conn);
    espconn_regist_time(&esp_conn, 25, 0);

    os_printf("espconn_accept [%d] !!! \r\n", ret);

}
*/

os_timer_t timer0;

static void ICACHE_FLASH_ATTR
loop()
{
    char v = 0b10101010;

    os_printf("Zyje!\r\n");

    for(char i=0;i<8;++i) {
        if( v & (1 << i))
            gpio_output_set(BIT12, 0, BIT12, 0);
        else
            gpio_output_set(0, BIT12, BIT12, 0);

        gpio_output_set(BIT13, 0, BIT13, 0);
        gpio_output_set(0, BIT13, BIT13, 0);
    }
    gpio_output_set(BIT14, 0, BIT14, 0);
    gpio_output_set(0, BIT14, BIT14, 0);
    gpio_output_set(0, BIT12, BIT12, 0);
}

/* Just connect to wi-fi */
void ICACHE_FLASH_ATTR
connect_wifi() 
{
    char ssid[32] = "internet";
    char password[64] = "kanapkazesrem3";

    // Init data
    struct station_config s_conf;
    s_conf.bssid_set = 0;
    os_memcpy(&s_conf.ssid, ssid, 32);
    os_memcpy(&s_conf.password, password, 64);

    // Connect ( saved in flash? )
    wifi_station_set_config(&s_conf);

    os_printf("Connected...\r\n");
    init_nanohttp(&nhttp);

    os_timer_setfn(&timer0, loop, NULL);
    os_timer_arm(&timer0, 1000, true);
}

// cmain
void ICACHE_FLASH_ATTR
user_init() 
{
    wifi_set_opmode(STATIONAP_MODE);

    gpio_init();
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5);

    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_GPIO13);
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12);
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, FUNC_GPIO14);

    uart_div_modify(0, UART_CLK_FREQ / 115200);

    connect_wifi();
}
