#include "gsm.h"
#include "uart.h"
#include "ringbuff.h"
#include <string.h>
#include <stdio.h>
#include "gsm_http/gsm_http.h"

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
    static uint32_t total_raw_bytes = 0;
    static uint32_t total_bytes_read = 0;
    static bool first_check = true;
    
    /* Kiểm tra raw mode status */
    if (gsm_http_reading_data() && first_check) {
        first_check = false;
        send_to_debug(">>> HW: Entering RAW mode, reading from UART...\r\n");
        
        // Debug: check UART available
        extern uint16_t uart_sim_available(void);
        uint16_t avail = uart_sim_available();
        char dbg_init[128];
        snprintf(dbg_init, sizeof(dbg_init), ">>> HW: Raw mode ON, UART available=%u bytes\r\n", avail);
        send_to_debug(dbg_init);
        
        // QUAN TRỌNG: Đọc tất cả dữ liệu còn lại trong ring buffer
        // vì có thể dữ liệu đã đến trước khi flag được set
        uint8_t buffered_data;
        uint32_t buffered_count = 0;
        while(uart_sim_read(&buffered_data, 1) > 0) {
            buffered_count++;
            total_raw_bytes++;
            
            // Tắt log chi tiết từng byte - chỉ log tổng số bytes
            // if (total_raw_bytes <= 20) {
            //     char dbg[64];
            //     snprintf(dbg, sizeof(dbg), ">>> HW RAW (buffered) #%lu: 0x%02X\r\n", 
            //              (unsigned long)total_raw_bytes, buffered_data);
            //     send_to_debug(dbg);
            // }
            
            gsm_http_handle_raw_byte(buffered_data);
        }
        
        if (buffered_count > 0) {
            char dbg[128];
            snprintf(dbg, sizeof(dbg), ">>> HW: Read %lu buffered bytes from ring buffer\r\n",
                     (unsigned long)buffered_count);
            send_to_debug(dbg);
        } else {
            send_to_debug(">>> HW: No buffered bytes found\r\n");
        }
    }
    
    /* Đọc tất cả bytes từ UART (dữ liệu mới đến) */
    uint32_t bytes_in_loop = 0;
    
    // DEBUG: Log tất cả bytes đến UART để xem module trả về gì
    static uint32_t total_bytes_logged = 0;
    static bool log_all_bytes = false;
    
    while((len = uart_sim_read(&data, 1)) > 0) {
        bytes_in_loop++;
        total_bytes_read++;
        
        /* QUAN TRỌNG: Nếu đang ở raw mode (HTTP module đã set flag), 
         * đọc TRỰC TIẾP từ UART và gọi callback, KHÔNG parse thành line */
        if (gsm_http_reading_data()) {
            total_raw_bytes++;
            
            // Tắt log chi tiết từng byte - chỉ log tổng số bytes
            // if (log_all_bytes && total_bytes_logged < 512) {
            //     char dbg[64];
            //     snprintf(dbg, sizeof(dbg), ">>> BYTE[%lu]: 0x%02X ('%c')\r\n", 
            //              (unsigned long)total_bytes_logged, 
            //              data,
            //              (data >= 32 && data < 127) ? data : '.');
            //     send_to_debug(dbg);
            //     total_bytes_logged++;
            //     
            //     if (total_bytes_logged >= 512) {
            //         send_to_debug(">>> DEBUG: Logged 512 bytes, stopping log...\r\n");
            //         log_all_bytes = false;
            //     }
            // }
            
            // Tắt log chi tiết từng byte - chỉ log tổng số bytes
            // if (total_raw_bytes <= 20) {
            //     char dbg[64];
            //     snprintf(dbg, sizeof(dbg), ">>> HW RAW #%lu: 0x%02X\r\n", 
            //              (unsigned long)total_raw_bytes, data);
            //     send_to_debug(dbg);
            // }
            
            /* Gọi callback để HTTP module xử lý byte này */
            gsm_http_handle_raw_byte(data);
            continue; // Bỏ qua parse text line - QUAN TRỌNG!
        }
        
        /* Nếu không ở raw mode, parse như text line */
        
        /* Xử lý prompt '>' */
        if(data == '>' && gsm_urc_line_index == 0) {
            gsm_urc_line_buffer[0] = '>';
            gsm_urc_line_buffer[1] = '\0';
            gsm_urc_queue_push(gsm_urc_line_buffer);
            continue;
        }
        
        /* Xử lý kết thúc dòng */
        if(data == '\r' || data == '\n') {
            if(gsm_urc_line_index > 0) {
                gsm_urc_line_buffer[gsm_urc_line_index] = '\0';
                
                /* Nếu là +HTTPREAD:, parse và set raw mode NGAY LẬP TỨC */
                if (strstr(gsm_urc_line_buffer, "+HTTPREAD:") != NULL) {
                    send_to_debug(">>> Hardware: detected +HTTPREAD:\r\n");
                    
                    // Parse +HTTPREAD: để lấy length
                    urc_t urc;
                    if (raw_data_bin(gsm_urc_line_buffer, &urc) && urc.v1 > 0) {
                        // QUAN TRỌNG: Set current_read_len TRƯỚC KHI set raw mode ON
                        extern void gsm_http_set_current_read_len(uint32_t len);
                        gsm_http_set_current_read_len((uint32_t)urc.v1);
                        
                        // Set raw mode ON NGAY LẬP TỨC
                        gsm_http_reading_data_set(true);
                        first_check = true;  // Reset để đọc buffered bytes ngay lập tức
                        
                        char dbg[128];
                        snprintf(dbg, sizeof(dbg), ">>> HW: Set raw mode ON immediately, expect %d bytes\r\n", urc.v1);
                        send_to_debug(dbg);
                        
                        // Bật log tất cả bytes để debug
                        log_all_bytes = true;
                        total_bytes_logged = 0;
                        send_to_debug(">>> DEBUG: Starting to log ALL bytes from UART after +HTTPREAD:\r\n");
                        
                        // QUAN TRỌNG: Đọc ngay tất cả bytes còn lại trong ring buffer
                        // vì có thể bytes đã đến trước khi raw mode được set
                        uint8_t buffered_data;
                        uint32_t buffered_count = 0;
                        extern void gsm_http_handle_raw_byte(uint8_t byte);
                        while(uart_sim_read(&buffered_data, 1) > 0) {
                            buffered_count++;
                            total_raw_bytes++;
                            gsm_http_handle_raw_byte(buffered_data);
                        }
                        
                        if (buffered_count > 0) {
                            char dbg2[128];
                            snprintf(dbg2, sizeof(dbg2), ">>> HW: Read %lu buffered bytes immediately after +HTTPREAD:\r\n",
                                     (unsigned long)buffered_count);
                            send_to_debug(dbg2);
                        }
                    }
                }
                
                /* Push line vào queue (bao gồm +HTTPREAD:) */
                gsm_urc_queue_push(gsm_urc_line_buffer);
                gsm_urc_line_index = 0;
            }
        }
        else {
            /* Tích lũy byte vào line buffer */
            if(gsm_urc_line_index < (GSM_URC_LINE_MAX_LEN - 1)) {
                gsm_urc_line_buffer[gsm_urc_line_index++] = data;
            }
            else {
                /* Buffer đầy, push và reset */
                gsm_urc_line_buffer[GSM_URC_LINE_MAX_LEN - 1] = '\0';
                gsm_urc_queue_push(gsm_urc_line_buffer);
                gsm_urc_line_index = 0;
            }
        }
    }
    
    /* Log debug info */
    if (gsm_http_reading_data()) {
        // Kiểm tra số bytes có sẵn trong ring buffer
        extern uint16_t uart_sim_available(void);
        uint16_t available = uart_sim_available();
        
        if (bytes_in_loop > 0) {
            char dbg[128];
            snprintf(dbg, sizeof(dbg), ">>> HW: read %lu bytes this loop, available=%u, total_raw=%lu\r\n",
                     (unsigned long)bytes_in_loop,
                     available,
                     (unsigned long)total_raw_bytes);
            send_to_debug(dbg);
        } else {
            // Chưa đọc được byte nào trong loop này
            static uint32_t empty_count = 0;
            empty_count++;
            
            // Log mỗi 50 lần để không spam
            if (empty_count % 50 == 0) {
                char dbg[128];
                snprintf(dbg, sizeof(dbg), ">>> HW: RAW mode active, available=%u, total_raw=%lu, empty_count=%lu\r\n",
                         available,
                         (unsigned long)total_raw_bytes,
                         (unsigned long)empty_count);
                send_to_debug(dbg);
            }
        }
    } else {
        first_check = true; // Reset khi thoát raw mode
        total_raw_bytes = 0;
        total_bytes_read = 0;
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

void send_to_debug(const char *data){
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

void delete_line(char *line){
    if (line != NULL) {
        memset(line, 0, strlen(line));
    }
}