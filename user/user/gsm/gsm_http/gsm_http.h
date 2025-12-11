#ifndef __GSM_HTTP_H__
#define __GSM_HTTP_H__

#include <stdint.h>
#include "gsm_urc.h"
#include "hardware.h"
#include "gsm_urc.h"
#include "gsm.h"


typedef void (*gsm_http_cb_t)(void *ctx, urc_type_t evt);

typedef struct {
    uint8_t  step;
    uint8_t  retry;
    uint32_t time_stamp;
    uint32_t timeout_ms;
    gsm_http_cb_t on_http_done;      
    gsm_http_cb_t on_http_error;
    char url[20];     
    char method [15];
    char status [10];
    char body [256];
} gsm_http_t;

void gsm_http_init(gsm_http_cb_t http_get_done_cb, gsm_http_cb_t http_get_error_cb);
void gsm_http_process(void);


#endif // __GSM_HTTP_H__
