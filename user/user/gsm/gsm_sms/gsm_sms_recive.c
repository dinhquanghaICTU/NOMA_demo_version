#include "gsm_sms_recive.h"
#include <stdio.h>


#define TIME_OUT 10000

gsm_sms_recive_t gsm_recive_ctx;

static bool is_timeout(uint32_t start_ms, uint32_t timeout_ms) {
    return (get_tick_ms() - start_ms) >= timeout_ms;
}


void gsm_sms_recive_init(gsm_sms_recive_cb_t sms_recive_done_cb, gsm_sms_recive_cb_t sms_recive_error_cb){
    gsm_recive_ctx.on_recive_done = sms_recive_done_cb;
    gsm_recive_ctx.on_recive_error = sms_recive_error_cb;
    gsm_recive_ctx.step = 0;
    gsm_recive_ctx.time_stamp = get_tick_ms();
    gsm_recive_ctx.timeout_ms = 0;
    gsm_recive_ctx.retry = 0;
    gsm_recive_ctx.number[0] = '\0';
    gsm_recive_ctx.text[0] = '\0';
    gsm_recive_ctx.sms_index = 0;

    send_at_comand("AT+CNMI=2,1,0,0,0\r\n");
}

void gsm_sms_recive_process(void){
    urc_t urc;
    char line[128];

    if (gsm_recive_ctx.step == 0) {
        while (gsm_fetch_line(line, sizeof(line))) {
            if (at_parse_line(line, &urc)) {
                if (urc.type == URC_CMTI) {
                    gsm_recive_ctx.sms_index = urc.v1;
                    gsm_recive_ctx.step = 1;
                    break;
                }
            }
        }
    }
    
    switch (gsm_recive_ctx.step) {
    case 0:
        break;
        
    case 1:
        if (gsm_recive_ctx.sms_index > 0) {
            char cmd[32];
            snprintf(cmd, sizeof(cmd), "AT+CMGR=%d\r\n", gsm_recive_ctx.sms_index);
            send_at_comand(cmd);
            gsm_recive_ctx.time_stamp = get_tick_ms();
            gsm_recive_ctx.step = 2;
        }
        break;
        
    case 2: {
        bool found_cmgr = false;
        bool got_content = false;
        
        while (gsm_fetch_line(line, sizeof(line))) {
            log_raw_line(line);
            
            if (at_parse_line(line, &urc)) {
                if (urc.type == URC_CMGR) {
                    strncpy(gsm_recive_ctx.number, urc.text, sizeof(gsm_recive_ctx.number) - 1);
                    gsm_recive_ctx.number[sizeof(gsm_recive_ctx.number) - 1] = '\0';
                    found_cmgr = true;
                } else if (urc.type == URC_OK && found_cmgr) {
                    gsm_recive_ctx.step = 3;
                    break;
                } else if (urc.type == URC_ERROR) {
                    gsm_recive_ctx.step = 20;
                    break;
                }
            } else if (found_cmgr && !got_content && line[0] != '\0' && line[0] != '\r' && line[0] != '\n') {
                strncpy(gsm_recive_ctx.text, line, sizeof(gsm_recive_ctx.text) - 1);
                gsm_recive_ctx.text[sizeof(gsm_recive_ctx.text) - 1] = '\0';
                got_content = true;
            }
        }
        
        if (is_timeout(gsm_recive_ctx.time_stamp, TIME_OUT)) {
            gsm_recive_ctx.step = 20;
        }
        break;
    }
    
    case 3:
        if (gsm_recive_ctx.sms_index > 0) {
            char cmd[32];
            snprintf(cmd, sizeof(cmd), "AT+CMGD=%d\r\n", gsm_recive_ctx.sms_index);
            send_at_comand(cmd);
            gsm_recive_ctx.time_stamp = get_tick_ms();
            gsm_recive_ctx.step = 4;
        } else {
            gsm_recive_ctx.step = 100;
        }
        break;
        
    case 4: {
        bool handled = false;
        while (gsm_fetch_line(line, sizeof(line))) {
            log_raw_line(line);
            if (at_parse_line(line, &urc)) {
                if (urc.type == URC_OK) {
                    gsm_recive_ctx.step = 100;
                    handled = true;
                    break;
                } else if (urc.type == URC_ERROR) {
                    gsm_recive_ctx.step = 20;
                    handled = true;
                    break;
                }
            }
        }
        if (!handled && is_timeout(gsm_recive_ctx.time_stamp, TIME_OUT)) {
            gsm_recive_ctx.step = 100;
        }
        break;
    }
    
    case 20:
        gsm_recive_ctx.retry++;
        if (gsm_recive_ctx.retry > 3) {
            send_to_debug("that bai da retry 3 \r\n");
        } else {
            send_to_debug("retry...\r\n");
            gsm_recive_ctx.step = 0;
            gsm_recive_ctx.time_stamp = get_tick_ms();
        }
        break;
        
        
    case 100:
        // if (gsm_recive_ctx.on_recive_done) {
        //     gsm_recive_ctx.on_recive_done(NULL, URC_OK);
        // }
        // gsm_recive_ctx.step = 0;
        // gsm_recive_ctx.sms_index = 0;
        break;
        
    default:
        break;
    }
}
