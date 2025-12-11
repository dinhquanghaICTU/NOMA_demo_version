#ifndef __GSM_NW_H__
#define __GSM_NW_H__

#include <stdint.h>
#include "gsm_urc.h"
#include "hardware.h"
#include "gsm.h"

typedef void (*gsm_net_cb_t)(void *ctx, urc_type_t evt);

typedef struct {
    uint8_t  step;
    uint8_t  retry;
    uint32_t time_stamp;
    uint32_t timeout_ms;
    gsm_net_cb_t on_ready;      
    gsm_net_cb_t on_error;  
} gsm_net_ctx_t;



void gsm_nw_init(gsm_net_cb_t ready_cb, gsm_net_cb_t error_cb);
void gsm_nw_process(void);  
bool gsm_nw_is_ready(void);
bool gsm_nw_fetch_line(char *buf, uint16_t len);


#endif // __GSM_NW_H__