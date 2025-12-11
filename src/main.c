#include "stm32f10x.h"
#include "hardware.h"
#include "uart.h"
#include "gsm.h"
#include "debug.h"
#include "gsm_network/gsm_nw.h"
#include "gsm_sms/gsm_sms_send.h"
#include "gsm_sms/gsm_sms_recive.h"

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
    
    while(1) {
        gsm_hardware_process_urc();
        gsm_nw_process();
        if(gsm_nw_is_ready()){
        	gsm_sms_recive_process();
        }
        delay_ms(1000);        
    }
}
