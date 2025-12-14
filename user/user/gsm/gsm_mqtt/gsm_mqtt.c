#include "gsm_mqtt.h"
#include "led/led.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

gsm_mqtt_context_t gsm_mqtt_ctx;

char line [200];
urc_t urc;

 bool is_timeout(uint32_t start_ms, uint32_t timeout_ms) {
    return (get_tick_ms() - start_ms) >= timeout_ms;
}

void gsm_mqtt_init(gsm_mqtt_cb_t mqtt_done_cb, gsm_mqtt_cb_t mqtt_error_cb){
    gsm_mqtt_ctx.on_mqtt_done = mqtt_done_cb;
    gsm_mqtt_ctx.on_mqtt_error = mqtt_error_cb;
    gsm_mqtt_ctx.retry = 0;
    gsm_mqtt_ctx.time_stamp = get_tick_ms();
    gsm_mqtt_ctx.phase = MQTT_PHASE_STOP; 
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
    gsm_mqtt_ctx.message.qos = 1; 
    gsm_mqtt_ctx.message.retain = 0;
    gsm_mqtt_ctx.on_mqtt_message = NULL;
}

static char mqtt_rx_topic[128];
static uint8_t mqtt_rx_payload[256];
static uint16_t mqtt_rx_topic_len = 0;
static uint16_t mqtt_rx_topic_expected = 0;
static uint16_t mqtt_rx_payload_len = 0;
static uint16_t mqtt_rx_payload_expected = 0;
static uint8_t mqtt_rx_state = 0;
static uint32_t mqtt_rx_timeout = 0;

void gsm_mqtt_set_message_callback(gsm_mqtt_msg_cb_t callback) {
    gsm_mqtt_ctx.on_mqtt_message = callback;
}

static inline int parse_int_mqtt(const char *s) {
    int val = 0;
    while (*s && isdigit(*s)) {
        val = val * 10 + (*s - '0');
        s++;
    }
    return val;
}

static void mqtt_rx_read_topic(void) {
    if (mqtt_rx_state != 1) return;
    if (mqtt_rx_topic_len >= mqtt_rx_topic_expected) {
        mqtt_rx_state = 2;
        return;
    }
    
    extern uint16_t uart_sim_available(void);
    extern uint16_t uart_sim_read(uint8_t *buf, uint16_t len);
    
    uint16_t avail = uart_sim_available();
    if (avail == 0) return;
    
    uint16_t remaining = mqtt_rx_topic_expected - mqtt_rx_topic_len;
    uint16_t to_read = (avail < remaining) ? avail : remaining;
    to_read = (to_read < (sizeof(mqtt_rx_topic) - mqtt_rx_topic_len - 1)) ? 
              to_read : (sizeof(mqtt_rx_topic) - mqtt_rx_topic_len - 1);
    
    if (to_read > 0) {
        uint16_t read = uart_sim_read((uint8_t *)&mqtt_rx_topic[mqtt_rx_topic_len], to_read);
        mqtt_rx_topic_len += read;
        
        if (mqtt_rx_topic_len >= mqtt_rx_topic_expected) {
            mqtt_rx_topic[mqtt_rx_topic_len] = '\0';
        }
    }
}

static void mqtt_rx_read_payload(void) {
    if (mqtt_rx_state != 2) return;
    if (mqtt_rx_payload_expected == 0) return;
    if (mqtt_rx_payload_len >= mqtt_rx_payload_expected) return;
    
    extern uint16_t uart_sim_available(void);
    extern uint16_t uart_sim_read(uint8_t *buf, uint16_t len);
    
    uint16_t avail = uart_sim_available();
    if (avail == 0) return;
    
    uint16_t remaining = mqtt_rx_payload_expected - mqtt_rx_payload_len;
    uint16_t to_read = (avail < remaining) ? avail : remaining;
    to_read = (to_read < (sizeof(mqtt_rx_payload) - mqtt_rx_payload_len)) ? 
              to_read : (sizeof(mqtt_rx_payload) - mqtt_rx_payload_len);
    
    if (to_read > 0) {
        uint16_t read = uart_sim_read(&mqtt_rx_payload[mqtt_rx_payload_len], to_read);
        mqtt_rx_payload_len += read;
    }
}


void gsm_mqtt_handle_urc(const char *line) {
    if (!line) return;
    
    urc_t urc;
    if (!at_parse_line(line, &urc)) {
        if (mqtt_rx_state == 1 && mqtt_rx_topic_expected > 0 && mqtt_rx_topic_len < mqtt_rx_topic_expected) {
            if (strlen(line) > 0 && line[0] != '+' && line[0] != 'A' && line[0] != 'O' && line[0] != 'E') {
                uint16_t line_len = strlen(line);
                uint16_t remaining = mqtt_rx_topic_expected - mqtt_rx_topic_len;
                uint16_t to_copy = (line_len < remaining) ? line_len : remaining;
                to_copy = (to_copy < sizeof(mqtt_rx_topic) - mqtt_rx_topic_len - 1) ? 
                         to_copy : (sizeof(mqtt_rx_topic) - mqtt_rx_topic_len - 1);
                memcpy(&mqtt_rx_topic[mqtt_rx_topic_len], line, to_copy);
                mqtt_rx_topic_len += to_copy;
                if (mqtt_rx_topic_len >= mqtt_rx_topic_expected) {
                    mqtt_rx_topic[mqtt_rx_topic_len] = '\0';
                }
            }
        }
        else if (mqtt_rx_state == 2 && mqtt_rx_payload_expected > 0 && mqtt_rx_payload_len < mqtt_rx_payload_expected) {
            if (strlen(line) > 0 && line[0] != '+' && line[0] != 'A' && line[0] != 'O' && line[0] != 'E') {
                uint16_t line_len = strlen(line);
                uint16_t remaining = mqtt_rx_payload_expected - mqtt_rx_payload_len;
                uint16_t to_copy = (line_len < remaining) ? line_len : remaining;
                to_copy = (to_copy < sizeof(mqtt_rx_payload) - mqtt_rx_payload_len) ? 
                         to_copy : (sizeof(mqtt_rx_payload) - mqtt_rx_payload_len);
                memcpy(&mqtt_rx_payload[mqtt_rx_payload_len], line, to_copy);
                mqtt_rx_payload_len += to_copy;
            }
        }
        return;
    }
    
    if (urc.type == URC_MQTT_RXSTART) {
        mqtt_rx_state = 1;
        mqtt_rx_topic_len = 0;
        mqtt_rx_topic_expected = urc.v2;
        mqtt_rx_payload_len = 0;
        mqtt_rx_payload_expected = urc.v3;
        mqtt_rx_timeout = get_tick_ms();
    }
    else if (urc.type == URC_MQTT_RXTOPIC) {
        mqtt_rx_state = 1;
        mqtt_rx_topic_len = 0;
        mqtt_rx_topic_expected = urc.v2;
    }
    else if (urc.type == URC_MQTT_RXPAYLOAD) {
        mqtt_rx_state = 2;
        mqtt_rx_payload_len = 0;
        mqtt_rx_payload_expected = urc.v2;
    }
    else if (urc.type == URC_MQTT_RXEND) {
        if (mqtt_rx_topic_len > 0 && mqtt_rx_payload_len > 0) {
            mqtt_rx_topic[mqtt_rx_topic_len] = '\0';
            if (gsm_mqtt_ctx.on_mqtt_message) {
                gsm_mqtt_ctx.on_mqtt_message(mqtt_rx_topic, mqtt_rx_payload, mqtt_rx_payload_len);
            }
        }
        mqtt_rx_state = 0;
        mqtt_rx_topic_len = 0;
        mqtt_rx_payload_len = 0;
    }
}

 bool mqtt_phase_stop(void) {
    switch (gsm_mqtt_ctx.step) {
    case 0:
        send_to_debug(">>> dong phien ket noi truoc\r\n");
        send_at_comand("AT+CMQTTSTOP\r\n");
        gsm_mqtt_ctx.time_stamp = get_tick_ms();
        gsm_mqtt_ctx.step = 1;
        return true;
        
    case 1:
        while(gsm_fetch_line(line, sizeof(line))){
            log_raw_line(line);
            char dbg[256];
            snprintf(dbg, sizeof(dbg), ">>> MQTT STOP response: %s\r\n", line);
            send_to_debug(dbg);
            
            if(at_parse_line(line, &urc)){
                if(urc.type == URC_OK){
                    send_to_debug(">>> MQTT STOP OK\r\n");
                    gsm_mqtt_ctx.step = 0;
                    delete_line(line);
                    return false;
                    break;
                }
                else if (urc.type == URC_ERROR) {
                    send_to_debug(">>> MQTT STOP ERROR (continue anyway)\r\n");
                    gsm_mqtt_ctx.step = 0;
                    delete_line(line);
                    return false;
                }
            }
            delete_line(line);
        }
        if (is_timeout(gsm_mqtt_ctx.time_stamp, 10000)) {
            send_to_debug(">>> MQTT STOP timeout (continue anyway)\r\n");
            gsm_mqtt_ctx.step = 0;
            return false;
        }
        return true;
        
    default:
        gsm_mqtt_ctx.step = 0;
        return false;
    }
}

bool mqtt_phase_start(void) {
    switch (gsm_mqtt_ctx.step) {
    case 0:
        send_at_comand("AT+CMQTTSTART\r\n");
        gsm_mqtt_ctx.time_stamp = get_tick_ms();
        gsm_mqtt_ctx.step = 1;
        return true;
        
    case 1:
        while(gsm_fetch_line(line, sizeof(line))){
            if(at_parse_line(line, &urc)){
                if(urc.type == URC_OK || urc.type == URC_ERROR){
                    gsm_mqtt_ctx.step = 0;
                    delete_line(line);
                    return false;
                }
            }
            delete_line(line);
        }
        if (is_timeout(gsm_mqtt_ctx.time_stamp, 10000)) {
            gsm_mqtt_ctx.step = 0;
            return false;
        }
        return true;
        
    default:
        gsm_mqtt_ctx.step = 0;
        return false;
    }
}

bool mqtt_phase_accq(void) {
    switch (gsm_mqtt_ctx.step) {
    case 0:
        send_at_comand("AT+CMQTTREL=0\r\n");
        gsm_mqtt_ctx.time_stamp = get_tick_ms();
        gsm_mqtt_ctx.step = 1;
        return true;
    
    case 1:
        while(gsm_fetch_line(line, sizeof(line))){
            if(at_parse_line(line, &urc)){
                if(urc.type == URC_OK || urc.type == URC_ERROR){
                    gsm_mqtt_ctx.step = 2;
                    delete_line(line);
                    return true;
                }
            }
            delete_line(line);
        }
        if (is_timeout(gsm_mqtt_ctx.time_stamp, 5000)) {
            gsm_mqtt_ctx.step = 2;
            return true;
        }
        return true;
    
    case 2: {
        char cmd[96];
        snprintf(cmd, sizeof(cmd), "AT+CMQTTACCQ=0,\"%s\",1\r\n", gsm_mqtt_ctx.config.client_id);
        send_at_comand(cmd);
        gsm_mqtt_ctx.time_stamp = get_tick_ms();
        gsm_mqtt_ctx.step = 3;
        return true;
    }
    
    case 3:
        while(gsm_fetch_line(line, sizeof(line))){
            if(at_parse_line(line, &urc)){
                if(urc.type == URC_OK){
                    gsm_mqtt_ctx.step = 0;
                    delete_line(line);
                    return false;
                }
                else if (urc.type == URC_ERROR || urc.type == URC_CME_ERROR) {
                    gsm_mqtt_ctx.phase = MQTT_PHASE_ERROR;
                    gsm_mqtt_ctx.step = 0;
                    delete_line(line);
                    return false;
                }
            }
            delete_line(line);
        }
        if (is_timeout(gsm_mqtt_ctx.time_stamp, 7000)) {
            gsm_mqtt_ctx.phase = MQTT_PHASE_ERROR;
            gsm_mqtt_ctx.step = 0;
            return false;
        }
        return true;
        
    default:
        gsm_mqtt_ctx.step = 0;
        return false;
    }
}

bool mqtt_phase_ssl_ver(void) {
    switch (gsm_mqtt_ctx.step) {
    case 0:
        send_at_comand("AT+CSSLCFG=\"sslversion\",0,4\r\n");
        gsm_mqtt_ctx.time_stamp = get_tick_ms();
        gsm_mqtt_ctx.step = 1;
        return true;
    
    case 1:
        while(gsm_fetch_line(line, sizeof(line))){
            if(at_parse_line(line, &urc)){
                if(urc.type == URC_OK){
                    gsm_mqtt_ctx.step = 0;
                    delete_line(line);
                    return false;
                }
                else if (urc.type == URC_ERROR || urc.type == URC_CME_ERROR) {
                    gsm_mqtt_ctx.phase = MQTT_PHASE_ERROR;
                    gsm_mqtt_ctx.step = 0;
                    delete_line(line);
                    return false;
                }
            }
            delete_line(line);
        }
        if (is_timeout(gsm_mqtt_ctx.time_stamp, 7000)) {
            gsm_mqtt_ctx.phase = MQTT_PHASE_ERROR;
            gsm_mqtt_ctx.step = 0;
            return false;
        }
        return true;
        
    default:
        gsm_mqtt_ctx.step = 0;
        return false;
    }
}

bool mqtt_phase_ssl_sni(void) {
    switch (gsm_mqtt_ctx.step) {
    case 0:
        send_at_comand("AT+CSSLCFG=\"enableSNI\",0,1\r\n");
        gsm_mqtt_ctx.time_stamp = get_tick_ms();
        gsm_mqtt_ctx.step = 1;
        return true;
    
    case 1:
        while(gsm_fetch_line(line, sizeof(line))){
            if(at_parse_line(line, &urc)){
                if(urc.type == URC_OK){
                    gsm_mqtt_ctx.step = 0;
                    delete_line(line);
                    return false;
                }
                else if (urc.type == URC_ERROR || urc.type == URC_CME_ERROR) {
                    gsm_mqtt_ctx.phase = MQTT_PHASE_ERROR;
                    gsm_mqtt_ctx.step = 0;
                    delete_line(line);
                    return false;
                }
            }
            delete_line(line);
        }
        if (is_timeout(gsm_mqtt_ctx.time_stamp, 7000)) {
            gsm_mqtt_ctx.phase = MQTT_PHASE_ERROR;
            gsm_mqtt_ctx.step = 0;
            return false;
        }
        return true;
        
    default:
        gsm_mqtt_ctx.step = 0;
        return false;
    }
}

bool mqtt_phase_ssl_auth(void) {
    switch (gsm_mqtt_ctx.step) {
    case 0: {
        char cmd[64];
        snprintf(cmd, sizeof(cmd), "AT+CSSLCFG=\"authmode\",0,0\r\n");
        char dbg[128];
        snprintf(dbg, sizeof(dbg), ">>> MQTT SSL AUTH: %s", cmd);
        send_to_debug(dbg);
        log_raw_line(cmd);
        send_at_comand(cmd);
        gsm_mqtt_ctx.time_stamp = get_tick_ms();
        gsm_mqtt_ctx.step = 1;
        return true;
    }
    
    case 1:
        while(gsm_fetch_line(line, sizeof(line))){
            log_raw_line(line);
            char dbg[256];
            snprintf(dbg, sizeof(dbg), ">>> MQTT SSL AUTH response: %s\r\n", line);
            send_to_debug(dbg);
            
            if(at_parse_line(line, &urc)){
                if(urc.type == URC_OK){
                    send_to_debug(">>> MQTT SSL AUTH OK\r\n");
                    gsm_mqtt_ctx.step = 2;
                    delete_line(line);
                    return true;
                }
                else if (urc.type == URC_ERROR || urc.type == URC_CME_ERROR) {
                    send_to_debug(">>> MQTT SSL AUTH ERROR (continue anyway)\r\n");
                    gsm_mqtt_ctx.step = 2;
                    delete_line(line);
                    return true;
                }
            }
            delete_line(line);
        }
        if (is_timeout(gsm_mqtt_ctx.time_stamp, 7000)) {
            send_to_debug(">>> MQTT SSL AUTH timeout (continue anyway)\r\n");
            gsm_mqtt_ctx.step = 2;
            return true;
        }
        return true;
    
    case 2: {
        char cmd[64];
        snprintf(cmd, sizeof(cmd), "AT+CSSLCFG=\"ignorelocaltime\",0,1\r\n");
        char dbg[128];
        snprintf(dbg, sizeof(dbg), ">>> MQTT SSL IGNORE_TIME: %s", cmd);
        send_to_debug(dbg);
        log_raw_line(cmd);
        send_at_comand(cmd);
        gsm_mqtt_ctx.time_stamp = get_tick_ms();
        gsm_mqtt_ctx.step = 3;
        return true;
    }
    
    case 3:
        while(gsm_fetch_line(line, sizeof(line))){
            log_raw_line(line);
            char dbg[256];
            snprintf(dbg, sizeof(dbg), ">>> MQTT SSL IGNORE_TIME response: %s\r\n", line);
            send_to_debug(dbg);
            
            if(at_parse_line(line, &urc)){
                if(urc.type == URC_OK || urc.type == URC_ERROR){
                    send_to_debug(">>> MQTT SSL IGNORE_TIME done\r\n");
                    gsm_mqtt_ctx.step = 0;
                    delete_line(line);
                    return false;
                }
            }
            delete_line(line);
        }
        if (is_timeout(gsm_mqtt_ctx.time_stamp, 5000)) {
            send_to_debug(">>> MQTT SSL IGNORE_TIME timeout (continue anyway)\r\n");
            gsm_mqtt_ctx.step = 0;
            return false;
        }
        return true;
        
    default:
        gsm_mqtt_ctx.step = 0;
        return false;
    }
}

bool mqtt_phase_ssl_tls(void) {
    switch (gsm_mqtt_ctx.step) {
    case 0: {
        char cmd[64];
        snprintf(cmd, sizeof(cmd), "AT+CMQTTSSLCFG=0,0\r\n");
        char dbg[128];
        snprintf(dbg, sizeof(dbg), ">>> MQTT SSL TLS: %s", cmd);
        send_to_debug(dbg);
        log_raw_line(cmd);
        send_at_comand(cmd);
        gsm_mqtt_ctx.time_stamp = get_tick_ms();
        gsm_mqtt_ctx.step = 1;
        return true;
    }
    
    case 1:
        while(gsm_fetch_line(line, sizeof(line))){
            log_raw_line(line);
            char dbg[256];
            snprintf(dbg, sizeof(dbg), ">>> MQTT SSL TLS response: %s\r\n", line);
            send_to_debug(dbg);
            
            if(at_parse_line(line, &urc)){
                if(urc.type == URC_OK){
                    send_to_debug(">>> MQTT SSL TLS OK\r\n");
                    gsm_mqtt_ctx.step = 0;
                    delete_line(line);
                    return false;
                }
                else if (urc.type == URC_ERROR || urc.type == URC_CME_ERROR) {
                    send_to_debug(">>> MQTT SSL TLS ERROR\r\n");
                    gsm_mqtt_ctx.phase = MQTT_PHASE_ERROR;
                    gsm_mqtt_ctx.step = 0;
                    delete_line(line);
                    return false;
                }
            }
            delete_line(line);
        }
        if (is_timeout(gsm_mqtt_ctx.time_stamp, 7000)) {
            send_to_debug(">>> MQTT SSL TLS timeout\r\n");
            gsm_mqtt_ctx.phase = MQTT_PHASE_ERROR;
            gsm_mqtt_ctx.step = 0;
            return false;
        }
        return true;
        
    default:
        gsm_mqtt_ctx.step = 0;
        return false;
    }
}

bool mqtt_phase_connect(void) {
    switch (gsm_mqtt_ctx.step) {
    case 0: {
        char cmd[256];
        snprintf(cmd, sizeof(cmd),
            "AT+CMQTTCONNECT=0,\"%s\",%u,%u,\"%s\",\"%s\"\r\n",
            gsm_mqtt_ctx.config.broker_url,
            gsm_mqtt_ctx.config.keep_alive,
            1,
            gsm_mqtt_ctx.config.username,
            gsm_mqtt_ctx.config.password);
        char dbg[320];
        snprintf(dbg, sizeof(dbg), ">>> MQTT CONNECT: AT+CMQTTCONNECT=0,\"%s\",%u,%u,\"%s\",\"%s\"\r\n",
                 gsm_mqtt_ctx.config.broker_url,
                 gsm_mqtt_ctx.config.keep_alive,
                 1,
                 gsm_mqtt_ctx.config.username,
                 gsm_mqtt_ctx.config.password);
        send_to_debug(dbg);
        log_raw_line(cmd);
        send_at_comand(cmd);
        gsm_mqtt_ctx.time_stamp = get_tick_ms();
        gsm_mqtt_ctx.step = 1;
        return true;
    }
    
    case 1:
        while(gsm_fetch_line(line, sizeof(line))){
            log_raw_line(line);
            char dbg[256];
            snprintf(dbg, sizeof(dbg), ">>> MQTT CONNECT response: %s\r\n", line);
            send_to_debug(dbg);
            
            if(strstr(line, "+CMQTTCONNECT:") != NULL){
                int client_idx = 0, result = 0;
                if(sscanf(line, "+CMQTTCONNECT: %d,%d", &client_idx, &result) == 2){
                    char err_dbg[256];
                    if(result == 0){
                        send_to_debug(">>> MQTT CONNECT OK - Connected!\r\n");
                        gsm_mqtt_ctx.is_connected = 1;
                        gsm_mqtt_ctx.phase = MQTT_PHASE_IDLE;
                        gsm_mqtt_ctx.step = 0;
                        delete_line(line);
                        return false;
                    } else {
                        const char *err_msg = "Unknown";
                        switch(result) {
                            case 1: err_msg = "Connection refused - unacceptable protocol version"; break;
                            case 2: err_msg = "Connection refused - identifier rejected"; break;
                            case 3: err_msg = "Connection refused - server unavailable"; break;
                            case 4: err_msg = "Connection refused - bad user name or password"; break;
                            case 5: err_msg = "Connection refused - not authorized"; break;
                            case 15: err_msg = "Connection failed - network error/SSL error"; break;
                            default: err_msg = "Connection failed"; break;
                        }
                        snprintf(err_dbg, sizeof(err_dbg), ">>> MQTT CONNECT ERROR: code=%d (%s)\r\n", result, err_msg);
                        send_to_debug(err_dbg);
                        gsm_mqtt_ctx.phase = MQTT_PHASE_ERROR;
                        gsm_mqtt_ctx.step = 0;
                        delete_line(line);
                        return false;
                    }
                }
            }
            
            if(at_parse_line(line, &urc)){
                if(urc.type == URC_OK){
                    send_to_debug(">>> MQTT CONNECT OK received, waiting for URC +CMQTTCONNECT...\r\n");
                    delete_line(line);
                    continue;
                }
                else if (urc.type == URC_ERROR || urc.type == URC_CME_ERROR) {
                    send_to_debug(">>> MQTT CONNECT ERROR (no URC code)\r\n");
                    gsm_mqtt_ctx.phase = MQTT_PHASE_ERROR;
                    gsm_mqtt_ctx.step = 0;
                    delete_line(line);
                    return false;
                }
            }
            delete_line(line);
        }
        if (is_timeout(gsm_mqtt_ctx.time_stamp, 60000)) {
            send_to_debug(">>> MQTT CONNECT timeout (60s)\r\n");
            gsm_mqtt_ctx.phase = MQTT_PHASE_ERROR;
            gsm_mqtt_ctx.step = 0;
            return false;
        }
        return true;
        
    default:
        gsm_mqtt_ctx.step = 0;
        return false;
    }
}

bool mqtt_phase_sub(void) {
    switch (gsm_mqtt_ctx.step) {
    case 0: {
        uint16_t topic_len = strlen(gsm_mqtt_ctx.message.topic);
        char cmd[128];
        snprintf(cmd, sizeof(cmd), "AT+CMQTTSUB=0,%u,1\r\n", topic_len);
        char dbg[256];
        snprintf(dbg, sizeof(dbg), ">>> MQTT SUB: %s", cmd);
        send_to_debug(dbg);
        log_raw_line(cmd);
        send_at_comand(cmd);
        gsm_mqtt_ctx.time_stamp = get_tick_ms();
        gsm_mqtt_ctx.step = 1;
        return true;
    }
    
    case 1: {
        static bool topic_sent = false;
        if (!topic_sent) {
            char dbg[256];
            snprintf(dbg, sizeof(dbg), ">>> MQTT SUB topic: %s\r\n", gsm_mqtt_ctx.message.topic);
            send_to_debug(dbg);
            log_raw_line(gsm_mqtt_ctx.message.topic);
            send_at_comand(gsm_mqtt_ctx.message.topic);
            send_at_comand("\r\n");
            topic_sent = true;
            gsm_mqtt_ctx.time_stamp = get_tick_ms();
            return true;
        }
        
        while(gsm_fetch_line(line, sizeof(line))){
            log_raw_line(line);
            char dbg[256];
            snprintf(dbg, sizeof(dbg), ">>> MQTT SUB response: %s\r\n", line);
            send_to_debug(dbg);
            
            if(at_parse_line(line, &urc)){
                if(urc.type == URC_MQTT_SUB){
                    int client_idx = urc.v1;
                    int result = urc.v2;
                    if(result == 0){
                        send_to_debug(">>> MQTT SUB OK - Subscribed!\r\n");
                        gsm_mqtt_ctx.phase = MQTT_PHASE_IDLE;
                        gsm_mqtt_ctx.step = 0;
                        topic_sent = false;
                        delete_line(line);
                        return false;
                    } else {
                        char err_dbg[128];
                        snprintf(err_dbg, sizeof(err_dbg), ">>> MQTT SUB ERROR: code=%d\r\n", result);
                        send_to_debug(err_dbg);
                        gsm_mqtt_ctx.phase = MQTT_PHASE_ERROR;
                        gsm_mqtt_ctx.step = 0;
                        topic_sent = false;
                        delete_line(line);
                        return false;
                    }
                }
                else if(urc.type == URC_OK){
                    send_to_debug(">>> MQTT SUB OK (no URC)\r\n");
                    gsm_mqtt_ctx.phase = MQTT_PHASE_IDLE;
                    gsm_mqtt_ctx.step = 0;
                    topic_sent = false;
                    delete_line(line);
                    return false;
                }
                else if (urc.type == URC_ERROR || urc.type == URC_CME_ERROR) {
                    send_to_debug(">>> MQTT SUB ERROR\r\n");
                    gsm_mqtt_ctx.phase = MQTT_PHASE_ERROR;
                    gsm_mqtt_ctx.step = 0;
                    topic_sent = false;
                    delete_line(line);
                    return false;
                }
            }
            delete_line(line);
        }
        if (is_timeout(gsm_mqtt_ctx.time_stamp, 5000)) {
            send_to_debug(">>> MQTT SUB timeout\r\n");
            gsm_mqtt_ctx.phase = MQTT_PHASE_ERROR;
            gsm_mqtt_ctx.step = 0;
            topic_sent = false;
            return false;
        }
        return true;
    }
    
    default:
        gsm_mqtt_ctx.step = 0;
        return false;
    }
}

void gsm_mqtt_process(void) {
    static uint32_t phase_start_time = 0;
    static gsm_mqtt_t last_phase = MQTT_PHASE_STOP;
    static uint32_t last_reset_time = 0;
    
    if (last_phase != gsm_mqtt_ctx.phase) {
        phase_start_time = get_tick_ms();
        last_phase = gsm_mqtt_ctx.phase;
    }
    
    if (gsm_mqtt_ctx.phase == MQTT_PHASE_IDLE && gsm_mqtt_ctx.is_connected) {
        return;
    }
    
    uint32_t timeout_limit = 15000;
    if (gsm_mqtt_ctx.phase == MQTT_PHASE_CONN) {
        timeout_limit = 60000;
    }
    
    if (get_tick_ms() - phase_start_time > timeout_limit) {
        gsm_mqtt_ctx.phase = MQTT_PHASE_STOP;
        gsm_mqtt_ctx.step = 0;
        phase_start_time = get_tick_ms();
        last_phase = MQTT_PHASE_STOP;
        last_reset_time = get_tick_ms();
    }
    
    if (last_reset_time > 0 && get_tick_ms() - last_reset_time < 2000) {
        return;
    }
    last_reset_time = 0;
    
    switch (gsm_mqtt_ctx.phase) {
    case MQTT_PHASE_STOP:
        if(!mqtt_phase_stop()){
            gsm_mqtt_ctx.phase = MQTT_PHASE_START;
            gsm_mqtt_ctx.step = 0;
            phase_start_time = get_tick_ms();
            last_phase = MQTT_PHASE_START;
        }
        break;
        
    case MQTT_PHASE_START:
        if(!mqtt_phase_start()){
            gsm_mqtt_ctx.phase = MQTT_PHASE_ACCQ;
            gsm_mqtt_ctx.step = 0;
            phase_start_time = get_tick_ms();
            last_phase = MQTT_PHASE_ACCQ;
        }
        break;
        
    case MQTT_PHASE_ACCQ:
        if(!mqtt_phase_accq()){
            bool use_ssl = (strstr(gsm_mqtt_ctx.config.broker_url, ":8883") != NULL);
            if (use_ssl) {
                gsm_mqtt_ctx.phase = MQTT_PHASE_SSL_VER;
            } else {
                gsm_mqtt_ctx.phase = MQTT_PHASE_CONN;
            }
            gsm_mqtt_ctx.step = 0;
            phase_start_time = get_tick_ms();
            last_phase = gsm_mqtt_ctx.phase;
        }
        break;
        
    case MQTT_PHASE_SSL_VER:
        if(!mqtt_phase_ssl_ver()){
            gsm_mqtt_ctx.phase = MQTT_PHASE_SSL_SNI;
            gsm_mqtt_ctx.step = 0;
            phase_start_time = get_tick_ms();
            last_phase = MQTT_PHASE_SSL_SNI;
        }
        break;
        
    case MQTT_PHASE_SSL_SNI:
        if(!mqtt_phase_ssl_sni()){
            gsm_mqtt_ctx.phase = MQTT_PHASE_SSL_TLS;
            gsm_mqtt_ctx.step = 0;
            phase_start_time = get_tick_ms();
            last_phase = MQTT_PHASE_SSL_TLS;
        }
        break;
        
    case MQTT_PHASE_SSL_TLS:
        if(!mqtt_phase_ssl_tls()){
            gsm_mqtt_ctx.phase = MQTT_PHASE_CONN;
            gsm_mqtt_ctx.step = 0;
            phase_start_time = get_tick_ms();
            last_phase = MQTT_PHASE_CONN;
        }
        break;
        
    case MQTT_PHASE_CONN:
        if(!mqtt_phase_connect()){
            if (gsm_mqtt_ctx.is_connected) {
                if (strlen(gsm_mqtt_ctx.message.topic) > 0) {
                    gsm_mqtt_ctx.phase = MQTT_PHASE_SUB;
                } else {
                    gsm_mqtt_ctx.phase = MQTT_PHASE_IDLE;
                }
                gsm_mqtt_ctx.step = 0;
                phase_start_time = get_tick_ms();
                last_phase = gsm_mqtt_ctx.phase;
            }
        }
        break;
        
    case MQTT_PHASE_SUB:
        if(!mqtt_phase_sub()){
            gsm_mqtt_ctx.phase = MQTT_PHASE_IDLE;
            gsm_mqtt_ctx.step = 0;
            phase_start_time = get_tick_ms();
            last_phase = MQTT_PHASE_IDLE;
        }
        break;
        
    case MQTT_PHASE_IDLE:
        if (!gsm_mqtt_ctx.is_connected) {
            gsm_mqtt_ctx.phase = MQTT_PHASE_STOP;
            gsm_mqtt_ctx.step = 0;
            phase_start_time = get_tick_ms();
            last_phase = MQTT_PHASE_STOP;
        } else {
            mqtt_rx_read_topic();
            mqtt_rx_read_payload();
        }
        break;
        
    case MQTT_PHASE_ERROR:
        {
            static uint32_t error_time = 0;
            if (error_time == 0) {
                error_time = get_tick_ms();
            }
            if (get_tick_ms() - error_time > 3000) {
                send_to_debug(">>> MQTT ERROR: resetting to STOP...\r\n");
                gsm_mqtt_ctx.phase = MQTT_PHASE_STOP;
                gsm_mqtt_ctx.step = 0;
                phase_start_time = get_tick_ms();
                last_phase = MQTT_PHASE_STOP;
                error_time = 0;
            }
        }
        break;
        
    default:
        gsm_mqtt_ctx.phase = MQTT_PHASE_STOP;
        gsm_mqtt_ctx.step = 0;
        phase_start_time = get_tick_ms();
        last_phase = MQTT_PHASE_STOP;
        break;
    }
}

void gsm_mqtt_config(const char *broker_url_full, const char *client_id, const char *username, const char *password) {
    if (broker_url_full) {
        strncpy(gsm_mqtt_ctx.config.broker_url, broker_url_full, sizeof(gsm_mqtt_ctx.config.broker_url) - 1);
        gsm_mqtt_ctx.config.broker_url[sizeof(gsm_mqtt_ctx.config.broker_url) - 1] = '\0';
    } else {
        strncpy(gsm_mqtt_ctx.config.broker_url, MQTT_URL, sizeof(gsm_mqtt_ctx.config.broker_url) - 1);
        gsm_mqtt_ctx.config.broker_url[sizeof(gsm_mqtt_ctx.config.broker_url) - 1] = '\0';
    }
    
    char dbg[256];
    snprintf(dbg, sizeof(dbg), ">>> MQTT Config: url=%s, keep_alive=%u\r\n", 
             gsm_mqtt_ctx.config.broker_url, gsm_mqtt_ctx.config.keep_alive);
    send_to_debug(dbg);
    
    if (client_id) {
        strncpy(gsm_mqtt_ctx.config.client_id, client_id, sizeof(gsm_mqtt_ctx.config.client_id) - 1);
        gsm_mqtt_ctx.config.client_id[sizeof(gsm_mqtt_ctx.config.client_id) - 1] = '\0';
    }
    
    if (username) {
        strncpy(gsm_mqtt_ctx.config.username, username, sizeof(gsm_mqtt_ctx.config.username) - 1);
        gsm_mqtt_ctx.config.username[sizeof(gsm_mqtt_ctx.config.username) - 1] = '\0';
    } else {
        strncpy(gsm_mqtt_ctx.config.username, MQTT_USER, sizeof(gsm_mqtt_ctx.config.username) - 1);
        gsm_mqtt_ctx.config.username[sizeof(gsm_mqtt_ctx.config.username) - 1] = '\0';
    }
    
    if (password) {
        strncpy(gsm_mqtt_ctx.config.password, password, sizeof(gsm_mqtt_ctx.config.password) - 1);
        gsm_mqtt_ctx.config.password[sizeof(gsm_mqtt_ctx.config.password) - 1] = '\0';
    } else {
        strncpy(gsm_mqtt_ctx.config.password, MQTT_PASS, sizeof(gsm_mqtt_ctx.config.password) - 1);
        gsm_mqtt_ctx.config.password[sizeof(gsm_mqtt_ctx.config.password) - 1] = '\0';
    }
    
    strncpy(gsm_mqtt_ctx.message.topic, MQTT_CONTROL_TOPIC, sizeof(gsm_mqtt_ctx.message.topic) - 1);
    gsm_mqtt_ctx.message.topic[sizeof(gsm_mqtt_ctx.message.topic) - 1] = '\0';
    gsm_mqtt_ctx.message.qos = 1;
}

void gsm_mqtt_subscribe(const char *topic, uint8_t qos) {
    if (!gsm_mqtt_ctx.is_connected) {
        strncpy(gsm_mqtt_ctx.message.topic, topic, sizeof(gsm_mqtt_ctx.message.topic) - 1);
        gsm_mqtt_ctx.message.topic[sizeof(gsm_mqtt_ctx.message.topic) - 1] = '\0';
        gsm_mqtt_ctx.message.qos = qos;
        return;
    }
    
    strncpy(gsm_mqtt_ctx.message.topic, topic, sizeof(gsm_mqtt_ctx.message.topic) - 1);
    gsm_mqtt_ctx.message.topic[sizeof(gsm_mqtt_ctx.message.topic) - 1] = '\0';
    gsm_mqtt_ctx.message.qos = qos;
    gsm_mqtt_ctx.phase = MQTT_PHASE_SUB;
    gsm_mqtt_ctx.step = 0;
}

bool gsm_mqtt_is_connected(void) {
    return gsm_mqtt_ctx.is_connected;
}

void gsm_mqtt_publish(const char *topic, const char *payload, uint8_t qos) {
    if (!gsm_mqtt_ctx.is_connected || !topic || !payload) {
        char dbg[128];
        snprintf(dbg, sizeof(dbg), ">>> MQTT PUB: not connected or invalid params\r\n");
        send_to_debug(dbg);
        return;
    }
    
    uint16_t topic_len = strlen(topic);
    uint16_t payload_len = strlen(payload);
    
    char cmd[128];
    char line[200];
    urc_t urc;
    uint32_t timeout;
    char dbg[256];
    
    snprintf(dbg, sizeof(dbg), ">>> MQTT PUB: topic=%s, payload=%s\r\n", topic, payload);
    send_to_debug(dbg);
    
    snprintf(cmd, sizeof(cmd), "AT+CMQTTTOPIC=0,%u\r\n", topic_len);
    send_at_comand(cmd);
    timeout = get_tick_ms();
    while (get_tick_ms() - timeout < 3000) {
        if (gsm_fetch_line(line, sizeof(line))) {
            if (at_parse_line(line, &urc)) {
                if (urc.type == URC_OK) {
                    delete_line(line);
                    break;
                } else if (urc.type == URC_ERROR) {
                    delete_line(line);
                    return;
                }
            }
            delete_line(line);
        }
    }
    
    send_at_comand(topic);
    send_at_comand("\r\n");
    delay_ms(100);
    
    snprintf(cmd, sizeof(cmd), "AT+CMQTTPAYLOAD=0,%u\r\n", payload_len);
    send_at_comand(cmd);
    timeout = get_tick_ms();
    while (get_tick_ms() - timeout < 3000) {
        if (gsm_fetch_line(line, sizeof(line))) {
            if (at_parse_line(line, &urc)) {
                if (urc.type == URC_OK) {
                    delete_line(line);
                    break;
                } else if (urc.type == URC_ERROR) {
                    delete_line(line);
                    return;
                }
            }
            delete_line(line);
        }
    }
    
    send_at_comand(payload);
    send_at_comand("\r\n");
    delay_ms(100);
    
    snprintf(cmd, sizeof(cmd), "AT+CMQTTPUB=0,%u,0\r\n", qos);
    send_at_comand(cmd);
    timeout = get_tick_ms();
    while (get_tick_ms() - timeout < 5000) {
        if (gsm_fetch_line(line, sizeof(line))) {
            if (at_parse_line(line, &urc)) {
                if (urc.type == URC_OK) {
                    send_to_debug(">>> MQTT PUB: OK - Published!\r\n");
                    delete_line(line);
                    return;
                } else if (urc.type == URC_ERROR) {
                    send_to_debug(">>> MQTT PUB: ERROR\r\n");
                    delete_line(line);
                    return;
                }
            }
            delete_line(line);
        }
    }
    send_to_debug(">>> MQTT PUB: timeout\r\n");
}



