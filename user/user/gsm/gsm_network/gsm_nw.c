#include "gsm_nw.h"
#include "gsm.h"
#include <string.h>

#define TIME_OUT 10000

gsm_net_ctx_t gsm_nw_ctx;

static bool is_timeout(uint32_t start_ms, uint32_t timeout_ms) {
    return (get_tick_ms() - start_ms) >= timeout_ms;
}



void gsm_nw_init(gsm_net_cb_t ready_cb, gsm_net_cb_t error_cb){
    gsm_nw_ctx.step = 0;
    gsm_nw_ctx.on_ready = ready_cb;
    gsm_nw_ctx.on_error = error_cb;
    gsm_nw_ctx.retry = 0;
    gsm_nw_ctx.time_stamp = get_tick_ms();
    gsm_nw_ctx.timeout_ms= 0;
}

void gsm_nw_process(void){
    urc_t urc;
    char line[128];
    switch (gsm_nw_ctx.step)
    {
    case 0:
        gsm_nw_ctx.retry = 0;
        send_to_debug(">>> check AT \r\n");
        send_at_comand("AT\r\n");
        gsm_nw_ctx.time_stamp = get_tick_ms(); 
        gsm_nw_ctx.step = 1;
        break;
    case 1: {
        bool handled = false;
        while (gsm_fetch_line(line, sizeof(line))) {
            log_raw_line(line);
            if (at_parse_line(line, &urc)) {
                if (urc.type == URC_OK) {
                    gsm_nw_ctx.step = 2;
                    gsm_nw_ctx.time_stamp = get_tick_ms();
                    handled = true;
                    break;
                } else if (urc.type == URC_ERROR) {
                    gsm_nw_ctx.step = 20;
                    handled = true;
                    break;
                }
            }
        }
        if (!handled && is_timeout(gsm_nw_ctx.time_stamp, TIME_OUT)) {
            send_to_debug("time out \r\n");
            gsm_nw_ctx.step = 20;
        }
        break;
    }
    case 2:
        send_to_debug(">>> check trang thai sim \r\n");
        send_at_comand("AT+CPIN?\r\n");
        gsm_nw_ctx.time_stamp = get_tick_ms(); 
        gsm_nw_ctx.step = 3;
        break;
    case 3: {
        bool handled = false;
        while (gsm_fetch_line(line, sizeof(line))) {
            log_raw_line(line);
            if (at_parse_line(line, &urc)) {
                if (urc.type == URC_CPIN_READY) {
                    gsm_nw_ctx.step = 4;
                    gsm_nw_ctx.time_stamp = get_tick_ms();
                    handled = true;
                    break;
                } else if (urc.type == URC_ERROR) {
                    gsm_nw_ctx.step = 20;
                    handled = true;
                    break;
                }
            }
        }
        if (!handled && is_timeout(gsm_nw_ctx.time_stamp, TIME_OUT)) {
            send_to_debug("time out \r\n");
            gsm_nw_ctx.step = 20;
        }
        break;
    }
    
    case 4:
        send_to_debug(">>> check trang thai dang ki mang \r\n");
        send_at_comand("AT+CREG?\r\n");
        gsm_nw_ctx.time_stamp = get_tick_ms(); 
        gsm_nw_ctx.step = 5;
        break;
    case 5: {
        bool handled = false;
        while (gsm_fetch_line(line, sizeof(line))) {
            log_raw_line(line);
            if (at_parse_line(line, &urc)) {
                if (urc.type == URC_CREG) {
                    gsm_nw_ctx.step = 6;
                    gsm_nw_ctx.time_stamp = get_tick_ms();
                    handled = true;
                    break;
                } else if (urc.type == URC_ERROR) {
                    gsm_nw_ctx.step = 20;
                    handled = true;
                    break;
                }
            }
        }
        if (!handled && is_timeout(gsm_nw_ctx.time_stamp, TIME_OUT)) {
            send_to_debug("time out \r\n");
            gsm_nw_ctx.step = 20;
        }
        break;
    }
    case 6:
        send_to_debug(">>> bat che do sms text\r\n");
        send_at_comand("AT+CMGF=1\r\n");
        gsm_nw_ctx.time_stamp = get_tick_ms(); 
        gsm_nw_ctx.step = 7;
        break;
    case 7: {
        bool handled = false;
        while (gsm_fetch_line(line, sizeof(line))) {
            log_raw_line(line);
            if (at_parse_line(line, &urc)) {
                if (urc.type == URC_OK) {
                    gsm_nw_ctx.step = 8;
                    gsm_nw_ctx.time_stamp = get_tick_ms();
                    handled = true;
                    break;
                } else if (urc.type == URC_ERROR) {
                    gsm_nw_ctx.step = 20;
                    handled = true;
                    break;
                }
            }
        }
        if (!handled && is_timeout(gsm_nw_ctx.time_stamp, TIME_OUT)) {
            send_to_debug("time out \r\n");
            gsm_nw_ctx.step = 20;
        }
        break;
    }
    case 8:
        send_to_debug(">>> charset GSM 7-bit\r\n");
        send_at_comand("AT+CSCS=\"GSM\"\r\n");
        gsm_nw_ctx.time_stamp = get_tick_ms(); 
        gsm_nw_ctx.step = 9;
        break;
    case 9: {
        bool handled = false;
        while (gsm_fetch_line(line, sizeof(line))) {
            log_raw_line(line);
            if (at_parse_line(line, &urc)) {
                if (urc.type == URC_OK) {
                    gsm_nw_ctx.step = 10;
                    gsm_nw_ctx.time_stamp = get_tick_ms();
                    handled = true;
                    break;
                } else if (urc.type == URC_ERROR) {
                    gsm_nw_ctx.step = 20;
                    handled = true;
                    break;
                }
            }
        }
        if (!handled && is_timeout(gsm_nw_ctx.time_stamp, TIME_OUT)) {
            send_to_debug("time out \r\n");
            gsm_nw_ctx.step = 20;
        }
        break;
    }

    case 10:
        send_to_debug(">>> chon bo nho\r\n");
        send_at_comand("AT+CPMS=\"ME\",\"ME\",\"ME\"\r\n");
        gsm_nw_ctx.time_stamp = get_tick_ms(); 
        gsm_nw_ctx.step = 11;
        break;
    case 11: {
        bool handled = false;
        while (gsm_fetch_line(line, sizeof(line))) {
            log_raw_line(line);
            if (at_parse_line(line, &urc)) {
                if (urc.type == URC_OK) {
                    gsm_nw_ctx.step = 12;
                    gsm_nw_ctx.time_stamp = get_tick_ms();
                    handled = true;
                    break;
                } else if (urc.type == URC_ERROR) {
                    gsm_nw_ctx.step = 20;
                    handled = true;
                    break;
                }
            }
        }
        if (!handled && is_timeout(gsm_nw_ctx.time_stamp, TIME_OUT)) {
            send_to_debug("time out \r\n");
            gsm_nw_ctx.step = 20;
        }
        break;
    }

    case 12:
        send_to_debug(">>> bat loi chi tiet\r\n");
        send_at_comand("AT+CMEE=1\r\n");
        gsm_nw_ctx.time_stamp = get_tick_ms(); 
        gsm_nw_ctx.step = 13;
        break;
    case 13: {
        bool handled = false;
        while (gsm_fetch_line(line, sizeof(line))) {
            log_raw_line(line);
            if (at_parse_line(line, &urc)) {
                if (urc.type == URC_OK) {
                    gsm_nw_ctx.step = 100;
                    gsm_nw_ctx.time_stamp = get_tick_ms();
                    handled = true;
                    break;
                } else if (urc.type == URC_ERROR) {
                    gsm_nw_ctx.step = 20;
                    handled = true;
                    break;
                }
            }
        }
        if (!handled && is_timeout(gsm_nw_ctx.time_stamp, TIME_OUT)) {
            send_to_debug("time out \r\n");
            gsm_nw_ctx.step = 20;
        }
        break;
    }
    case 20:
        gsm_nw_ctx.retry++;
        if (gsm_nw_ctx.retry > 3) {
            send_to_debug("that bai da retry 3 \r\n");
        } else {
            send_to_debug("retry...\r\n");
            gsm_nw_ctx.step = 0;
            gsm_nw_ctx.time_stamp = get_tick_ms();
        }
        break;

    case 100:
        // done base net
        break;
    default:
        break;
    }
}  



bool gsm_nw_is_ready(void){
    if (gsm_nw_ctx.step == 100){
        return true;
    }
    return false;
}
