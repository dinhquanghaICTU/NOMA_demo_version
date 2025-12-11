#include "gsm.h"
#include "uart.h"
#include "ringbuff.h"
#include <string.h>

#define GSM_URC_QUEUE_SIZE 10
#define GSM_URC_LINE_MAX_LEN 128

typedef struct {
    char lines[GSM_URC_QUEUE_SIZE][GSM_URC_LINE_MAX_LEN];
    uint8_t head;
    uint8_t tail;
    uint8_t count;
} gsm_urc_queue_t;

static gsm_urc_queue_t gsm_urc_queue;

static char gsm_urc_line_buffer[GSM_URC_LINE_MAX_LEN];
static uint16_t gsm_urc_line_index = 0;

void gsm_hardware_init(void) {
    memset(&gsm_urc_queue, 0, sizeof(gsm_urc_queue_t));
    gsm_urc_line_index = 0;
}

static bool gsm_urc_queue_push(const char *line) {
    if(gsm_urc_queue.count >= GSM_URC_QUEUE_SIZE) {
        return false;
    }
    
    strncpy(gsm_urc_queue.lines[gsm_urc_queue.head], line, GSM_URC_LINE_MAX_LEN - 1);
    gsm_urc_queue.lines[gsm_urc_queue.head][GSM_URC_LINE_MAX_LEN - 1] = '\0';
    
    gsm_urc_queue.head = (gsm_urc_queue.head + 1) % GSM_URC_QUEUE_SIZE;
    gsm_urc_queue.count++;
    
    return true;
}

static bool gsm_urc_queue_pop(char *line, uint16_t max_len) {
    if(gsm_urc_queue.count == 0) {
        return false;
    }
    
    strncpy(line, gsm_urc_queue.lines[gsm_urc_queue.tail], max_len - 1);
    line[max_len - 1] = '\0';
    
    gsm_urc_queue.tail = (gsm_urc_queue.tail + 1) % GSM_URC_QUEUE_SIZE;
    gsm_urc_queue.count--;
    
    return true;
}

void gsm_hardware_process_urc(void) {
    uint8_t data;
    uint16_t len;
    
    while((len = uart_sim_read(&data, 1)) > 0) {
        if(data == '>' && gsm_urc_line_index == 0) {
            gsm_urc_line_buffer[0] = '>';
            gsm_urc_line_buffer[1] = '\0';
            gsm_urc_queue_push(gsm_urc_line_buffer);
            continue;
        }
        
        if(data == '\r' || data == '\n') {
            if(gsm_urc_line_index > 0) {
                gsm_urc_line_buffer[gsm_urc_line_index] = '\0';
                gsm_urc_queue_push(gsm_urc_line_buffer);
                gsm_urc_line_index = 0;
            }
        }
        else {
            if(gsm_urc_line_index < (GSM_URC_LINE_MAX_LEN - 1)) {
                gsm_urc_line_buffer[gsm_urc_line_index++] = data;
            }
            else {
                gsm_urc_line_buffer[GSM_URC_LINE_MAX_LEN - 1] = '\0';
                gsm_urc_queue_push(gsm_urc_line_buffer);
                gsm_urc_line_index = 0;
            }
        }
    }
}

bool gsm_hardware_urc_get_line(char *line, uint16_t max_len) {
    return gsm_urc_queue_pop(line, max_len);
}

uint8_t gsm_hardware_urc_available(void) {
    return gsm_urc_queue.count;
}

void gsm_hardware_urc_reset(void) {
    gsm_urc_queue.head = 0;
    gsm_urc_queue.tail = 0;
    gsm_urc_queue.count = 0;
    gsm_urc_line_index = 0;
}

void gsm_hardware_send(uint8_t *data, uint16_t len) {
    uart_sim_send_string((char*)data, len);
}

void send_at_comand(const char *data){
    uart_sim_send_string((char*)data, strlen(data));
}

void send_to_debug(char *data){
    uint16_t i;
    for(i = 0; i < strlen(data); i++) {
        uart_send_byte(data[i]);
    }
}


void gsm_hardware_send_byte(uint8_t data) {
    uart_sim_send_byte(data);
}


bool gsm_fetch_line(char *buf, uint16_t len){
    if (!buf || len == 0) return false;
    if (!gsm_hardware_urc_available()) return false;
    if (!gsm_hardware_urc_get_line(buf, len)) return false;
    return true;
}


void log_raw_line(const char *line) {
    send_to_debug(line);
    send_to_debug("\r\n");
}