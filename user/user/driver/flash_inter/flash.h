#ifndef __FLASH_H__
#define __FLASH_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>


#define FLASH_PAGE_SIZE        1024U


#define FLASH_BASE_APP         0x08011000U   /* Page 20 để test OTA */
#define FLASH_OTA_START        FLASH_BASE_APP
#define FLASH_OTA_MAX_SIZE     (64 * 1024U)  /* tùy chỉnh lại nếu cần lớn hơn */
#define FLASH_OTA_END          (FLASH_OTA_START + FLASH_OTA_MAX_SIZE)

bool flash_init(void);
bool flash_erase(uint32_t addr, uint32_t len);
bool flash_write(uint32_t addr, const uint8_t *data, uint32_t len);
bool flash_read(uint32_t addr, uint8_t *out, uint32_t len);

#endif //__FLASH_H__
