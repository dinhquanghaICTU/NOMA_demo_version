#ifndef __GSM_MQTT_H__
#define __GSM_MQTT_H__

#include "gsm_urc.h"
#include <stdint.h>
#include <string.h>
#include "hardware.h"
#include "gsm.h"

#define MQTT_URL                "tcp://a907cd687c3b4309adeb32fd3bf1c2b6.s1.eu.hivemq.cloud:8883"
#define MQTT_USER               "quanghaictu"
#define MQTT_PASS               "Quangha123456"
#define MQTT_CONTROL_TOPIC      "DV_test_led"

typedef void (*gsm_mqtt_cb_t)(void *ctx, urc_type_t evt);
typedef void (*gsm_mqtt_msg_cb_t)(const char *topic, const uint8_t *payload, uint16_t payload_len);

typedef enum {
    MQTT_PHASE_STOP,
    MQTT_PHASE_START,       // CMQTTSTART
    MQTT_PHASE_ACCQ,        // CMQTTACCQ
    MQTT_PHASE_SSL_VER,     // AT+CSSLCFG="sslversion"
    MQTT_PHASE_SSL_SNI,     // AT+CSSLCFG="enableSNI"
    MQTT_PHASE_SSL_AUTH,    // AT+CSSLCFG="authmode"
    MQTT_PHASE_SSL_TLS,     // AT+CMQTTSSLCFG
    MQTT_PHASE_CONN,        // CMQTTCONNECT
    MQTT_PHASE_IDLE,        // đã kết nối, chờ publish/subscribe
    MQTT_PHASE_PUB,         // đang publish (TOPIC/PAYLOAD/PUB)
    MQTT_PHASE_SUB,         // đang subscribe (nếu dùng)
    MQTT_PHASE_DONE,        // hoàn tất một yêu cầu
    MQTT_PHASE_ERROR
}gsm_mqtt_t; 


typedef struct {
    char broker_url[128];
    uint16_t port;
    char client_id[64];
    char username[64];
    char password[64];
    uint8_t keep_alive;
} mqtt_config_t;

typedef struct {
    char topic[128];
    uint8_t *payload;
    uint16_t payload_len;
    uint8_t qos;
    uint8_t retain;
} mqtt_message_t;

typedef struct {
    uint8_t step;
    mqtt_config_t config;
    mqtt_message_t message;
    gsm_mqtt_t phase;
    uint8_t retry;
    uint32_t time_stamp;
    uint32_t timeout_ms;
    uint8_t is_connected;
    
    gsm_mqtt_cb_t on_mqtt_done;
    gsm_mqtt_cb_t on_mqtt_error;
    gsm_mqtt_msg_cb_t on_mqtt_message;
    
} gsm_mqtt_context_t;

void gsm_mqtt_init(gsm_mqtt_cb_t on_mqtt_done_cb, gsm_mqtt_cb_t on_mqtt_error_cb);
void gsm_mqtt_process(void);
void gsm_mqtt_config(const char *broker_url_full, const char *client_id, const char *username, const char *password);
void gsm_mqtt_subscribe(const char *topic, uint8_t qos);
void gsm_mqtt_publish(const char *topic, const char *payload, uint8_t qos);
bool gsm_mqtt_is_connected(void);
void gsm_mqtt_set_message_callback(gsm_mqtt_msg_cb_t callback);
void gsm_mqtt_handle_urc(const char *line);

#endif //__GSM_MQTT_H__