#ifndef __GSM_SMS_H__
#define __GSM_SMS_H__ 

#include "gsm_urc.h"
#include <stdint.h>
#include "gsm.h"
#include "hardware.h"

typedef void (*gsm_sms_send_cb_t)(void *ctx, urc_type_t evt);

typedef struct {
    uint8_t  step;
    uint8_t  retry;
    uint32_t time_stamp;
    uint32_t timeout_ms;
    gsm_sms_send_cb_t on_send_done;      
    gsm_sms_send_cb_t on_send_error;  
    char number [20];
    char text [128];

} gsm_sms_send_t;


void gsm_sms_send_init(gsm_sms_send_cb_t sms_send_done_cb, gsm_sms_send_cb_t sms_send_error);
void gsm_sms_send_process(const char *sdt, const char *message);


#endif // __GSM_SMS_H__