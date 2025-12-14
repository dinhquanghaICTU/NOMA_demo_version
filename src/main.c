#include "stm32f10x.h"
#include "hardware.h"
#include "uart.h"
#include "gsm.h"
#include "debug.h"
#include "gsm_network/gsm_nw.h"
#include "gsm_sms/gsm_sms_send.h"
#include "gsm_sms/gsm_sms_recive.h"
#include "gsm_http/gsm_http.h"
#include "led/led.h"
#include "gsm_mqtt/gsm_mqtt.h"

int main(void) {
    rcc_config();
    systick_config();
    gsm_gpio_init();
    uart_init();
    gsm_nw_init(NULL, NULL);
    gsm_sms_send_init(NULL, NULL);
    // while(!gsm_power_on()) {}
    gsm_hardware_init();
    gsm_sms_recive_init(NULL,NULL);
    gsm_http_init(NULL, NULL);

    led_init();
    gsm_mqtt_init(NULL,NULL);

    while(1) {
        gsm_hardware_process_urc();
         gsm_nw_process();
        if(gsm_nw_is_ready()){
//             gsm_http_process("http://18.141.139.170:8080/raw/bling_led.bin");
           gsm_http_process("http://18.141.139.170:8080/raw/2000.bin");

        }
//
//        	turn_on_led();
//            delay_ms(1000);
//            turn_off_led();
//            delay_ms(1000);
//         if(gsm_nw_is_ready()){
//             gsm_http_process("http://raw.githubusercontent.com/dinhquanghaICTU/file_bin_test_OTA/main/bling_led.binary");
//         }
        

         delay_ms(400);
    }
}
