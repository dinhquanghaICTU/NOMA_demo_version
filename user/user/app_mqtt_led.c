#include "gsm_mqtt/gsm_mqtt.h"
#include "led/led.h"
#include "w25Qx/w25Qx.h"
#include "gsm.h"
#include "gsm_network/gsm_nw.h"
#include <stdio.h>
#include <string.h>

#define MQTT_TOPIC_LED_CONTROL "DV_test_led"

static void mqtt_message_cb(const char *topic, const uint8_t *payload, uint16_t payload_len) {
    char dbg[256];
    snprintf(dbg, sizeof(dbg), ">>> MQTT msg: topic=%s, len=%u\r\n", topic ? topic : "NULL", payload_len);
    send_to_debug(dbg);
    
    if (!topic || !payload || payload_len == 0) {
        send_to_debug(">>> MQTT msg: invalid params\r\n");
        return;
    }
    
    if (strcmp(topic, MQTT_TOPIC_LED_CONTROL) == 0) {
        uint8_t state = 0;
        
        if (payload[0] == '{') {
            const char *msg_start = strstr((const char *)payload, "\"message\"");
            if (msg_start) {
                const char *val_start = strchr(msg_start, ':');
                if (val_start) {
                    val_start++;
                    while (*val_start == ' ' || *val_start == '"') val_start++;
                    state = *val_start - '0';
                }
            }
        } else {
            state = payload[0] - '0';
        }
        
        snprintf(dbg, sizeof(dbg), ">>> Parsed state: %d\r\n", state);
        send_to_debug(dbg);
        
        if (state <= 1) {
            led_set_state(state);
            led_state_save(state);
            
            snprintf(dbg, sizeof(dbg), ">>> LED set to %d\r\n", state);
            send_to_debug(dbg);
        }
    } else {
        snprintf(dbg, sizeof(dbg), ">>> Topic mismatch: %s != %s\r\n", topic, MQTT_TOPIC_LED_CONTROL);
        send_to_debug(dbg);
    }
}

static void mqtt_done_cb(void *ctx, urc_type_t evt) {
    send_to_debug(">>> MQTT done callback\r\n");
}

static void mqtt_error_cb(void *ctx, urc_type_t evt) {
    send_to_debug(">>> MQTT error callback\r\n");
}

void app_mqtt_led_init(void) {
    w25qxx_init();
    led_init();
    led_state_init();
    
    gsm_mqtt_init(mqtt_done_cb, mqtt_error_cb);
    gsm_mqtt_set_message_callback(mqtt_message_cb);
}

void app_mqtt_led_process(void) {
    static bool configured = false;
    static bool subscribed = false;
    static uint32_t last_log = 0;
    
    gsm_mqtt_process();
    
    if (gsm_nw_is_ready() && !configured) {
        send_to_debug(">>> MQTT: Network ready, configuring...\r\n");
        gsm_mqtt_config(
            "ssl://f6fa5bcc03514f218dfb2b84d9c1925b.s1.eu.hivemq.cloud:8883",  // Full URL with protocol
            "stm32_client_001",
            "imbdang",
            "Abc12345"
        );
        configured = true;
    }
    
    if (gsm_mqtt_is_connected()) {
        if (!subscribed) {
            send_to_debug(">>> MQTT connected, subscribing...\r\n");
            gsm_mqtt_subscribe(MQTT_TOPIC_LED_CONTROL, 0);
            subscribed = true;
        }
    } else {
        if (get_tick_ms() - last_log > 5000) {
            send_to_debug(">>> MQTT not connected yet...\r\n");
            last_log = get_tick_ms();
        }
    }
}
