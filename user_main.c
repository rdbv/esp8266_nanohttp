#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "user_interface.h"
#include "espconn.h"
#include "nanohttp.h"


LOCAL Nano_Http nhttp;


/* Just connect to wi-fi */
void ICACHE_FLASH_ATTR
connect_wifi() 
{
    char ssid[32] = "qnet";
    char password[64] = "rakietowekapcie";

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
