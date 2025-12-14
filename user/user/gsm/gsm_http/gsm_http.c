#include "gsm_http.h"
#include <string.h>
#include <stdio.h>
#include "flash_inter/flash.h"
#include "uart/uart.h"
#include "hardware.h"

#define TIME_OUT 10000

gsm_http_t gsm_http_ctx;
char line[256];
urc_t urc;
static volatile bool http_reading_data = false;

#define HTTP_DOWNLOAD_BUFFER_SIZE 8192
static uint8_t gsm_http_download_buffer[HTTP_DOWNLOAD_BUFFER_SIZE] = {0};
static uint32_t gsm_http_downloaded_bytes = 0;
static uint32_t gsm_http_flash_write_addr = 0;
static bool gsm_http_flash_initialized = false;
static uint8_t gsm_http_retry_count = 0;
static uint32_t gsm_http_read_offset = 0;
static uint32_t gsm_http_total_file_size = 0;
static bool gsm_http_using_range_header = false;

static bool is_timeout(uint32_t start_ms, uint32_t timeout_ms) {
    return (get_tick_ms() - start_ms) >= timeout_ms;
}

void gsm_http_init(gsm_http_cb_t http_get_done_cb, gsm_http_cb_t http_get_error_cb){
    gsm_http_ctx.url[0] = '\0';
    gsm_http_ctx.method = 0;
    gsm_http_ctx.body[0] = '\0';
    gsm_http_ctx.retry = 0;
    gsm_http_ctx.on_http_done = http_get_done_cb;
    gsm_http_ctx.on_http_error = http_get_error_cb;
    gsm_http_ctx.status = 0;
    gsm_http_ctx.datalen = 0;  
    gsm_http_ctx.time_stamp = get_tick_ms();
    gsm_http_ctx.timeout_ms = 0;
    gsm_http_ctx.step = 0;
    gsm_http_ctx.chunk_len = 512;
    gsm_http_ctx.offset = 0;
    gsm_http_ctx.total_size = 0;
    gsm_http_ctx.current_read_len = 0;
    gsm_http_ctx.raw_bytes_received = 0;
    gsm_http_ctx.http_phase = HTTP_PHASE_HTTP_INIT;
    gsm_http_downloaded_bytes = 0;
    gsm_http_retry_count = 0;
    gsm_http_flash_initialized = false;
    gsm_http_total_file_size = 0;
    gsm_http_using_range_header = false;
}

bool http_phase_init(){
    switch (gsm_http_ctx.step)
    {
    case 0:
        send_at_comand("AT+HTTPTERM\r\n");
        gsm_http_ctx.time_stamp = get_tick_ms();
        gsm_http_ctx.step = 1;
        return true;
        
    case 1:{
        bool handled = false;
        while (gsm_fetch_line(line, sizeof(line))) {
            log_raw_line(line);
            if (at_parse_line(line, &urc)) {
                if (urc.type == URC_OK || urc.type == URC_ERROR) {
                    gsm_http_ctx.step = 2;
                    gsm_http_ctx.time_stamp = get_tick_ms();
                    handled = true;
                    delete_line(line);
                    break;
                }
            }
            delete_line(line);
        }
        if (!handled && is_timeout(gsm_http_ctx.time_stamp, TIME_OUT)) {
            gsm_http_ctx.step = 2;
        }
        return true;
    }
    
    case 2:
        send_at_comand("AT+CSSLCFG=\"sslversion\",0,4\r\n");
        gsm_http_ctx.time_stamp = get_tick_ms();
        gsm_http_ctx.step = 3;
        return true;

    case 3:{
        bool handled = false;
        while (gsm_fetch_line(line, sizeof(line))) {
            log_raw_line(line);
            if (at_parse_line(line, &urc)) {
                if (urc.type == URC_OK || urc.type == URC_ERROR) {
                    gsm_http_ctx.step = 4;
                    gsm_http_ctx.time_stamp = get_tick_ms();
                    handled = true;
                    delete_line(line);
                    break;
                }
            }
            delete_line(line);
        }
        if (!handled && is_timeout(gsm_http_ctx.time_stamp, TIME_OUT)) {
            gsm_http_ctx.step = 4;
        }
        return true;
    }
    
    case 4:
        send_at_comand("AT+CSSLCFG=\"authmode\",0,0\r\n");
        gsm_http_ctx.time_stamp = get_tick_ms();
        gsm_http_ctx.step = 5;
        return true;

    case 5:{
        bool handled = false;
        while (gsm_fetch_line(line, sizeof(line))) {
            log_raw_line(line);
            if (at_parse_line(line, &urc)) {
                if (urc.type == URC_OK || urc.type == URC_ERROR) {
                    gsm_http_ctx.step = 6;
                    gsm_http_ctx.time_stamp = get_tick_ms();
                    handled = true;
                    delete_line(line);
                    break;
                }
            }
            delete_line(line);
        }
        if (!handled && is_timeout(gsm_http_ctx.time_stamp, TIME_OUT)) {
            gsm_http_ctx.step = 6;
        }
        return true;
    }
    
    case 6:
        send_at_comand("AT+HTTPINIT\r\n");
        gsm_http_ctx.time_stamp = get_tick_ms();
        gsm_http_ctx.step = 7;
        return true;

    case 7:{
        bool handled = false;
        while (gsm_fetch_line(line, sizeof(line))) {
            log_raw_line(line);
            if (at_parse_line(line, &urc)) {
                if (urc.type == URC_OK || urc.type == URC_ERROR) {
                    gsm_http_ctx.step = 8;
                    gsm_http_ctx.time_stamp = get_tick_ms();
                    handled = true;
                    delete_line(line);
                    break;
                }
            }
            delete_line(line);
        }
        if (!handled && is_timeout(gsm_http_ctx.time_stamp, TIME_OUT)) {
            gsm_http_ctx.step = 8;
        }
        return true;
    }
    
    case 8:
        send_at_comand("AT+HTTPSSL=1\r\n");
        gsm_http_ctx.time_stamp = get_tick_ms();
        gsm_http_ctx.step = 9;
        return true;

    case 9:{
        bool handled = false;
        while (gsm_fetch_line(line, sizeof(line))) {
            log_raw_line(line);
            if (at_parse_line(line, &urc)) {
                if (urc.type == URC_OK || urc.type == URC_ERROR) {
                    gsm_http_ctx.step = 10;
                    gsm_http_ctx.time_stamp = get_tick_ms();
                    handled = true;
                    delete_line(line);
                    break;
                }
            }
            delete_line(line);
        }
        if (!handled && is_timeout(gsm_http_ctx.time_stamp, TIME_OUT)) {
            gsm_http_ctx.step = 10;
        }
        return true;
    }
    
    case 10:
        gsm_http_ctx.http_phase = HTTP_PHASE_SET_URL;
        gsm_http_ctx.step = 0;
        return false;
    
    default:
        return false;
    }
}

bool http_phase_set_url(){
    switch (gsm_http_ctx.step)
    {
    case 0:
        {
            char cmd[512];
            snprintf(cmd, sizeof(cmd), "AT+HTTPPARA=\"URL\",\"%s\"\r\n", gsm_http_ctx.url);
            send_at_comand(cmd);
            gsm_http_ctx.time_stamp = get_tick_ms();
            gsm_http_ctx.step = 1;
        }
        return true;
        
    case 1:{
        bool handled = false;
        while (gsm_fetch_line(line, sizeof(line))) {
            log_raw_line(line);
            if (at_parse_line(line, &urc)) {
                if (urc.type == URC_OK || urc.type == URC_ERROR) {
                    gsm_http_ctx.http_phase = HTTP_PHASE_SEND_REQUEST;
                    gsm_http_ctx.step = 0;
                    handled = true;
                    delete_line(line);
                    break;
                }
            }
            delete_line(line);
        }
        if (!handled && is_timeout(gsm_http_ctx.time_stamp, TIME_OUT)) {
            gsm_http_ctx.http_phase = HTTP_PHASE_ERROR;
            gsm_http_ctx.step = 0;
        }
        return true;
    }
    
    default:
        return false;
    }
}

bool http_phase_chunk(){
    switch (gsm_http_ctx.step)
    {
    case 0:
        send_at_comand("AT+HTTPTERM\r\n");
        gsm_http_ctx.time_stamp = get_tick_ms();
        gsm_http_ctx.step = 1;
        return true;
        
    case 1:{
        bool handled = false;
        while (gsm_fetch_line(line, sizeof(line))) {
            log_raw_line(line);
            if (at_parse_line(line, &urc)) {
                if (urc.type == URC_OK || urc.type == URC_ERROR) {
                    gsm_http_ctx.step = 2;
                    gsm_http_ctx.time_stamp = get_tick_ms();
                    handled = true;
                    delete_line(line);
                    break;
                }
            }
            delete_line(line);
        }
        if (!handled && is_timeout(gsm_http_ctx.time_stamp, TIME_OUT)) {
            gsm_http_ctx.step = 2;
        }
        return true;
    }
    
    case 2:
        send_at_comand("AT+HTTPINIT\r\n");
        gsm_http_ctx.time_stamp = get_tick_ms();
        gsm_http_ctx.step = 3;
        return true;

    case 3:{
        bool handled = false;
        while (gsm_fetch_line(line, sizeof(line))) {
            log_raw_line(line);
            if (at_parse_line(line, &urc)) {
                if (urc.type == URC_OK || urc.type == URC_ERROR) {
                    gsm_http_ctx.step = 4;
                    gsm_http_ctx.time_stamp = get_tick_ms();
                    handled = true;
                    delete_line(line);
                    break;
                }
            }
            delete_line(line);
        }
        if (!handled && is_timeout(gsm_http_ctx.time_stamp, TIME_OUT)) {
            gsm_http_ctx.step = 4;
        }
        return true;
    }
    
    case 4:
        {
            char cmd[512];
            snprintf(cmd, sizeof(cmd), "AT+HTTPPARA=\"URL\",\"%s\"\r\n", gsm_http_ctx.url);
            send_at_comand(cmd);
            gsm_http_ctx.time_stamp = get_tick_ms();
            gsm_http_ctx.step = 5;
        }
        return true;
        
    case 5:{
        bool handled = false;
        while (gsm_fetch_line(line, sizeof(line))) {
            log_raw_line(line);
            if (at_parse_line(line, &urc)) {
                if (urc.type == URC_OK || urc.type == URC_ERROR) {
                    gsm_http_ctx.step = 6;
                    gsm_http_ctx.time_stamp = get_tick_ms();
                    handled = true;
                    delete_line(line);
                    break;
                }
            }
            delete_line(line);
        }
        if (!handled && is_timeout(gsm_http_ctx.time_stamp, TIME_OUT)) {
            gsm_http_ctx.step = 6;
        }
        return true;
    }
    
    case 6:
        {
            if (gsm_http_total_file_size == 0) {
                send_to_debug(">>> ERROR: Total file size not set\r\n");
                gsm_http_ctx.http_phase = HTTP_PHASE_ERROR;
                gsm_http_ctx.step = 0;
                return false;
            }
            
            uint32_t range_start = gsm_http_downloaded_bytes;
            uint32_t range_end = gsm_http_total_file_size - 1;
            char cmd[128];
            snprintf(cmd, sizeof(cmd), "AT+HTTPPARA=\"USERDATA\",\"Range: bytes=%lu-%lu\"\r\n",
                     (unsigned long)range_start, (unsigned long)range_end);
            send_at_comand(cmd);
            gsm_http_ctx.time_stamp = get_tick_ms();
            gsm_http_ctx.step = 7;
            gsm_http_using_range_header = true;
        }
        return true;
        
    case 7:{
        bool handled = false;
        while (gsm_fetch_line(line, sizeof(line))) {
            log_raw_line(line);
            if (at_parse_line(line, &urc)) {
                if (urc.type == URC_OK || urc.type == URC_ERROR) {
                    gsm_http_ctx.step = 8;
                    gsm_http_ctx.time_stamp = get_tick_ms();
                    handled = true;
                    delete_line(line);
                    break;
                }
            }
            delete_line(line);
        }
        if (!handled && is_timeout(gsm_http_ctx.time_stamp, TIME_OUT)) {
            gsm_http_ctx.step = 8;
        }
        return true;
    }
    
    case 8:
        send_at_comand("AT+HTTPACTION=0\r\n");
        gsm_http_ctx.time_stamp = get_tick_ms();
        gsm_http_ctx.step = 9;
        return true;
        
    case 9:{
        bool httpaction_received = false;
        
        while (gsm_fetch_line(line, sizeof(line))) {
            log_raw_line(line);
            
            if (at_parse_line(line, &urc)) {
                if (urc.type == URC_HTTPACTION) {
                    int v1, v2, v3;
                    if (sscanf(line, "+HTTPACTION: %d,%d,%d", &v1, &v2, &v3) == 3) {
                        gsm_http_ctx.status = v2;
                        gsm_http_ctx.datalen = v3;
                        httpaction_received = true;
                    }
                } else if (urc.type == URC_OK) {
                    if (httpaction_received) {
                        delete_line(line);
                        break;
                    }
                } else if (urc.type == URC_ERROR) {
                    send_to_debug(">>> ERROR: HTTPACTION failed\r\n");
                    gsm_http_ctx.http_phase = HTTP_PHASE_ERROR;
                    gsm_http_ctx.step = 0;
                    delete_line(line);
                    return false;
                }
            } else {
                if (strstr(line, "+HTTPACTION:") != NULL) {
                    int v1, v2, v3;
                    if (sscanf(line, "+HTTPACTION: %d,%d,%d", &v1, &v2, &v3) == 3) {
                        gsm_http_ctx.status = v2;
                        gsm_http_ctx.datalen = v3;
                        if (gsm_http_total_file_size == 0) {
                            gsm_http_total_file_size = v3;
                        }
                        httpaction_received = true;
                    }
                }
            }
            delete_line(line);
        }
        
        if (httpaction_received && (gsm_http_ctx.status == 200 || gsm_http_ctx.status == 206)) {
            uint32_t remaining = gsm_http_ctx.datalen - gsm_http_downloaded_bytes;
            uint32_t chunk_size = (remaining > 256) ? 256 : remaining;
            
            if (gsm_http_total_file_size > 0 && 
                gsm_http_ctx.datalen == gsm_http_total_file_size && 
                gsm_http_downloaded_bytes > 0) {
                chunk_size = (gsm_http_ctx.datalen > 256) ? 256 : gsm_http_ctx.datalen;
            }
            
            gsm_http_ctx.raw_bytes_received = 0;
            gsm_http_ctx.current_read_len = 0;
            
            char cmd[64];
            snprintf(cmd, sizeof(cmd), "AT+HTTPREAD=0,%lu\r\n", (unsigned long)chunk_size);
            send_at_comand(cmd);
            gsm_http_ctx.time_stamp = get_tick_ms();
            gsm_http_ctx.http_phase = HTTP_PHASE_READ_DATA;
            gsm_http_ctx.step = 1;
            return true;
        } else {
            send_to_debug(">>> ERROR: Failed to get HTTPACTION\r\n");
            gsm_http_ctx.http_phase = HTTP_PHASE_ERROR;
            gsm_http_ctx.step = 0;
            return false;
        }
    }
    
    default:
        return false;
    }
}

bool http_send_request(){
    switch (gsm_http_ctx.step)
    {
    case 0:
        send_at_comand("AT+HTTPACTION=0\r\n");
        gsm_http_ctx.time_stamp = get_tick_ms();
        gsm_http_ctx.step = 1;
        gsm_http_ctx.status = 0;
        gsm_http_ctx.datalen = 0;
        return true;
        
    case 1:{
        bool httpaction_received = false;
        
        while (gsm_fetch_line(line, sizeof(line))) {
            log_raw_line(line);
            
            if (at_parse_line(line, &urc)) {
                if (urc.type == URC_HTTPACTION) {
                    int v1, v2, v3;
                    if (sscanf(line, "+HTTPACTION: %d,%d,%d", &v1, &v2, &v3) == 3) {
                        gsm_http_ctx.status = v2;
                        gsm_http_ctx.datalen = v3;
                        if (gsm_http_total_file_size == 0 && v2 == 200) {
                            gsm_http_total_file_size = v3;
                        }
                        httpaction_received = true;
                    }
                } else if (urc.type == URC_OK) {
                    if (httpaction_received) {
                        delete_line(line);
                        break;
                    }
                } else if (urc.type == URC_ERROR) {
                    send_to_debug(">>> ERROR: HTTPACTION failed\r\n");
                    gsm_http_ctx.http_phase = HTTP_PHASE_ERROR;
                    gsm_http_ctx.step = 0;
                    delete_line(line);
                    return false;
                }
            } else {
                if (strstr(line, "+HTTPACTION:") != NULL) {
                    int v1, v2, v3;
                    if (sscanf(line, "+HTTPACTION: %d,%d,%d", &v1, &v2, &v3) == 3) {
                        gsm_http_ctx.status = v2;
                        gsm_http_ctx.datalen = v3;
                        if (gsm_http_total_file_size == 0 && v2 == 200) {
                            gsm_http_total_file_size = v3;
                        }
                        httpaction_received = true;
                    }
                }
            }
            delete_line(line);
        }
        
        if (httpaction_received) {
            if (gsm_http_ctx.status == 200 && gsm_http_ctx.datalen > 0) {
                if (gsm_http_ctx.datalen > HTTP_DOWNLOAD_BUFFER_SIZE) {
                    send_to_debug(">>> ERROR: File too large\r\n");
                    gsm_http_ctx.http_phase = HTTP_PHASE_ERROR;
                    gsm_http_ctx.step = 0;
                    return false;
                }
                
                gsm_http_ctx.http_phase = HTTP_PHASE_READ_DATA;
                gsm_http_ctx.step = 0;
                return true;
            } else {
                send_to_debug(">>> ERROR: HTTP status error\r\n");
                gsm_http_ctx.http_phase = HTTP_PHASE_ERROR;
                gsm_http_ctx.step = 0;
                return false;
            }
        }
        
        uint32_t elapsed = get_tick_ms() - gsm_http_ctx.time_stamp;
        if (elapsed >= 60000) {
            send_to_debug(">>> ERROR: Timeout\r\n");
            gsm_http_ctx.http_phase = HTTP_PHASE_ERROR;
            gsm_http_ctx.step = 0;
            return false;
        }
        return true;
    }
    
    default:
        return false;
    }
}

bool http_phase_read(void){
    switch (gsm_http_ctx.step)
    {
    case 0: {
        if (!gsm_http_flash_initialized) {
            extern bool flash_init(void);
            extern bool flash_erase(uint32_t addr, uint32_t len);
            
            if (!flash_init()) {
                send_to_debug(">>> ERROR: Flash init failed\r\n");
                gsm_http_ctx.http_phase = HTTP_PHASE_ERROR;
                return false;
            }
            
            uint32_t erase_size = ((gsm_http_ctx.datalen + 1023) / 1024) * 1024;
            
            if (!flash_erase(FLASH_OTA_START, erase_size)) {
                send_to_debug(">>> ERROR: Flash erase failed\r\n");
                gsm_http_ctx.http_phase = HTTP_PHASE_ERROR;
                return false;
            }
            
            gsm_http_flash_initialized = true;
        }
        
        gsm_http_downloaded_bytes = 0;
        gsm_http_ctx.raw_bytes_received = 0;
        gsm_http_ctx.current_read_len = 0;
        gsm_http_retry_count = 0;
        gsm_http_read_offset = 0;
        
        uint32_t chunk_size = (gsm_http_ctx.datalen > 256) ? 256 : gsm_http_ctx.datalen;
        char cmd[64];
        snprintf(cmd, sizeof(cmd), "AT+HTTPREAD=0,%lu\r\n", (unsigned long)chunk_size);
        send_at_comand(cmd);
        gsm_http_ctx.time_stamp = get_tick_ms();
        gsm_http_ctx.step = 1;
        return true;
    }
        
    case 1: {
        while (gsm_fetch_line(line, sizeof(line))) {
            log_raw_line(line);
            
            urc_t urc;
            
            if (raw_data_bin(line, &urc) && urc.type == URC_HTTPREAD_END) {
                delete_line(line);
                continue;
            }
            
            if (at_parse_line(line, &urc)) {
                if (urc.type == URC_OK) {
                    gsm_http_reading_data_set(false);
                    
                    uint32_t target_size = (gsm_http_total_file_size > 0) ? gsm_http_total_file_size : gsm_http_ctx.datalen;
                    
                    if (gsm_http_downloaded_bytes >= target_size) {
                        gsm_http_ctx.step = 2;
                        return true;
                    } else {
                        gsm_http_ctx.http_phase = HTTP_PHASE_CHUNK;
                        gsm_http_ctx.step = 0;
                        delete_line(line);
                        return true;
                    }
                } else if (urc.type == URC_ERROR) {
                    gsm_http_reading_data_set(false);
                    
                    uint32_t target_size = (gsm_http_total_file_size > 0) ? gsm_http_total_file_size : gsm_http_ctx.datalen;
                    if (gsm_http_downloaded_bytes >= target_size) {
                        gsm_http_ctx.step = 2;
                        return true;
                    }
                    
                    if (gsm_http_read_offset > 0) {
                        gsm_http_read_offset = 0;
                    }
                    
                    if (gsm_http_retry_count < 5) {
                        gsm_http_retry_count++;
                        gsm_http_ctx.raw_bytes_received = 0;
                        gsm_http_ctx.current_read_len = 0;
                        gsm_http_read_offset = 0;
                        
                        uint32_t remaining = gsm_http_ctx.datalen - gsm_http_downloaded_bytes;
                        uint32_t chunk_size = (remaining > 512) ? 512 : remaining;
                        
                        char cmd[64];
                        snprintf(cmd, sizeof(cmd), "AT+HTTPREAD=0,%lu\r\n", (unsigned long)chunk_size);
                        send_at_comand(cmd);
                        gsm_http_ctx.time_stamp = get_tick_ms();
                        delete_line(line);
                        return true;
                    } else {
                        send_to_debug(">>> ERROR: Too many retries\r\n");
                        gsm_http_ctx.http_phase = HTTP_PHASE_ERROR;
                        return false;
                    }
                }
            }
            delete_line(line);
        }
        
        uint32_t elapsed = get_tick_ms() - gsm_http_ctx.time_stamp;
        if (elapsed >= 60000) {
            gsm_http_reading_data_set(false);
            
            uint32_t target_size = (gsm_http_total_file_size > 0) ? gsm_http_total_file_size : gsm_http_ctx.datalen;
            if (gsm_http_downloaded_bytes >= target_size) {
                gsm_http_ctx.step = 2;
                return true;
            } else {
                send_to_debug(">>> ERROR: Timeout\r\n");
                gsm_http_ctx.http_phase = HTTP_PHASE_ERROR;
                return false;
            }
        }
        return true;
    }
    
    case 2: {
        extern bool flash_write(uint32_t addr, const uint8_t *data, uint32_t len);
        
        uint32_t written = 0;
        uint32_t addr = FLASH_OTA_START;
        
        while (written < gsm_http_downloaded_bytes) {
            uint32_t chunk_size = (gsm_http_downloaded_bytes - written > 512) ? 512 : (gsm_http_downloaded_bytes - written);
            
            if (chunk_size % 2 != 0 && (written + chunk_size) < gsm_http_downloaded_bytes) {
                chunk_size--;
            }
            
            if (!flash_write(addr, &gsm_http_download_buffer[written], chunk_size)) {
                send_to_debug(">>> ERROR: Flash write failed\r\n");
                gsm_http_ctx.http_phase = HTTP_PHASE_ERROR;
                return false;
            }
            
            addr += chunk_size;
            written += chunk_size;
        }
        
        if (written < gsm_http_downloaded_bytes) {
            if (!flash_write(addr, &gsm_http_download_buffer[written], 1)) {
                send_to_debug(">>> ERROR: Flash write failed\r\n");
                gsm_http_ctx.http_phase = HTTP_PHASE_ERROR;
                return false;
            }
            written++;
        }
        
        extern bool flash_read(uint32_t addr, uint8_t *out, uint32_t len);
        uint8_t verify_buf[256];
        bool verify_ok = true;
        
        for (uint32_t i = 0; i < gsm_http_downloaded_bytes; i += 256) {
            uint32_t read_len = (gsm_http_downloaded_bytes - i > 256) ? 256 : (gsm_http_downloaded_bytes - i);
            if (!flash_read(FLASH_OTA_START + i, verify_buf, read_len)) {
                verify_ok = false;
                break;
            }
            
            if (memcmp(verify_buf, &gsm_http_download_buffer[i], read_len) != 0) {
                verify_ok = false;
                break;
            }
        }
        
        char dbg[128];
        snprintf(dbg, sizeof(dbg), ">>> OTA complete: %lu bytes, verify=%s\r\n",
                 (unsigned long)gsm_http_downloaded_bytes,
                 verify_ok ? "OK" : "FAILED");
        send_to_debug(dbg);
        
        gsm_http_ctx.offset = gsm_http_downloaded_bytes;
        gsm_http_ctx.http_phase = HTTP_PHASE_DONE;
        gsm_http_ctx.step = 0;
        return false;
    }
    
    default:
        return false;
    }
}

void gsm_http_process(const char *url){
    if (url && url[0] != '\0' &&  gsm_http_ctx.step == 0) {
        strncpy(gsm_http_ctx.url, url, sizeof(gsm_http_ctx.url) - 1);
        gsm_http_ctx.url[sizeof(gsm_http_ctx.url) - 1] = '\0';
    }
    
    switch (gsm_http_ctx.http_phase)
    {
    case HTTP_PHASE_HTTP_INIT:
        http_phase_init();
        break;
        
    case HTTP_PHASE_SET_URL:
        http_phase_set_url();
        break;
        
    case HTTP_PHASE_SEND_REQUEST:
        http_send_request();    
        break;

    case HTTP_PHASE_READ_DATA:
        http_phase_read();
        break;
        
    case HTTP_PHASE_CHUNK:
        http_phase_chunk();
        break;
        
    case HTTP_PHASE_DONE:
        if (gsm_http_ctx.on_http_done) {
            gsm_http_ctx.on_http_done(NULL, URC_OK);
        }
        break;
        
    case HTTP_PHASE_ERROR:
        if (gsm_http_ctx.on_http_error) {
            gsm_http_ctx.on_http_error(NULL, URC_ERROR);
        }
        break;
        
    default:
        break;
    }
}

bool gsm_http_reading_data(void) {
    return http_reading_data;
}

void gsm_http_reading_data_set(bool state) {
    http_reading_data = state;
}

void gsm_http_set_current_read_len(uint32_t len) {
    gsm_http_ctx.current_read_len = len;
}

void gsm_http_handle_raw_byte(uint8_t byte) {
    if (gsm_http_downloaded_bytes >= HTTP_DOWNLOAD_BUFFER_SIZE) {
        return;
    }
    
    if (gsm_http_ctx.current_read_len > 0 && 
        gsm_http_ctx.raw_bytes_received >= gsm_http_ctx.current_read_len) {
        return;
    }
    
    if (!gsm_http_using_range_header && gsm_http_downloaded_bytes > 0) {
        if (gsm_http_ctx.raw_bytes_received < gsm_http_downloaded_bytes) {
            gsm_http_ctx.raw_bytes_received++;
            return;
        }
    }
    
    gsm_http_download_buffer[gsm_http_downloaded_bytes] = byte;
    gsm_http_downloaded_bytes++;
    gsm_http_ctx.raw_bytes_received++;
}

bool gsm_http_check_download_complete(void) {
    return (gsm_http_ctx.offset >= gsm_http_ctx.datalen);
}
