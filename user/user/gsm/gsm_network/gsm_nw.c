#include "gsm_nw.h"
#include "gsm.h"
#include <string.h>

#define TIME_OUT 10000

gsm_net_ctx_t gsm_nw_ctx;
static char line[128];
static urc_t urc;

static bool is_timeout(uint32_t start_ms, uint32_t timeout_ms) {
    return (get_tick_ms() - start_ms) >= timeout_ms;
}

void gsm_nw_init(gsm_net_cb_t ready_cb, gsm_net_cb_t error_cb){
    memset(&gsm_nw_ctx, 0, sizeof(gsm_nw_ctx));
    gsm_nw_ctx.on_ready = ready_cb;
    gsm_nw_ctx.on_error = error_cb;
    gsm_nw_ctx.state = GSM_NW_BASIC;
    gsm_nw_ctx.time_stamp = get_tick_ms();
}


bool gsm_nw_phase_basic(void) {
    switch (gsm_nw_ctx.step) {
    case 0:
        gsm_nw_ctx.retry = 0;
        send_to_debug(">>> check AT\r\n");
        send_at_comand("AT\r\n");
        gsm_nw_ctx.time_stamp = get_tick_ms();
        gsm_nw_ctx.step = 1;
        return false;

    case 1: { 
        bool handled = false;
        while (gsm_fetch_line(line, sizeof(line))) {
            log_raw_line(line);
            if (at_parse_line(line, &urc)) {
                if (urc.type == URC_OK) {
                    gsm_nw_ctx.step = 2;
                    gsm_nw_ctx.time_stamp = get_tick_ms();
                    handled = true;
                    delete_line(line);
                    break;
                } else if (urc.type == URC_ERROR) {
                    gsm_nw_ctx.step = 20;
                    handled = true;
                    delete_line(line);
                    break;
                }
            }
            delete_line(line);
        }
        if (!handled && is_timeout(gsm_nw_ctx.time_stamp, TIME_OUT)) {
            send_to_debug("time out AT\r\n");
            gsm_nw_ctx.step = 20;
        }
        return false;
    }

    case 2:
        send_to_debug(">>> check SIM\r\n");
        send_at_comand("AT+CPIN?\r\n");
        gsm_nw_ctx.time_stamp = get_tick_ms();
        gsm_nw_ctx.step = 3;
        return false;

    case 3: { 
        bool handled = false;
        while (gsm_fetch_line(line, sizeof(line))) {
            log_raw_line(line);
            if (at_parse_line(line, &urc)) {
                if (urc.type == URC_CPIN_READY) {
                    gsm_nw_ctx.step = 4;
                    gsm_nw_ctx.time_stamp = get_tick_ms();
                    handled = true;
                    delete_line(line);
                    break;
                } else if (urc.type == URC_ERROR) {
                    gsm_nw_ctx.step = 20;
                    handled = true;
                    delete_line(line);
                    break;
                }
            }
            delete_line(line);
        }
        if (!handled && is_timeout(gsm_nw_ctx.time_stamp, TIME_OUT)) {
            send_to_debug("time out CPIN\r\n");
            gsm_nw_ctx.step = 20;
        }
        return false;
    }

    case 4:
        send_to_debug(">>> check CREG\r\n");
        send_at_comand("AT+CREG?\r\n");
        gsm_nw_ctx.time_stamp = get_tick_ms();
        gsm_nw_ctx.step = 5;
        return false;

    case 5: { 
        bool handled = false;
        while (gsm_fetch_line(line, sizeof(line))) {
            log_raw_line(line);
            if (at_parse_line(line, &urc)) {
                if (urc.type == URC_CREG) {
                    gsm_nw_ctx.step = 6;
                    gsm_nw_ctx.time_stamp = get_tick_ms();
                    handled = true;
                    delete_line(line);
                    break;
                } else if (urc.type == URC_ERROR) {
                    gsm_nw_ctx.step = 20;
                    handled = true;
                    delete_line(line);
                    break;
                }
            }
            delete_line(line);
        }
        if (!handled && is_timeout(gsm_nw_ctx.time_stamp, TIME_OUT)) {
            send_to_debug("time out CREG\r\n");
            gsm_nw_ctx.step = 20;
        }
        return false;
    }

    case 6:
        send_to_debug(">>> SMS text mode\r\n");
        send_at_comand("AT+CMGF=1\r\n");
        gsm_nw_ctx.time_stamp = get_tick_ms();
        gsm_nw_ctx.step = 7;
        return false;

    case 7: { 
        bool handled = false;
        while (gsm_fetch_line(line, sizeof(line))) {
            log_raw_line(line);
            if (at_parse_line(line, &urc)) {
                if (urc.type == URC_OK) {
                    gsm_nw_ctx.step = 8;
                    gsm_nw_ctx.time_stamp = get_tick_ms();
                    handled = true;
                    delete_line(line);
                    break;
                } else if (urc.type == URC_ERROR) {
                    gsm_nw_ctx.step = 20;
                    handled = true;
                    delete_line(line);
                    break;
                }
            }
            delete_line(line);
        }
        if (!handled && is_timeout(gsm_nw_ctx.time_stamp, TIME_OUT)) {
            send_to_debug("time out CMGF\r\n");
            gsm_nw_ctx.step = 20;
        }
        return false;
    }

    case 8:
        send_to_debug(">>> charset GSM\r\n");
        send_at_comand("AT+CSCS=\"GSM\"\r\n");
        gsm_nw_ctx.time_stamp = get_tick_ms();
        gsm_nw_ctx.step = 9;
        return false;

    case 9: { 
        bool handled = false;
        while (gsm_fetch_line(line, sizeof(line))) {
            log_raw_line(line);
            if (at_parse_line(line, &urc)) {
                if (urc.type == URC_OK) {
                    gsm_nw_ctx.step = 10;
                    gsm_nw_ctx.time_stamp = get_tick_ms();
                    handled = true;
                    delete_line(line);
                    break;
                } else if (urc.type == URC_ERROR) {
                    gsm_nw_ctx.step = 20;
                    handled = true;
                    delete_line(line);
                    break;
                }
            }
            delete_line(line);
        }
        if (!handled && is_timeout(gsm_nw_ctx.time_stamp, TIME_OUT)) {
            send_to_debug("time out CSCS\r\n");
            gsm_nw_ctx.step = 20;
        }
        return false;
    }

    case 10:
        send_to_debug(">>> select storage\r\n");
        send_at_comand("AT+CPMS=\"ME\",\"ME\",\"ME\"\r\n");
        gsm_nw_ctx.time_stamp = get_tick_ms();
        gsm_nw_ctx.step = 11;
        return false;

    case 11: { 
        bool handled = false;
        while (gsm_fetch_line(line, sizeof(line))) {
            log_raw_line(line);
            if (at_parse_line(line, &urc)) {
                if (urc.type == URC_OK) {
                    gsm_nw_ctx.step = 12;
                    gsm_nw_ctx.time_stamp = get_tick_ms();
                    handled = true;
                    delete_line(line);
                    break;
                } else if (urc.type == URC_ERROR) {
                    gsm_nw_ctx.step = 20;
                    handled = true;
                    delete_line(line);
                    break;
                }
            }
            delete_line(line);
        }
        if (!handled && is_timeout(gsm_nw_ctx.time_stamp, TIME_OUT)) {
            send_to_debug("time out CPMS\r\n");
            gsm_nw_ctx.step = 20;
        }
        return false;
    }

    case 12:
        send_to_debug(">>> enable detail error\r\n");
        send_at_comand("AT+CMEE=1\r\n");
        gsm_nw_ctx.time_stamp = get_tick_ms();
        gsm_nw_ctx.step = 13;
        return false;

    case 13: { 
        bool handled = false;
        while (gsm_fetch_line(line, sizeof(line))) {
            log_raw_line(line);
            if (at_parse_line(line, &urc)) {
                if (urc.type == URC_OK) {
                    gsm_nw_ctx.step = 100;
                    gsm_nw_ctx.time_stamp = get_tick_ms();
                    handled = true;
                    delete_line(line);
                    break;
                } else if (urc.type == URC_ERROR) {
                    gsm_nw_ctx.step = 20;
                    handled = true;
                    delete_line(line);
                    break;
                }
            }
            delete_line(line);
        }
        if (!handled && is_timeout(gsm_nw_ctx.time_stamp, TIME_OUT)) {
            send_to_debug("time out CMEE\r\n");
            gsm_nw_ctx.step = 20;
        }
        return false;
    }

    case 20: 
        gsm_nw_ctx.retry++;
        if (gsm_nw_ctx.retry > 3) {
            send_to_debug("nw fail after 3 retry\r\n");
            if (gsm_nw_ctx.on_error) gsm_nw_ctx.on_error(NULL, URC_ERROR);
            gsm_nw_ctx.step = 100; 
        } else {
            send_to_debug("retry...\r\n");
            gsm_nw_ctx.step = 0;
            gsm_nw_ctx.time_stamp = get_tick_ms();
        }
        return false;

    case 100:
        return true;

    default:
        break;
    }
    return (gsm_nw_ctx.step == 100);
}

void gsm_nw_process(void) {

    switch (gsm_nw_ctx.state) {
    case GSM_NW_BASIC:
        if (gsm_nw_phase_basic()){
            /* BASIC done -> switch to LTE and reset step/timer */
            gsm_nw_ctx.state = GSM_NW_LTE;
            gsm_nw_ctx.step = 0;
            gsm_nw_ctx.time_stamp = get_tick_ms();
        }
        break;
    case GSM_NW_LTE:
        if (gsm_nw_phase_lte()){
            if (gsm_nw_ctx.step == 100) {
                gsm_nw_ctx.state = GSM_NW_DONE;
            }
        }
        break;

    case GSM_NW_DONE:

        if (gsm_nw_ctx.step != 101) {
            send_to_debug (">>>> cau hinh mang da song \r\n");
            gsm_nw_ctx.step = 101; 
        }
        break;
    default:
        break;
    }
}


bool gsm_nw_process_step(void){
    gsm_nw_process();
    return (gsm_nw_ctx.step != 100);
}

bool gsm_nw_is_ready(void){
    return (gsm_nw_ctx.state == GSM_NW_DONE);
}


bool gsm_nw_phase_lte() {
    switch (gsm_nw_ctx.step) {
    case 0:
        gsm_nw_ctx.retry = 0;
        send_to_debug(">>> set gprs\r\n");
        send_at_comand("AT+CGATT=1\r\n");
        gsm_nw_ctx.time_stamp = get_tick_ms();
        gsm_nw_ctx.step = 1;
        return false;

    case 1: { 
        bool handled = false;
        while (gsm_fetch_line(line, sizeof(line))) {
            log_raw_line(line);
            if (at_parse_line(line, &urc)) {
                if (urc.type == URC_OK) {
                    gsm_nw_ctx.step = 2;
                    gsm_nw_ctx.time_stamp = get_tick_ms();
                    handled = true;
                    delete_line(line);
                    break;
                } else if (urc.type == URC_ERROR) {
                    send_to_debug(">>> CGATT ERROR\r\n");
                    gsm_nw_ctx.step = 20;
                    handled = true;
                    delete_line(line);
                    break;
                }
            }
            delete_line(line);
        }
        if (!handled && is_timeout(gsm_nw_ctx.time_stamp, TIME_OUT)) {
            send_to_debug("time out CGATT\r\n");
            gsm_nw_ctx.step = 20;
        }
        return false;
    }

    case 2:
        send_to_debug(">>> khai bao PDP\r\n");
        send_at_comand("AT+CGDCONT=1,\"IP\",\"v-internet\"\r\n");
        gsm_nw_ctx.time_stamp = get_tick_ms();
        gsm_nw_ctx.step = 3;
        return false;

    case 3: { 
        bool handled = false;
        while (gsm_fetch_line(line, sizeof(line))) {
            log_raw_line(line);
            if (at_parse_line(line, &urc)) {
                if (urc.type == URC_OK) {
                    gsm_nw_ctx.step = 4;
                    gsm_nw_ctx.time_stamp = get_tick_ms();
                    handled = true;
                    delete_line(line);
                    break;
                } else if (urc.type == URC_ERROR) {
                    send_to_debug(">>> CGDCONT ERROR\r\n");
                    gsm_nw_ctx.step = 20;
                    handled = true;
                    delete_line(line);
                    break;
                }
            }
            delete_line(line);
        }
        if (!handled && is_timeout(gsm_nw_ctx.time_stamp, TIME_OUT)) {
            send_to_debug("time out CGDCONT\r\n");
            gsm_nw_ctx.step = 20;
        }
        return false;
    }

    case 4:
        send_to_debug(">>> activate PDP\r\n");
        send_at_comand("AT+CGACT=1,1\r\n");
        gsm_nw_ctx.time_stamp = get_tick_ms();
        gsm_nw_ctx.step = 5;
        return false;

    case 5: { 
        bool handled = false;
        while (gsm_fetch_line(line, sizeof(line))) {
            log_raw_line(line);
            if (at_parse_line(line, &urc)) {
                if (urc.type == URC_OK) {
                    gsm_nw_ctx.step = 6;
                    gsm_nw_ctx.time_stamp = get_tick_ms();
                    handled = true;
                    delete_line(line);
                    break;
                } else if (urc.type == URC_ERROR) {
                    send_to_debug(">>> CGACT ERROR\r\n");
                    gsm_nw_ctx.step = 20;
                    handled = true;
                    delete_line(line);
                    break;
                }
            }
            delete_line(line);
        }
        if (!handled && is_timeout(gsm_nw_ctx.time_stamp, TIME_OUT)) {
            send_to_debug("time out CGACT\r\n");
            gsm_nw_ctx.step = 20;
        }
        return false;
    }

    case 6:
        send_to_debug(">>> check IP\r\n");
        send_at_comand("AT+CGPADDR=1\r\n");
        gsm_nw_ctx.time_stamp = get_tick_ms();
        gsm_nw_ctx.step = 7;
        return false;

    case 7: { 
        bool got_ip = false, got_ok = false;
        while (gsm_fetch_line(line, sizeof(line))) {
            log_raw_line(line);
            if (strstr(line, "+CGPADDR:") != NULL) {
                got_ip = true;
            }
            if (at_parse_line(line, &urc)) {
                if (urc.type == URC_OK) {
                    got_ok = true;
                    delete_line(line);
                } else if (urc.type == URC_ERROR) {
                    send_to_debug(">>> CGPADDR ERROR\r\n");
                    gsm_nw_ctx.step = 20;
                    delete_line(line);
                    return false;
                }
            }
            delete_line(line);
        }
        if (got_ip && got_ok) {
            send_to_debug(">>> IP confirmed, GPRS ready\r\n");
            gsm_nw_ctx.step = 100;
            return true;
        }
        if (is_timeout(gsm_nw_ctx.time_stamp, 5000)) {
            send_to_debug("time out check IP\r\n");
            gsm_nw_ctx.step = 20;
        }
        return false;
    }

    case 20: 
        gsm_nw_ctx.retry++;
        if (gsm_nw_ctx.retry > 3) {
            send_to_debug("nw LTE fail after 3 retry\r\n");
            if (gsm_nw_ctx.on_error) gsm_nw_ctx.on_error(NULL, URC_ERROR);
            gsm_nw_ctx.step = 100;
        } else {
            send_to_debug("retry LTE...\r\n");
            gsm_nw_ctx.step = 0;
            gsm_nw_ctx.time_stamp = get_tick_ms();
        }
        return false;

    case 100:
        return true;

    default:
        break;
    }
    return (gsm_nw_ctx.step == 100);
}
