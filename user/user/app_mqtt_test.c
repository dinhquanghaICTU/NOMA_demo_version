#include "gsm_mqtt/gsm_mqtt.h"
#include "gsm.h"
#include "gsm_network/gsm_nw.h"
#include <stdio.h>
#include <string.h>

static void mqtt_test_done_cb(void *ctx, urc_type_t evt) {
    send_to_debug(">>> MQTT TEST: Done callback\r\n");
}

static void mqtt_test_error_cb(void *ctx, urc_type_t evt) {
    send_to_debug(">>> MQTT TEST: Error callback\r\n");
}

void app_mqtt_test_init(void) {
    send_to_debug(">>> MQTT TEST: Initializing...\r\n");
    gsm_mqtt_init(mqtt_test_done_cb, mqtt_test_error_cb);
}

//void app_mqtt_test_process(void) {
//    static bool configured = false;
//    static bool test_started = false;
//    static uint32_t last_log = 0;
//    static uint32_t network_ready_time = 0;
//
//    if (!test_started && gsm_nw_is_ready()) {
//        if (network_ready_time == 0) {
//            network_ready_time = get_tick_ms();
//        }
//
//        uint32_t now = get_tick_ms();
//        if (now - network_ready_time > 3000) {  // Wait 3 seconds after network ready
//            if (!configured) {
//                send_to_debug(">>> MQTT TEST: Configuring...\r\n");
//                gsm_mqtt_config(
//                    NULL,
//                    "stm32_test_001",
//                    NULL,
//                    NULL
//                );
//                configured = true;
//                test_started = true;
//            }
//        }
//    }
//
//    gsm_mqtt_process();
//
//    if (get_tick_ms() - last_log > 2000) {
//        if (gsm_mqtt_is_connected()) {
//            send_to_debug(">>> MQTT TEST: CONNECTED!\r\n");
//        } else {
//            char dbg[128];
//            extern gsm_mqtt_context_t gsm_mqtt_ctx;
//            snprintf(dbg, sizeof(dbg), ">>> MQTT TEST: Not connected (phase=%d, step=%d)\r\n",
//                     gsm_mqtt_ctx.phase, gsm_mqtt_ctx.step);
//            send_to_debug(dbg);
//        }
//        last_log = get_tick_ms();
//    }
//}
//
