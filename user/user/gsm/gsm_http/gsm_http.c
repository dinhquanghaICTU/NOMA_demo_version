#include "gsm_http.h"

#define TIME_OUT 10000

gsm_http_t gsm_http_ctx;

static bool is_timeout(uint32_t start_ms, uint32_t timeout_ms) {
    return (get_tick_ms() - start_ms) >= timeout_ms;
}

void gsm_http_init(gsm_http_cb_t http_get_done_cb, gsm_http_cb_t http_get_error_cb){
    gsm_http_ctx.url[0]= '\0';
    gsm_http_ctx.method [0] = '\0';
    gsm_http_ctx.body [0]= '\0';
    gsm_http_ctx.retry = 0 ;
    gsm_http_ctx.on_http_done = http_get_done_cb;
    gsm_http_ctx.on_http_error = http_get_error_cb;
    gsm_http_ctx.status[0]='\0';
    gsm_http_ctx.time_stamp = get_tick_ms();
    gsm_http_ctx.timeout_ms = 0;
    gsm_http_ctx.step = 0;    
}


void gsm_http_process(char url){
    char line [128];
    urc_t urc;
    switch (gsm_http_ctx.step)
    {
    case 0:
        send_to_debug(">>> set gprs \r\n");
        send_at_comand("AT+CGATT=1\r\n");
        gsm_http_ctx.time_stamp = get_tick_ms();
        gsm_http_ctx.step = 1;
        break;

    case 1:{
        bool handled = false;
        while (gsm_fetch_line(line, sizeof(line))) {
            log_raw_line(line);
            if (at_parse_line(line, &urc)) {
                if (urc.type == URC_OK) {
                    gsm_http_ctx.step = 2;
                    gsm_http_ctx.time_stamp = get_tick_ms();
                    handled = true;
                    break;
                } else if (urc.type == URC_ERROR) {
                    gsm_http_ctx.step = 20;
                    handled = true;
                    break;
                }
            }
        }
        if (!handled && is_timeout(gsm_http_ctx.time_stamp, TIME_OUT)) {
            send_to_debug("time out \r\n");
            gsm_http_ctx.step = 20;
        }
        break;
    }

    case 2:
        send_to_debug(">>> khai bao PDP \r\n");
        send_at_comand("AT+CGDCONT=1,\"IP\",\"v-internet\"\r\n");
        gsm_http_ctx.time_stamp = get_tick_ms();
        gsm_http_ctx.step = 3;
        break;

    case 3:{
        bool handled = false;
        while (gsm_fetch_line(line, sizeof(line))) {
            log_raw_line(line);
            if (at_parse_line(line, &urc)) {
                if (urc.type == URC_OK) {
                    gsm_http_ctx.step = 4;
                    gsm_http_ctx.time_stamp = get_tick_ms();
                    handled = true;
                    break;
                } else if (urc.type == URC_ERROR) {
                    gsm_http_ctx.step = 20;
                    handled = true;
                    break;
                }
            }
        }
        if (!handled && is_timeout(gsm_http_ctx.time_stamp, TIME_OUT)) {
            send_to_debug("time out \r\n");
            gsm_http_ctx.step = 20;
        }
        break;
    }
    case 4:
        send_to_debug(">>> activate PDP\r\n");
        send_at_comand("AT+CGACT=1,1\r\n");
        gsm_http_ctx.time_stamp = get_tick_ms();
        gsm_http_ctx.step = 5;
        break;
    case 5:{
        bool handled = false;
        while (gsm_fetch_line(line, sizeof(line))) {
            log_raw_line(line);
            if (at_parse_line(line, &urc)) {
                if (urc.type == URC_OK) {
                    gsm_http_ctx.step = 6;
                    gsm_http_ctx.time_stamp = get_tick_ms();
                    handled = true;
                    break;
                } else if (urc.type == URC_ERROR) {
                    gsm_http_ctx.step = 20;
                    handled = true;
                    break;
                }
            }
        }
        if (!handled && is_timeout(gsm_http_ctx.time_stamp, TIME_OUT)) {
            send_to_debug("time out \r\n");
            gsm_http_ctx.step = 20;
        }
        break;
    }
    case 6:
        send_to_debug(">>> khoi tao phien http \r\n");
        send_at_comand("AT+HTTPINIT\r\n");
        gsm_http_ctx.time_stamp = get_tick_ms();
        gsm_http_ctx.step = 7;
        break;

    case 7:{
        bool handled = false;
        while (gsm_fetch_line(line, sizeof(line))) {
            log_raw_line(line);
            if (at_parse_line(line, &urc)) {
                if (urc.type == URC_OK) {
                    gsm_http_ctx.step = 8;
                    gsm_http_ctx.time_stamp = get_tick_ms();
                    handled = true;
                    break;
                } else if (urc.type == URC_ERROR) {
                    gsm_http_ctx.step = 20;
                    handled = true;
                    break;
                }
            }
        }
        if (!handled && is_timeout(gsm_http_ctx.time_stamp, TIME_OUT)) {
            send_to_debug("time out \r\n");
            gsm_http_ctx.step = 20;
        }
        break;
    }

    case 8:
        send_to_debug(">>> set CID\r\n");
        send_at_comand("AT+HTTPPARA=\"CID\",1\r\n");
        gsm_http_ctx.time_stamp = get_tick_ms();
        gsm_http_ctx.step = 9;
        break;
    
    case 9:{
        bool handled = false;
        while (gsm_fetch_line(line, sizeof(line))) {
            log_raw_line(line);
            if (at_parse_line(line, &urc)) {
                if (urc.type == URC_OK) {
                    gsm_http_ctx.step = 10;
                    gsm_http_ctx.time_stamp = get_tick_ms();
                    handled = true;
                    break;
                } else if (urc.type == URC_ERROR) {
                    gsm_http_ctx.step = 20;
                    handled = true;
                    break;
                }
            }
        }
        if (!handled && is_timeout(gsm_http_ctx.time_stamp, TIME_OUT)) {
            send_to_debug("time out \r\n");
            gsm_http_ctx.step = 20;
        }
        break;
    }
    case 10:
        send_to_debug(">>> set url \r\n");
        snprintf(gsm_http_ctx.url, sizeof(gsm_http_ctx.url), "AT+HTTPPARA=\"URL\",\"%s\"\r\n",url);
        send_at_comand(gsm_http_ctx.url); 
        gsm_http_ctx.time_stamp = get_tick_ms();
        gsm_http_ctx.step = 11;    
        break;

    case 11:{
        bool handled = false;
        while (gsm_fetch_line(line, sizeof(line))) {
            log_raw_line(line);
            if (at_parse_line(line, &urc)) {
                if (urc.type == URC_OK) {
                    gsm_http_ctx.step = 12;
                    gsm_http_ctx.time_stamp = get_tick_ms();
                    handled = true;
                    break;
                } else if (urc.type == URC_ERROR) {
                    gsm_http_ctx.step = 20;
                    handled = true;
                    break;
                }
            }
        }
        if (!handled && is_timeout(gsm_http_ctx.time_stamp, TIME_OUT)) {
            send_to_debug("time out \r\n");
            gsm_http_ctx.step = 20;
        }
        break;
    }

    case 12:
        send_to_debug("");
        break;

    case 20:
        send_to_debug("loi \r\n");
        break;
    case 100:

        break;

    default:
        break;
    }

}
