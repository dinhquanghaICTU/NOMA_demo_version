#include "debug.h"

void get_response(void) {
    char line[128];
    if (gsm_hardware_urc_available() > 0) {
        if (gsm_hardware_urc_get_line(line, sizeof(line))) {
            uart_send_string("SIM: ", 5);
            uart_send_string(line, strlen(line));
            uart_send_string("\r\n", 2);
        }
    }
}

