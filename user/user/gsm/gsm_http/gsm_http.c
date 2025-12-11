#include "gsm_http.h"

#define TIME_OUT 10000

gsm_sms_recive_t gsm_recive_ctx;

static bool is_timeout(uint32_t start_ms, uint32_t timeout_ms) {
    return (get_tick_ms() - start_ms) >= timeout_ms;
}



void gsm_http_init(gsm_http_cb_t http_get_done_cb, gsm_http_cb_t http_get_error_cb){
    
}


void gsm_http_process(void){

}
