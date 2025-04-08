/*
 * This file is part of the KeepKey project.
 *
 * Copyright (C) 2025 markrypto
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef __STP32_H__
#define __STP32_H__

#include <stdbool.h>
#include <stdint.h>

#include "keepkey/board/pin.h"

#define SPIF_PAGE_SIZE          0x100

// spi flash commands
#define SPIF_WRITE_ENABLE       0x06
#define SPIF_WRITE_DISABLE	    0x04
#define SPIF_VOL_STATUS_REG_WRITE_ENABLE	0x50
#define SPIF_READ_STATUS_REG_1	0x05
#define SPIF_READ_STATUS_REG_2	0x35
#define SPIF_READ_STATUS_REG_3	0x15
#define SPIF_WRITE_STATUS_REG_1	0x01
#define SPIF_WRITE_STATUS_REG_2	0x31
#define SPIF_WRITE_STATUS_REG_3	0x11
#define SPIF_SECT_ERASE_4KB		  0x20
#define SPIF_BLK_ERASE_32KB		  0x52
#define SPIF_BLK_ERASE_64KB		  0xD8
#define SPIF_CHIP_ERASE1			  0xC7
#define SPIF_CHIP_ERASE2			  0x60
#define SPIF_READ_UID				    0x4B
#define SPIF_READ_JEDEC_ID			0x9F
#define SPIF_READ_DEVICE_ID     0x90
#define SPIF_READ_DATA          0x03
#define SPIF_FAST_READ          0x0B
#define SPIF_PAGE_PROGRAM       0x02
#define SPIF_ENABLE_RESET       0x66
#define SPIF_RESET              0x99


#define SPIF_SPI_PORT SPI3

// spi flash bus pins for the sparkfun thing plus stm32f4 board
static const Pin SPIF_MOSI_PIN = {GPIOB, GPIO5};
static const Pin SPIF_MISO_PIN = {GPIOB, GPIO4};
static const Pin SPIF_SCK_PIN =  {GPIOB, GPIO3};
static const Pin SPIF_CS_PIN =   {GPIOA, GPIO15};


bool spif_waitReady(uint8_t status[2]);
bool spif_command(uint8_t command);
bool spif_read_command(uint8_t command, uint8_t *response, uint32_t length);
// bool spif_write_command(uint8_t command, uint8_t *data, uint32_t length);
bool spif_sector_command(uint8_t command, uint32_t address);
bool spif_write_data(uint32_t address, uint8_t *data, uint32_t data_length);
bool spif_read_data(uint32_t address, uint8_t *data, uint32_t data_length);
void spif_init_device(void);
bool spif_chipErase(void);

#endif