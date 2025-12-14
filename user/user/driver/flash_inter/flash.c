#include "flash.h"
#include "stm32f10x_flash.h"
#include "stm32f10x.h"

static bool flash_is_unlocked = false;

bool flash_init(void) {
    if (!flash_is_unlocked) {
        FLASH_Unlock();
        flash_is_unlocked = true;
    }
    return true;
}

bool flash_erase(uint32_t addr, uint32_t len) {
    if (!flash_is_unlocked) {
        flash_init();
    }
    
    if (addr < FLASH_OTA_START || (addr + len) > FLASH_OTA_END) {
        return false; 
    }
    
    // Tính toán page cần erase
    uint32_t start_page = (addr / FLASH_PAGE_SIZE) * FLASH_PAGE_SIZE;
    uint32_t end_addr = addr + len;
    uint32_t end_page = ((end_addr + FLASH_PAGE_SIZE - 1) / FLASH_PAGE_SIZE) * FLASH_PAGE_SIZE;
    
    FLASH_Status status;
    uint32_t page_addr = start_page;
    while (page_addr < end_page) {
        status = FLASH_ErasePage(page_addr);
        if (status != FLASH_COMPLETE) {
            return false;
        }
        page_addr += FLASH_PAGE_SIZE;
    }
    
    return true;
}

bool flash_write(uint32_t addr, const uint8_t *data, uint32_t len) {
    // Đảm bảo flash được unlock
    if (!flash_is_unlocked) {
        if (!flash_init()) {
            return false;  // Không thể unlock flash
        }
    }
    
    // Kiểm tra address và length
    if (addr < FLASH_OTA_START || (addr + len) > FLASH_OTA_END) {
        return false; 
    }
    
    // Kiểm tra address align với halfword (2 bytes)
    if ((addr % 2) != 0) {
        return false;  // Address không align với halfword
    }
    
    if (data == NULL || len == 0) {
        return false;
    }
    
    // KHÔNG TỰ ĐỘNG ERASE - vùng đã được erase ở bước init
    // Chỉ ghi trực tiếp
    
    FLASH_Status status;
    uint32_t write_addr = addr;
    const uint8_t *src = data;
    uint32_t remaining = len;
    
    // Ghi theo halfword (STM32F103 yêu cầu ghi 16-bit)
    while (remaining > 0) {
        uint16_t halfword;
        
        if (remaining >= 2) {
            // Ghi 2 byte cùng lúc
            halfword = (uint16_t)(src[0] | (src[1] << 8));
            status = FLASH_ProgramHalfWord(write_addr, halfword);
            if (status != FLASH_COMPLETE) {
                return false;
            }
            
            // Verify ngay sau khi ghi
            uint16_t readback = *((volatile uint16_t *)write_addr);
            if (readback != halfword) {
                return false; // Ghi sai
            }
            
            write_addr += 2;
            src += 2;
            remaining -= 2;
        } else {
            // Byte lẻ cuối: ghi kèm 0xFF
            halfword = (uint16_t)(src[0] | 0xFF00);
            status = FLASH_ProgramHalfWord(write_addr, halfword);
            if (status != FLASH_COMPLETE) {
                return false;
            }
            
            // Verify
            uint16_t readback = *((volatile uint16_t *)write_addr);
            if ((readback & 0xFF) != src[0]) {
                return false;
            }
            
            remaining = 0;
        }
    }
    
    return true;
}

bool flash_read(uint32_t addr, uint8_t *out, uint32_t len) {
    if (addr < FLASH_OTA_START || (addr + len) > FLASH_OTA_END) {
        return false;
    }
    
    if (out == NULL || len == 0) {
        return false;
    }
    
    const uint8_t *src = (const uint8_t *)addr;
    for (uint32_t i = 0; i < len; i++) {
        out[i] = src[i];
    }
    
    return true;
}