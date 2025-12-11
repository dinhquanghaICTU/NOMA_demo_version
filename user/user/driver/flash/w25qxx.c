#include "w25qxx.h"

static void w25q64_send_cmd(uint8_t cmd) {
    SPI_FLASH_CS_LOW();
    spi_transfer(cmd);
    SPI_FLASH_CS_HIGH();
}

void w25q64_init(void) {
    spi_init();
    delay_ms(10);
    w25q64_read_id();
}

void w25q64_write_enable(void) {
    w25q64_send_cmd(W25Q64_CMD_WRITE_ENABLE);
}

void w25q64_wait_busy(void) {
    uint8_t status;
    do {
        SPI_FLASH_CS_LOW();
        spi_transfer(W25Q64_CMD_READ_STATUS_REG);
        status = spi_transfer(0xFF);
        SPI_FLASH_CS_HIGH();
    } while (status & 0x01);
}

uint32_t w25q64_read_id(void) {
    uint32_t id = 0;
    SPI_FLASH_CS_LOW();
    spi_transfer(W25Q64_CMD_READ_ID);
    id = (uint32_t)spi_transfer(0xFF) << 16;
    id |= (uint32_t)spi_transfer(0xFF) << 8;
    id |= (uint32_t)spi_transfer(0xFF);
    SPI_FLASH_CS_HIGH();
    return id;
}

void w25q64_erase_sector(uint32_t addr) {
    w25q64_write_enable();
    SPI_FLASH_CS_LOW();
    spi_transfer(W25Q64_CMD_SECTOR_ERASE);
    spi_transfer((addr >> 16) & 0xFF);
    spi_transfer((addr >> 8) & 0xFF);
    spi_transfer(addr & 0xFF);
    SPI_FLASH_CS_HIGH();
    w25q64_wait_busy();
}

void w25q64_write_page(uint32_t addr, uint8_t *data, uint16_t len) {
    if (len > W25Q64_PAGE_SIZE) len = W25Q64_PAGE_SIZE;
    
    w25q64_write_enable();
    SPI_FLASH_CS_LOW();
    spi_transfer(W25Q64_CMD_PAGE_PROGRAM);
    spi_transfer((addr >> 16) & 0xFF);
    spi_transfer((addr >> 8) & 0xFF);
    spi_transfer(addr & 0xFF);
    
    for (uint16_t i = 0; i < len; i++) {
        spi_transfer(data[i]);
    }
    SPI_FLASH_CS_HIGH();
    w25q64_wait_busy();
}

void w25q64_write(uint32_t addr, uint8_t *data, uint16_t len) {
    uint16_t page_offset;
    uint16_t write_len;
    uint32_t current_addr = addr;
    uint16_t remaining = len;
    
    while (remaining > 0) {
        page_offset = current_addr % W25Q64_PAGE_SIZE;
        write_len = W25Q64_PAGE_SIZE - page_offset;
        if (write_len > remaining) write_len = remaining;
        
        if (page_offset == 0 && write_len == W25Q64_PAGE_SIZE) {
            w25q64_write_page(current_addr, data, write_len);
        } else {
            uint8_t page_buffer[W25Q64_PAGE_SIZE];
            w25q64_read((current_addr / W25Q64_PAGE_SIZE) * W25Q64_PAGE_SIZE, page_buffer, W25Q64_PAGE_SIZE);
            for (uint16_t i = 0; i < write_len; i++) {
                page_buffer[page_offset + i] = data[i];
            }
            w25q64_write_page((current_addr / W25Q64_PAGE_SIZE) * W25Q64_PAGE_SIZE, page_buffer, W25Q64_PAGE_SIZE);
        }
        
        data += write_len;
        current_addr += write_len;
        remaining -= write_len;
    }
}

void w25q64_read(uint32_t addr, uint8_t *data, uint16_t len) {
    SPI_FLASH_CS_LOW();
    spi_transfer(W25Q64_CMD_READ_DATA);
    spi_transfer((addr >> 16) & 0xFF);
    spi_transfer((addr >> 8) & 0xFF);
    spi_transfer(addr & 0xFF);
    
    for (uint16_t i = 0; i < len; i++) {
        data[i] = spi_transfer(0xFF);
    }
    SPI_FLASH_CS_HIGH();
}

