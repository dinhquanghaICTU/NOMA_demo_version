#ifndef __GSM_SMS_RECIVE_H__
#define __GSM_SMS_RECIVE_H__
#include <stdint.h>
#include "gsm_urc.h"
#include "gsm.h"
#include "hardware.h"

typedef void (*gsm_sms_recive_cb_t)(void *ctx, urc_type_t evt);

typedef struct {
    uint8_t  step;
    uint8_t  retry;
    uint32_t time_stamp;
    uint32_t timeout_ms;
    gsm_sms_recive_cb_t on_recive_done;      
    gsm_sms_recive_cb_t on_recive_error;
    char number[20];     
    char text[160];     
    uint8_t sms_index;  
} gsm_sms_recive_t;

void gsm_sms_recive_init(gsm_sms_recive_cb_t sms_recive_done_cb, gsm_sms_recive_cb_t sms_recive_error);
void gsm_sms_recive_process(void);

#endif // __GSM_SMS_RECIVE_H__