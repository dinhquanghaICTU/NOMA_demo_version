#include "gsm_mqtt.h"
#include <stdio.h>

gsm_mqtt_context_t gsm_mqtt_ctx;

char line [200];
urc_t urc;

static bool is_timeout(uint32_t start_ms, uint32_t timeout_ms) {
    return (get_tick_ms() - start_ms) >= timeout_ms;
}

void gsm_mqtt_init(gsm_mqtt_cb_t mqtt_done_cb, gsm_mqtt_cb_t mqtt_error_cb){
    gsm_mqtt_ctx.on_mqtt_done = mqtt_done_cb;
    gsm_mqtt_ctx.on_mqtt_error = mqtt_error_cb;
    gsm_mqtt_ctx.retry = 0;
    gsm_mqtt_ctx.time_stamp = get_tick_ms();
    gsm_mqtt_ctx.phase = MQTT_PHASE_START; 
    gsm_mqtt_ctx.timeout_ms = 0;
    gsm_mqtt_ctx.is_connected = 0;
    gsm_mqtt_ctx.step = 0;

    gsm_mqtt_ctx.config.broker_url[0] = '\0';
    gsm_mqtt_ctx.config.client_id[0] = '\0';
    gsm_mqtt_ctx.config.keep_alive = 60;  
    gsm_mqtt_ctx.config.port = 1883; 
    gsm_mqtt_ctx.config.username[0]= '\0';
    gsm_mqtt_ctx.config.password[0]= '\0';     

    gsm_mqtt_ctx.message.topic[0] = '\0';      
    gsm_mqtt_ctx.message.payload_len = 0;
    gsm_mqtt_ctx.message.payload = NULL;       
    gsm_mqtt_ctx.message.qos = 0; 
    gsm_mqtt_ctx.message.retain = 0;
}

bool mqtt_phase_start(void) {
    switch (gsm_mqtt_ctx.step) {
    case 0:
        send_to_debug(">>> mqtt start\r\n");
        send_at_comand("AT+CMQTTSTART\r\n");
        gsm_mqtt_ctx.time_stamp = get_tick_ms();
        gsm_mqtt_ctx.step = 1;
        return true;

    case 1: {
        bool handled = false;
        while (gsm_fetch_line(line, sizeof(line))) {
            log_raw_line(line);
            if (at_parse_line(line, &urc)) {
                if (urc.type == URC_OK) {
                    gsm_mqtt_ctx.step = 0;
                    gsm_mqtt_ctx.time_stamp = get_tick_ms();
                    delete_line(line);
                    return false; 
                } else if (urc.type == URC_ERROR) {
                    gsm_mqtt_ctx.step = 20;
                    handled = true;
                }
            }
            delete_line(line);
        }
        if (!handled && is_timeout(gsm_mqtt_ctx.time_stamp, 5000)) {
            gsm_mqtt_ctx.step = 20;
        }
        return true;
    }

    case 20:
        
        return false;

    default:
        return true;
    }
}

bool mqtt_phase_conn(const char *client_id, const char *url){

    if (client_id) {
        strncpy(gsm_mqtt_ctx.config.client_id, client_id,
                sizeof(gsm_mqtt_ctx.config.client_id) - 1);
        gsm_mqtt_ctx.config.client_id[sizeof(gsm_mqtt_ctx.config.client_id) - 1] = '\0';
    }

    switch (gsm_mqtt_ctx.phase)
    {
    case 0:
        send_to_debug(">>> khoi tao client ID \r\n");
        char cmd[96];
        snprintf(cmd, sizeof(cmd),"AT+CMQTTACCQ=0,\"%s\"\r\n",gsm_mqtt_ctx.config.client_id);
        send_at_comand(cmd);
        gsm_mqtt_ctx.time_stamp = get_tick_ms();
        gsm_mqtt_ctx.step = 1;
        break;
    case 1:{
        bool handled = false;
        while (gsm_fetch_line(line, sizeof(line))) {
            log_raw_line(line);
            if (at_parse_line(line, &urc)) {
                if (urc.type == URC_OK) {
                    gsm_mqtt_ctx.step = 2;
                    gsm_mqtt_ctx.time_stamp = get_tick_ms();
                    delete_line(line);
                    return false; 
                } else if (urc.type == URC_ERROR) {
                    gsm_mqtt_ctx.step = 20;
                    handled = true;
                }
            }
            delete_line(line);
        }
        if (!handled && is_timeout(gsm_mqtt_ctx.time_stamp, 5000)) {
            gsm_mqtt_ctx.step = 20;
        }
        return true;
    }
    case 2:{
        char cmd[128];
            snprintf(cmd, sizeof(cmd),
            "AT+CMQTTCONNECT=0,\"tcp://%s:%u\",%u,%u,\"%s\",\"%s\"\r\n",gsm_mqtt_ctx.config.broker_url, gsm_mqtt_ctx.config.port, gsm_mqtt_ctx.config.keep_alive, 0, gsm_mqtt_ctx.config.username,gsm_mqtt_ctx.config.password);
            send_at_comand(cmd);
            gsm_mqtt_ctx.time_stamp = get_tick_ms();
            gsm_mqtt_ctx.step = 3;
        break;
    }
    case 3:{
        bool handled = false;
        while (gsm_fetch_line(line, sizeof(line))) {
            log_raw_line(line);
            if (at_parse_line(line, &urc)) {
                if (urc.type == URC_OK) {
                    gsm_mqtt_ctx.step = 4;
                    gsm_mqtt_ctx.time_stamp = get_tick_ms();
                    delete_line(line);
                    return false; 
                } else if (urc.type == URC_ERROR) {
                    gsm_mqtt_ctx.step = 20;
                    handled = true;
                }
            }
            delete_line(line);
        }
        if (!handled && is_timeout(gsm_mqtt_ctx.time_stamp, 5000)) {
            gsm_mqtt_ctx.step = 20;
        }
        return true;
    }

    default:
        return false;
    }
    return false;
}

void gsm_mqtt_process(){
    switch (gsm_mqtt_ctx.phase)
    {
    case MQTT_PHASE_START:
        if(!mqtt_phase_start()){
            gsm_mqtt_ctx.phase = MQTT_PHASE_CONN;
        }
        break;

    case MQTT_PHASE_CONN:

        break;
    
    default:
        break;
    }
}

bool is_connect(){
    return gsm_mqtt_ctx.is_connected;
}
