//  ***************************************************************************
/// @file    veeprom.c
/// @author  NeoProg
//  ***************************************************************************
#include "veeprom.h"
#include "project_base.h"
#define FLASH_PAGE_SIZE                     (1024)
#define VEEPROM_SERVICE_HEADER_SIZE         (10)
#define VEEPROM_PAGE_1_ADDR                 (0x08003800)
#define VEEPROM_PAGE_2_ADDR                 (0x08003C00)
#define VEEPROM_PAGE_SIZE                   (FLASH_PAGE_SIZE - VEEPROM_SERVICE_HEADER_SIZE)

#define PAGE_CHECKSUM_OFFSET                (VEEPROM_PAGE_SIZE)

#define PAGE_STATE_OFFSET                   (VEEPROM_PAGE_SIZE + 2)
#define PAGE_STATE_INVALID                  ((uint64_t)(0x0000000000000000))
#define PAGE_STATE_COPY                     ((uint64_t)(0x000000000000FFFF))
#define PAGE_STATE_VALID                    ((uint64_t)(0x00000000FFFFFFFF))
#define PAGE_STATE_WRITE                    ((uint64_t)(0x0000FFFFFFFFFFFF))
#define PAGE_STATE_ERASED                   ((uint64_t)(0xFFFFFFFFFFFFFFFF))


static uint32_t active_page_addr = 0;
static uint32_t inactive_page_addr = 0;


static bool flash_lock();
static bool flash_unlock();
static bool flash_wait_and_check();
static bool flash_page_erase(uint32_t flash_addr);

static uint64_t flash_page_get_state(uint32_t flash_addr);
static bool     flash_page_set_state(uint32_t flash_addr, uint64_t state);

static uint16_t flash_page_calc_checksum(uint32_t flash_addr);
static uint16_t flash_page_read_checksum(uint32_t flash_addr);
static bool     flash_page_write_checksum(uint32_t flash_addr, uint16_t checksum);

static uint8_t  flash_read_8(uint32_t flash_addr);
static uint16_t flash_read_16(uint32_t flash_addr);
static uint32_t flash_read_32(uint32_t flash_addr);
static bool     flash_write_16(uint32_t flash_addr, uint16_t value);



//  ***************************************************************************
/// @brief  VEEPROM driver initializetion
/// @return true - init success, false - fail
//  ***************************************************************************
bool veeprom_init() {
    // Search active page
    uint64_t page1_state = flash_page_get_state(VEEPROM_PAGE_1_ADDR);
    uint64_t page2_state = flash_page_get_state(VEEPROM_PAGE_2_ADDR);
    if (page1_state == PAGE_STATE_VALID) {
        active_page_addr = VEEPROM_PAGE_1_ADDR;
        inactive_page_addr = VEEPROM_PAGE_2_ADDR;
    } 
    else if (page2_state == PAGE_STATE_VALID) {
        active_page_addr = VEEPROM_PAGE_2_ADDR;
        inactive_page_addr = VEEPROM_PAGE_1_ADDR;
    } 
    else if (page1_state == PAGE_STATE_COPY) {
        active_page_addr = VEEPROM_PAGE_1_ADDR;
        inactive_page_addr = VEEPROM_PAGE_2_ADDR;
    }
    else if (page2_state == PAGE_STATE_COPY) {
        active_page_addr = VEEPROM_PAGE_2_ADDR;
        inactive_page_addr = VEEPROM_PAGE_1_ADDR;
    }
    else {
        if (!flash_page_erase(VEEPROM_PAGE_1_ADDR)) {
            return false;
        }
        active_page_addr = VEEPROM_PAGE_1_ADDR;
        inactive_page_addr = VEEPROM_PAGE_2_ADDR;
        return true;
    }
    
    // Check checksum
    return flash_page_read_checksum(active_page_addr) != flash_page_calc_checksum(active_page_addr);
}

//  ***************************************************************************
/// @brief  Mass erase VEEPROM
/// @return true - init success, false - fail
//  ***************************************************************************
bool veeprom_mass_erase() {
    return flash_page_erase(VEEPROM_PAGE_1_ADDR) && flash_page_erase(VEEPROM_PAGE_2_ADDR);
}

//  ***************************************************************************
/// @brief  Read data from VEEPROM
/// @param  [in] veeprom_addr: virtual address [0x0000...size-1]
/// @param  [out] buffer: pointer to buffer for data
/// @param  [in] bytes_count: bytes count for read
/// @return true - init success, false - fail
//  ***************************************************************************
bool veeprom_read(uint32_t veeprom_addr, uint8_t* buffer, uint32_t bytes_count) {
    if (veeprom_addr + bytes_count >= VEEPROM_PAGE_SIZE || !active_page_addr) {
        return false;
    }
    while (bytes_count) {
        *buffer = flash_read_8(active_page_addr + veeprom_addr);
        ++buffer;
        --bytes_count;
    }
    return true;
}
uint8_t veeprom_read_8(uint32_t veeprom_addr) {
    uint8_t data = 0;
    veeprom_read(veeprom_addr, &data, sizeof(data));
    return data;
}
uint16_t veeprom_read_16(uint32_t veeprom_addr) {
    uint16_t data = 0;
    veeprom_read(veeprom_addr, (uint8_t*)&data, sizeof(data));
    return data;
}
uint32_t veeprom_read_32(uint32_t veeprom_addr) {
    uint32_t data = 0;
    veeprom_read(veeprom_addr, (uint8_t*)&data, sizeof(data));
    return data;
}

//  ***************************************************************************
/// @brief  Write data to VEEPROM
/// @param  [in] veeprom_addr: virtual address [0x0000...size-1]
/// @param  [out] data: pointer to data for write
/// @param  [in] bytes_count: bytes count for write
/// @return true - init success, false - fail
//  ***************************************************************************
bool veeprom_write(uint32_t veeprom_addr, uint8_t* data, uint32_t bytes_count) {
    // Erase inactive page (set ERASED state)
    if (!flash_page_erase(inactive_page_addr)) {
        return false;
    }
    
    flash_unlock();
    
    // Set COPY state for active page
    if (!flash_page_set_state(active_page_addr, PAGE_STATE_COPY)) {
        flash_lock();
        return false;
    }
    
    // Set WRITE state for inactive page
    if (!flash_page_set_state(inactive_page_addr, PAGE_STATE_WRITE)) {
        flash_lock();
        return false;
    }
    
    // Copy data from active page into inactive with change data
    for (uint32_t offset = 0; offset < VEEPROM_PAGE_SIZE; /* NONE */) {
        uint8_t byte[2] = {0};
        for (uint32_t i = 0; i < 2; ++i) {
            if (offset >= veeprom_addr && offset < veeprom_addr + bytes_count) {
                byte[i] = *data;
                ++data;
            } else {
                byte[i] = flash_read_8(active_page_addr + offset);
            }
            ++offset;
        }
        uint16_t word = ((byte[0] << 8) & 0xFF00) | byte[1];
        if (word != flash_read_16(inactive_page_addr + offset - 2)) {
            // Write data
            if (!flash_write_16(inactive_page_addr + offset - 2, word)) {
                flash_lock();
                return false;
            }
        }
    }
    
    // Calc checksum for inactive page
    uint16_t checksum = flash_page_calc_checksum(inactive_page_addr);
    if (!flash_page_write_checksum(inactive_page_addr, checksum)) {
        flash_lock();
        return false;
    }
    
    // Set VALID state for inactive page
    if (!flash_page_set_state(inactive_page_addr, PAGE_STATE_VALID)) {
        flash_lock();
        return false;
    }
    
    // Set INVALID state for active page
    if (!flash_page_set_state(active_page_addr, PAGE_STATE_INVALID)) {
        flash_lock();
        return false;
    }
    
    // Swap pages
    uint32_t tmp = inactive_page_addr;
    inactive_page_addr = active_page_addr;
    active_page_addr = tmp;
    
    flash_lock();
    return true;
}
bool veeprom_write_8(uint32_t veeprom_addr, uint8_t value) {
    return veeprom_write(veeprom_addr, &value, 1);
}
bool veeprom_write_16(uint32_t veeprom_addr, uint16_t value) {
    return veeprom_write(veeprom_addr, (uint8_t*)&value, 2);
}
bool veeprom_write_32(uint32_t veeprom_addr, uint32_t value) {
    return veeprom_write(veeprom_addr, (uint8_t*)&value, 4);
}





//  ***************************************************************************
/// @brief  Lock/unlock FLASH
/// @return true - init success, false - fail
//  ***************************************************************************
static bool flash_lock() {
    FLASH->CR |= FLASH_CR_LOCK;
    return (FLASH->CR & FLASH_CR_LOCK) == FLASH_CR_LOCK;
}
static bool flash_unlock() {
    if (FLASH->CR & FLASH_CR_LOCK) {
        FLASH->KEYR = 0x45670123;
        FLASH->KEYR = 0xCDEF89AB;
    }
    return (FLASH->CR & FLASH_CR_LOCK) != FLASH_CR_LOCK;
}

//  ***************************************************************************
/// @brief  Wait FLASH operation complete
/// @return true - operation comleted, false - operation comleted with error
//  ***************************************************************************
static bool flash_wait_and_check() {
    while (FLASH->SR & FLASH_SR_BSY);
    if (FLASH->SR & (FLASH_SR_PGERR | FLASH_SR_WRPRTERR)) {
        FLASH->SR |= FLASH_SR_PGERR | FLASH_SR_WRPRTERR | FLASH_SR_EOP;
        return false;
    }
    FLASH->SR |= FLASH_SR_PGERR | FLASH_SR_WRPRTERR | FLASH_SR_EOP;
    return true;
}

//  ***************************************************************************
/// @brief  Erase FLASH page
/// @param  [in] flash_addr: page address for erase
/// @return true - success, false - fail
//  ***************************************************************************
static bool flash_page_erase(uint32_t flash_addr) {
    flash_unlock();
    
    FLASH->CR |= FLASH_CR_PER;
    FLASH->AR = flash_addr;
    FLASH->CR |= FLASH_CR_STRT;
    bool result = flash_wait_and_check();
    FLASH->CR &= ~FLASH_CR_PER;
    
    flash_lock();
    return result;
}

//  ***************************************************************************
/// @brief  Get/set FLASH page state
/// @param  [in] flash_addr: page address
/// @param  [in] state: new page state (for flash_page_set_state)
/// @return true - success, false - fail
//  ***************************************************************************
static uint64_t flash_page_get_state(uint32_t flash_addr) {
    uint64_t state = 0;
    for (uint8_t i = 0; i < 4; ++i) {
        state = (state << 16) | flash_read_16(flash_addr + PAGE_STATE_OFFSET + i * 2);
    }
    return state;
}
static bool flash_page_set_state(uint32_t flash_addr, uint64_t state) {
    uint64_t mask = 0xFFFF000000000000;
    for (uint8_t i = 0; i < 4; ++i) {
        if (state & mask) {
            if (flash_read_16(flash_addr + PAGE_STATE_OFFSET + i * 2) != 0xFFFF) {
                return false;
            }
            continue;
        }
        if (!flash_write_16(flash_addr + PAGE_STATE_OFFSET + i * 2, 0x0000)) {
            return false;
        }
        mask >>= 16;
    }
    return true;
}

//  ***************************************************************************
/// @brief  Calc/read/write checksum
/// @param  [in] flash_addr: page address
/// @param  [in] checksum: new page checksum (for flash_page_write_checksum)
/// @return true - success, false - fail
//  ***************************************************************************
static uint16_t flash_page_calc_checksum(uint32_t flash_addr) {
    uint32_t bytes_count = VEEPROM_PAGE_SIZE;
    uint16_t checksum = 0;
    while (bytes_count) {
        checksum += flash_read_8(flash_addr);
        ++flash_addr;
        --bytes_count;
    }
    return checksum;
}
static uint16_t flash_page_read_checksum(uint32_t flash_addr) {
    return flash_read_16(flash_addr + PAGE_CHECKSUM_OFFSET);
}
static bool flash_page_write_checksum(uint32_t flash_addr, uint16_t checksum) {
    return flash_write_16(flash_addr + PAGE_CHECKSUM_OFFSET, checksum);
}

//  ***************************************************************************
/// @brief  Read data from FLASH in BE format
/// @param  [in] flash_addr: page address
/// @param  [in] state: new page state (for flash_page_set_state)
/// @return cell value
//  ***************************************************************************
static uint8_t flash_read_8(uint32_t flash_addr) {
    return *((uint8_t*)flash_addr);
}
static uint16_t flash_read_16(uint32_t flash_addr) {
    return __REV16(*((uint16_t*)flash_addr));
}
static uint32_t flash_read_32(uint32_t flash_addr) {
    return __REV(*((uint32_t*)flash_addr));
}

//  ***************************************************************************
/// @brief  Write word to FLASH in LE format
/// @param  [in] flash_addr: page address
/// @param  [in] value: new cell value
/// @return true - success, false - fail
//  ***************************************************************************
static bool flash_write_16(uint32_t flash_addr, uint16_t value) {
    FLASH->CR |= FLASH_CR_PG;
    *((uint16_t*)flash_addr) = __REV16(value);
    bool result = flash_wait_and_check();
    FLASH->CR &= ~FLASH_CR_PG;
    
    if (flash_read_16(flash_addr) != value) {
        return false;
    }
    return result;
}
