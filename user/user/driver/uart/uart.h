#ifndef __UART_H__
#define __UART_H__

#include "stm32f10x_usart.h"
#include "hardware.h"

void uart_init(void);
void uart_send_byte(char data);
void uart_send_string(char *data, uint16_t length);

void uart_debug_rx_handler(uint8_t data);
void uart_sim_rx_handler(uint8_t data);

uint16_t uart_debug_read(uint8_t *data, uint16_t len);
uint16_t uart_sim_read(uint8_t *data, uint16_t len);

uint16_t uart_debug_available(void);
uint16_t uart_sim_available(void);

void uart_sim_send_byte(uint8_t data);
void uart_sim_send_string(char *data, uint16_t length);



#endif // __UART_H__