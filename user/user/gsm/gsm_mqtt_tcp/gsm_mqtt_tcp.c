// This module is disabled - using MQTT AT commands instead (gsm_mqtt module)
// File kept for compatibility but all functions are stubs

#include "gsm_mqtt_tcp.h"
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

gsm_mqtt_tcp_ctx_t gsm_mqtt_tcp_ctx;

void gsm_mqtt_tcp_init(void) {
    // Stub - using gsm_mqtt instead
}

void gsm_mqtt_tcp_connect(const char *broker, uint16_t port, const char *client_id,
                          const char *username, const char *password, uint16_t keep_alive,
                          void (*on_message_cb)(const char *topic, const uint8_t *payload, uint16_t payload_len)) {
    // Stub - using gsm_mqtt instead
    (void)broker;
    (void)port;
    (void)client_id;
    (void)username;
    (void)password;
    (void)keep_alive;
    (void)on_message_cb;
}

void gsm_mqtt_tcp_subscribe(const char *topic, uint8_t qos) {
    // Stub - using gsm_mqtt instead
    (void)topic;
    (void)qos;
}

void gsm_mqtt_tcp_process(void) {
    // Stub - using gsm_mqtt instead
}

bool gsm_mqtt_tcp_is_connected(void) {
    return false; // Stub - using gsm_mqtt instead
}
