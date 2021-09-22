//  ***************************************************************************
/// @file    veeprom.h
/// @author  NeoProg
/// @brief   VEEPROM driver
//  ***************************************************************************
#ifndef _VEEPROM_H_
#define _VEEPROM_H_
#include <stdint.h>
#include <stdbool.h>


//  ***************************************************************************
/// @brief  VEEPROM driver initializetion
/// @return true - init success, false - fail
//  ***************************************************************************
extern bool veeprom_init();

//  ***************************************************************************
/// @brief  Mass erase VEEPROM
/// @return true - init success, false - fail
//  ***************************************************************************
extern bool veeprom_mass_erase();

//  ***************************************************************************
/// @brief  Read data from VEEPROM
/// @param  [in] veeprom_addr: virtual address [0x0000...size-1]
/// @param  [out] buffer: pointer to buffer for data
/// @param  [in] bytes_count: bytes count for read
/// @return true - init success, false - fail
//  ***************************************************************************
extern bool veeprom_read(uint32_t veeprom_addr, uint8_t* buffer, uint32_t bytes_count);
extern uint8_t veeprom_read_8(uint32_t veeprom_addr);
extern uint16_t veeprom_read_16(uint32_t veeprom_addr);
extern uint32_t veeprom_read_32(uint32_t veeprom_addr);

//  ***************************************************************************
/// @brief  Write data to VEEPROM
/// @param  [in] veeprom_addr: virtual address [0x0000...size-1]
/// @param  [out] data: pointer to data for write
/// @param  [in] bytes_count: bytes count for write
/// @return true - init success, false - fail
//  ***************************************************************************
extern bool veeprom_write(uint32_t veeprom_addr, uint8_t* data, uint32_t bytes_count);
extern bool veeprom_write_8(uint32_t veeprom_addr, uint8_t  value);
extern bool veeprom_write_16(uint32_t veeprom_addr, uint16_t value);
extern bool veeprom_write_32(uint32_t veeprom_addr, uint32_t value);


#endif // _VEEPROM_H_
