#ifndef __GSM_H__
#define __GSM_H__

#include <stdint.h>
#include <stdbool.h>

void gsm_hardware_init(void);
void gsm_hardware_process_urc(void);
bool gsm_hardware_urc_get_line(char *line, uint16_t max_len);
uint8_t gsm_hardware_urc_available(void);
void gsm_hardware_urc_reset(void);
void gsm_hardware_send(uint8_t *data, uint16_t len);
void gsm_hardware_send_byte(uint8_t data);
void send_at_comand(const char *data);
void send_to_debug(char *data);
void connect_network(void);
bool gsm_fetch_line(char *buf, uint16_t len);
void log_raw_line(const char *line);
void delete_line(const char *line);

#endif // __GSM_H__

