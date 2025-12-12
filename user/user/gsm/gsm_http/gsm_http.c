#include "gsm_http.h"
#include <string.h>
#include <stdio.h>

#define TIME_OUT 10000

gsm_http_t gsm_http_ctx;
char line[256];
urc_t urc;

static bool is_timeout(uint32_t start_ms, uint32_t timeout_ms) {
    return (get_tick_ms() - start_ms) >= timeout_ms;
}

void gsm_http_init(gsm_http_cb_t http_get_done_cb, gsm_http_cb_t http_get_error_cb){
    gsm_http_ctx.url[0] = '\0';
    gsm_http_ctx.method = 0;
    gsm_http_ctx.body[0] = '\0';
    gsm_http_ctx.retry = 0;
    gsm_http_ctx.on_http_done = http_get_done_cb;
    gsm_http_ctx.on_http_error = http_get_error_cb;
    gsm_http_ctx.status = 0;
    gsm_http_ctx.datalen = 0;  
    gsm_http_ctx.time_stamp = get_tick_ms();
    gsm_http_ctx.timeout_ms = 0;
    gsm_http_ctx.step = 0;
    gsm_http_ctx.chunk_len = 512;
    gsm_http_ctx.offset = 0;
    gsm_http_ctx.total_size = 0;
    gsm_http_ctx.http_phase = HTTP_PHASE_GPRS; 
}

bool http_phase_init(){
    switch (gsm_http_ctx.step)
    {
    case 0:
        send_to_debug(">>> xoa sesion cu\r\n");
        send_at_comand("AT+HTTPTERM\r\n");
        gsm_http_ctx.time_stamp = get_tick_ms();
        gsm_http_ctx.step = 1;
        return true;
        
    case 1:{
        bool handled = false;
        while (gsm_fetch_line(line, sizeof(line))) {
            log_raw_line(line);
            if (at_parse_line(line, &urc)) {
                if (urc.type == URC_OK || urc.type == URC_ERROR) {
                    gsm_http_ctx.step = 2;
                    gsm_http_ctx.time_stamp = get_tick_ms();
                    handled = true;
                    delete_line(line);
                    break;
                }
            }
            delete_line(line);
        }
        if (!handled && is_timeout(gsm_http_ctx.time_stamp, TIME_OUT)) {
            gsm_http_ctx.step = 2;
        }
        return true;
    }
    
    case 2:
        send_to_debug(">>> khoi tao  http \r\n");
        send_at_comand("AT+HTTPINIT\r\n");
        gsm_http_ctx.time_stamp = get_tick_ms();
        gsm_http_ctx.step = 3;
        return true;

    case 3:{
        bool handled = false;
        while (gsm_fetch_line(line, sizeof(line))) {
            log_raw_line(line);
            if (at_parse_line(line, &urc)) {
                if (urc.type == URC_OK) {
                    while (gsm_fetch_line(line, sizeof(line))) {
                        log_raw_line(line);
                        delete_line(line);
                    }
                    gsm_http_ctx.http_phase = HTTP_PHASE_SET_URL;
                    gsm_http_ctx.step = 0;
                    delete_line(line);
                    return false;
                } else if (urc.type == URC_ERROR) {
                    send_to_debug(">>> HTTPINIT ERROR\r\n");
                    gsm_http_ctx.http_phase = HTTP_PHASE_ERROR;
                    gsm_http_ctx.step = 0;
                    delete_line(line);
                    return false;
                }
            }
            delete_line(line);
        }
        if (!handled && is_timeout(gsm_http_ctx.time_stamp, TIME_OUT)) {
            send_to_debug("time out HTTPINIT\r\n");
            gsm_http_ctx.http_phase = HTTP_PHASE_ERROR;
            gsm_http_ctx.step = 0;
            return false;
        }
        return true;
    }

    default:
        return true;
    }
}


bool http_phase_set_url(){
    switch (gsm_http_ctx.step)
    {
    case 0:{
        char cmd[512];
        send_to_debug(">>> set url \r\n");
        log_raw_line(gsm_http_ctx.url);
        snprintf(cmd, sizeof(cmd), "AT+HTTPPARA=\"URL\",\"%s\"\r\n", gsm_http_ctx.url);
        log_raw_line(cmd);  
        send_at_comand(cmd);  
        gsm_http_ctx.time_stamp = get_tick_ms();
        gsm_http_ctx.step = 1;    
        return true;
    }
    
    case 1:{
        bool handled = false;
        while (gsm_fetch_line(line, sizeof(line))) {
            log_raw_line(line);
            if (at_parse_line(line, &urc)) {
                if (urc.type == URC_OK) {
                    while (gsm_fetch_line(line, sizeof(line))) {
                        log_raw_line(line);
                        delete_line(line);
                    }
                    gsm_http_ctx.http_phase = HTTP_PHASE_SEND_REQUEST;
                    gsm_http_ctx.step = 0;
                    delete_line(line);
                    return false;
                } else if (urc.type == URC_ERROR) {
                    send_to_debug(">>> Set URL ERROR\r\n");
                    gsm_http_ctx.http_phase = HTTP_PHASE_ERROR;
                    gsm_http_ctx.step = 0;
                    delete_line(line);
                    return false;
                }
            }
            delete_line(line);
        }
        if (!handled && is_timeout(gsm_http_ctx.time_stamp, TIME_OUT)) {
            send_to_debug("time out Set URL\r\n");
            gsm_http_ctx.http_phase = HTTP_PHASE_ERROR;
            gsm_http_ctx.step = 0;
            return false;
        }
        return true;
    }
    
    default:
        return true;
    }
}

void gsm_http_process(const char *url){
    if (url && url[0] != '\0' && 
        gsm_http_ctx.http_phase == HTTP_PHASE_GPRS && 
        gsm_http_ctx.step == 0) {
        strncpy(gsm_http_ctx.url, url, sizeof(gsm_http_ctx.url) - 1);
        gsm_http_ctx.url[sizeof(gsm_http_ctx.url) - 1] = '\0';
    }
    
    switch (gsm_http_ctx.http_phase)
    {
    case HTTP_PHASE_GPRS:
        phase_gprs();
        break;

    case HTTP_PHASE_HTTP_INIT:
        http_phase_init();
        break;
        
    case HTTP_PHASE_SET_URL:
        http_phase_set_url();
        break;
        
    case HTTP_PHASE_SEND_REQUEST:
    case HTTP_PHASE_READ_DATA:
    case HTTP_PHASE_CHUNK:
    case HTTP_PHASE_DONE:
    case HTTP_PHASE_ERROR:
    default:
        break;
    }
}


