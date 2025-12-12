#ifndef __GSM_HTTP_H__
#define __GSM_HTTP_H__

#include <stdint.h>
#include "gsm_urc.h"
#include "hardware.h"
#include "gsm_urc.h"
#include "gsm.h"



typedef void (*gsm_http_cb_t)(void *ctx, urc_type_t evt);

typedef enum {
    HTTP_PHASE_GPRS = 0,
    HTTP_PHASE_HTTP_INIT,
    HTTP_PHASE_SET_URL,
    HTTP_PHASE_SEND_REQUEST,
    HTTP_PHASE_READ_DATA,
    HTTP_PHASE_CHUNK,
    HTTP_PHASE_DONE,
    HTTP_PHASE_ERROR
}http_phase_t; 


typedef struct {
    uint8_t  step;
    uint8_t  retry;
    uint32_t time_stamp;
    uint32_t timeout_ms;
    gsm_http_cb_t on_http_done;
    gsm_http_cb_t on_http_error;

    char     url[256];   
    uint8_t  method;      
    uint16_t status;      
    uint16_t datalen;     
    char     body[128];  

    uint32_t offset;
    uint32_t chunk_len;
    uint32_t total_size; 
    http_phase_t http_phase;
} gsm_http_t;

void gsm_http_init(gsm_http_cb_t http_get_done_cb, gsm_http_cb_t http_get_error_cb);
void gsm_http_process(const char *url);


#endif // __GSM_HTTP_H__
