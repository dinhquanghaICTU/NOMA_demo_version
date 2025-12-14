#ifndef __GSM_HTTP_H__
#define __GSM_HTTP_H__

#include <stdint.h>
#include "gsm_urc.h"
#include "hardware.h"
#include "gsm_urc.h"
#include "gsm.h"
#include <stdio.h>



typedef void (*gsm_http_cb_t)(void *ctx, urc_type_t evt);

typedef enum {
    HTTP_PHASE_HTTP_INIT,
    HTTP_PHASE_SET_URL,
    HTTP_PHASE_SEND_REQUEST,
    HTTP_PHASE_READ_DATA,
    HTTP_PHASE_CHUNK,
    HTTP_PHASE_DONE,
    HTTP_PHASE_ERROR
}http_phase_t; 


typedef struct {
    uint8_t  step;
    uint8_t  retry;
    uint32_t time_stamp;
    uint32_t timeout_ms;
    gsm_http_cb_t on_http_done;
    gsm_http_cb_t on_http_error;

    char     url[256];   
    uint8_t  method;      
    uint16_t status;      
    uint32_t datalen;     // Changed from uint16_t to uint32_t to support files > 64KB
    char     body[128];  

    uint32_t offset;
    uint32_t chunk_len;
    uint32_t total_size; 
    uint16_t current_read_len;
    uint32_t raw_bytes_received;  // Đếm bytes đã đọc trong raw mode
    http_phase_t http_phase;
} gsm_http_t;

void gsm_http_init(gsm_http_cb_t http_get_done_cb, gsm_http_cb_t http_get_error_cb);
void gsm_http_process(const char *url);

/* Raw data hook for HTTPREAD payload */
bool gsm_http_raw_active(void);
void gsm_http_raw_push(uint8_t b);
bool gsm_http_raw_complete(void);
uint32_t gsm_http_raw_got(void);
void gsm_http_raw_start(uint32_t len); /* Bật raw mode với len */
bool gsm_http_reading_data(void); /* Check xem có đang đọc dữ liệu binary không */
void gsm_http_reading_data_set(bool state); /* Set flag đọc dữ liệu binary */
void gsm_http_handle_raw_byte(uint8_t byte); /* Xử lý byte raw từ UART */
void gsm_http_set_current_read_len(uint32_t len); /* Set current_read_len và reset raw_bytes_received */


#endif // __GSM_HTTP_H__
