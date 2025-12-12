#ifndef __GSM_MQTT_H__
#define __GSM_MQTT_H__

#include "gsm_urc.h"
#include <stdint.h>
#include <string.h>
#include "hardware.h"
#include "gsm.h"

typedef void (*gsm_mqtt_cb_t)(void *ctx, urc_type_t evt);

typedef enum {
    MQTT_PHASE_START,       // CMQTTSTART
    MQTT_PHASE_CONN,        // CMQTTACCQ + CMQTTCONNECT
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
    // char username[64];
    // char password[64];
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
    
} gsm_mqtt_context_t;

void gsm_mqtt_init(gsm_mqtt_cb_t on_mqtt_done_cb, gsm_mqtt_cb_t on_mqtt_error_cb);
void gsm_mqtt_process();

#endif //__GSM_MQTT_H__