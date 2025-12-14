#include "stm32f10x.h"
#include "hardware.h"
#include "uart.h"
#include "gsm.h"
#include "debug.h"
#include "gsm_network/gsm_nw.h"
#include "gsm_sms/gsm_sms_send.h"
#include "gsm_sms/gsm_sms_recive.h"
#include "gsm_http/gsm_http.h"
#include "led/led.h"
#include "gsm_mqtt/gsm_mqtt.h"
#include "w25Qx/w25Qx.h"
#include "../user/third_party/jsmn/jsmn.h"
#include <stdio.h>
#include <string.h>

extern void app_mqtt_test_init(void);
extern void app_mqtt_test_process(void);

static void mqtt_message_cb(const char *topic, const uint8_t *payload, uint16_t payload_len) {
    char dbg[256];
    snprintf(dbg, sizeof(dbg), ">>> MQTT msg: topic=%s, len=%u\r\n", topic ? topic : "NULL", payload_len);
    send_to_debug(dbg);
    
    if (!topic || !payload || payload_len == 0) {
        send_to_debug(">>> MQTT msg: invalid params\r\n");
        return;
    }
    
    if (strcmp(topic, "DV_test_led") == 0) {
        uint8_t state = 0;
        
        if (payload[0] == '{') {
            jsmn_parser parser;
            jsmntok_t tokens[16];
            int num_tokens;
            
            jsmn_init(&parser);
            num_tokens = jsmn_parse(&parser, (const char *)payload, payload_len, tokens, 16);
            
            if (num_tokens > 0 && tokens[0].type == JSMN_OBJECT) {
                for (int i = 1; i < num_tokens; i++) {
                    if (tokens[i].type == JSMN_STRING) {
                        int key_len = tokens[i].end - tokens[i].start;
                        if (key_len == 7 && strncmp((const char *)payload + tokens[i].start, "message", 7) == 0) {
                            if (i + 1 < num_tokens) {
                                jsmntok_t *val_tok = &tokens[i + 1];
                                if (val_tok->type == JSMN_PRIMITIVE || val_tok->type == JSMN_STRING) {
                                    int val_start = val_tok->start;
                                    int val_end = val_tok->end;
                                    if (val_start < payload_len && val_end <= payload_len) {
                                        if (val_tok->type == JSMN_STRING && val_start + 1 < val_end) {
                                            val_start++;
                                            val_end--;
                                        }
                                        if (val_start < val_end) {
                                            state = payload[val_start] - '0';
                                        }
                                    }
                                }
                            }
                            break;
                        }
                    }
                }
            }
        } else {
            if (payload_len > 0) {
                state = payload[0] - '0';
            }
        }
        
        snprintf(dbg, sizeof(dbg), ">>> Parsed state: %d (payload_len=%u, first_char=0x%02X)\r\n", 
                 state, payload_len, payload_len > 0 ? payload[0] : 0);
        send_to_debug(dbg);
        
        if (state <= 1) {
            extern void led_set_state(uint8_t state);
            extern void led_state_save(uint8_t state);
            led_set_state(state);
            led_state_save(state);
            
            snprintf(dbg, sizeof(dbg), ">>> LED set to %d\r\n", state);
            send_to_debug(dbg);
            
            if (gsm_mqtt_is_connected()) {
                char status_msg[64];
                if (state == 1) {
                    snprintf(status_msg, sizeof(status_msg), "{\"status\":\"on\"}");
                } else {
                    snprintf(status_msg, sizeof(status_msg), "{\"status\":\"off\"}");
                }
                gsm_mqtt_publish("DV_test_led_status", status_msg, 1);
                snprintf(dbg, sizeof(dbg), ">>> Published status: %s\r\n", status_msg);
                send_to_debug(dbg);
            }
        }
    }
}

int main(void) {
    rcc_config();
    systick_config();
    gsm_gpio_init();
    uart_init();
    gsm_nw_init(NULL, NULL);
    gsm_sms_send_init(NULL, NULL);
    // while(!gsm_power_on()) {}
    gsm_hardware_init();
    gsm_sms_recive_init(NULL,NULL);
    gsm_http_init(NULL, NULL);

    w25qxx_init();
    led_init();
    led_state_init();
    
    extern void mqtt_message_cb(const char *topic, const uint8_t *payload, uint16_t payload_len);
    gsm_mqtt_init(NULL, NULL);
    gsm_mqtt_set_message_callback(mqtt_message_cb);

    while(1) {
        gsm_hardware_process_urc();
        gsm_nw_process();
        
        if(gsm_nw_is_ready()){
            static bool mqtt_configured = false;
            static bool mqtt_subscribed = false;
            if (!mqtt_configured) {
                gsm_mqtt_config(NULL, "stm32_led", NULL, NULL);
                mqtt_configured = true;
            }
            gsm_mqtt_process();
            
            if (gsm_mqtt_is_connected() && !mqtt_subscribed) {
                gsm_mqtt_subscribe(MQTT_CONTROL_TOPIC, 1);
                mqtt_subscribed = true;
            }
        }

        delay_ms(100);
    }
}
