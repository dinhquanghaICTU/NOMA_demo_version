#ifndef __GSM_MQTT_TCP_H__
#define __GSM_MQTT_TCP_H__

//#include "gsm_tcp/gsm_tcp.h"
#include <stdint.h>
#include <stdbool.h>

typedef void (*mqtt_tcp_msg_cb_t)(const char *topic, const uint8_t *payload, uint16_t payload_len);

typedef struct {
    char broker[128];
    uint16_t port;
    char client_id[64];
    char username[64];
    char password[64];
    uint16_t keep_alive;
    bool is_connected;
    mqtt_tcp_msg_cb_t on_message;
} gsm_mqtt_tcp_ctx_t;

void gsm_mqtt_tcp_init(void);
void gsm_mqtt_tcp_connect(const char *broker, uint16_t port, const char *client_id,
                         const char *username, const char *password, uint16_t keep_alive,
                         mqtt_tcp_msg_cb_t on_message);
void gsm_mqtt_tcp_subscribe(const char *topic, uint8_t qos);
void gsm_mqtt_tcp_process(void);
bool gsm_mqtt_tcp_is_connected(void);

#endif //__GSM_MQTT_TCP_H__

