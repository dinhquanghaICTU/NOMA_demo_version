#include "gsm_sms_send.h"
#include <string.h>
#include <stdio.h>

#define TIME_OUT 10000

gsm_sms_send_t gsm_send_ctx;

static bool is_timeout(uint32_t start_ms, uint32_t timeout_ms) {
    return (get_tick_ms() - start_ms) >= timeout_ms;
}

void gsm_sms_send_init(gsm_sms_send_cb_t sms_send_done_cb, gsm_sms_send_cb_t sms_send_error_cb){
    gsm_send_ctx.step = 0;
    gsm_send_ctx.retry = 0; 
    gsm_send_ctx.time_stamp = get_tick_ms();
    gsm_send_ctx.timeout_ms =0;
    gsm_send_ctx.on_send_done = sms_send_done_cb;
    gsm_send_ctx.on_send_error = sms_send_error_cb;
    gsm_send_ctx.number[0] =  '\0';
    gsm_send_ctx.text[0] =  '\0';
}


void gsm_sms_send_process(const char * sdt, const char *message){
    urc_t urc;
    char line[128];

    switch (gsm_send_ctx.step)
    {
    case 0:
        send_to_debug(">>> chon kieu text mode\r\n");
        send_at_comand("AT+CMGF=1\r\n");
        gsm_send_ctx.time_stamp = get_tick_ms();
        gsm_send_ctx.step = 1;
        break;
    case 1: {
        bool handled = false;
        while (gsm_fetch_line(line, sizeof(line))) {
            log_raw_line(line);
            if (at_parse_line(line, &urc)) {
                if (urc.type == URC_OK) {
                    gsm_send_ctx.step = 2;
                    gsm_send_ctx.time_stamp = get_tick_ms();
                    handled = true;
                    break;
                } else if (urc.type == URC_ERROR) {
                    gsm_send_ctx.step = 20;
                    handled = true;
                    break;
                }
            }
        }
        if (!handled && is_timeout(gsm_send_ctx.time_stamp, TIME_OUT)) {
            send_to_debug("time out \r\n");
            gsm_send_ctx.step = 20;
        }
        break;
    }

    case 2:
        if (!sdt || !message) {
            gsm_send_ctx.step = 20;
            break;
        }
        send_to_debug(">>> them so dien thoai\r\n");
        char cmd[64];

        strncpy(gsm_send_ctx.number, sdt, sizeof(gsm_send_ctx.number) - 1);
        gsm_send_ctx.number[sizeof(gsm_send_ctx.number) - 1] = '\0';
        
        strncpy(gsm_send_ctx.text, message, sizeof(gsm_send_ctx.text) - 1);
        gsm_send_ctx.text[sizeof(gsm_send_ctx.text) - 1] = '\0';

        snprintf(cmd, sizeof(cmd), "AT+CMGS=\"%s\"\r\n", gsm_send_ctx.number);
        send_at_comand(cmd);
        gsm_send_ctx.time_stamp = get_tick_ms();
        gsm_send_ctx.step = 3;  
        break;

    case 3: {
        bool found_prompt = false;
        while (gsm_fetch_line(line, sizeof(line))) {
            log_raw_line(line);
            if (strstr(line, ">") != NULL) {
                gsm_send_ctx.step = 4;  
                gsm_send_ctx.time_stamp = get_tick_ms();
                found_prompt = true;
                break;
            }
        }
        if (!found_prompt && is_timeout(gsm_send_ctx.time_stamp, TIME_OUT)) {
            gsm_send_ctx.step = 20; 
        }
        break;
    }

    case 4:

        send_at_comand(gsm_send_ctx.text);
        gsm_hardware_send_byte(0x1A);  
        gsm_send_ctx.step = 5;
        gsm_send_ctx.time_stamp = get_tick_ms();
        break;

    case 5: {
        bool handled = false;
        while (gsm_fetch_line(line, sizeof(line))) {
            log_raw_line(line);
            if (strstr(line, "+CMGS:") != NULL) {
                continue;
            }
            if (at_parse_line(line, &urc)) {
                if (urc.type == URC_OK) {
                    gsm_send_ctx.step = 100;
                    handled = true;
                    break;
                } else if (urc.type == URC_ERROR) {
                    gsm_send_ctx.step = 20;
                    handled = true;
                    break;
                }
            }
        }
        if (!handled && is_timeout(gsm_send_ctx.time_stamp, TIME_OUT)) {
            send_to_debug("time out \r\n");
            gsm_send_ctx.step = 20;
        }
        break;
    }

    case 20:
        gsm_send_ctx.retry++;
        if (gsm_send_ctx.retry > 3) {
            send_to_debug("that bai da retry 3 \r\n");
        } else {
            send_to_debug("retry...\r\n");
            gsm_send_ctx.step = 0;
            gsm_send_ctx.time_stamp = get_tick_ms();
        }
        break;
    case 100:

        // gsm_send_ctx.step = 0;
        // gsm_send_ctx.retry = 0;
        // if (gsm_send_ctx.on_send_done) {
        //     gsm_send_ctx.on_send_done(NULL, URC_OK);
        // }
        break;
    default:
        break;
    }
}