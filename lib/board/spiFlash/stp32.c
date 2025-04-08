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

#ifndef EMULATOR
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>
#endif

#include "keepkey/board/spiFlash/stp32.h"
#include "keepkey/board/timer.h"

#ifdef TWO_DISP
#include "keepkey/board/ssd1351/ssd1351.h"
#endif

#include <stdbool.h>
#include <stdio.h>

static void spifSelect(void) {
  CLEAR_PIN(SPIF_CS_PIN);
}
static void spifDeselect(void) {
  SET_PIN(SPIF_CS_PIN);
}

// static void spifReset(void) {
//   spifSelect();
//   spi_send(SPIF_SPI_PORT, (uint16_t)SPIF_ENABLE_RESET);
//   delay_us(10);   // apparently spi_send doesn't wait for buffer to finish as it should
//   spi_send(SPIF_SPI_PORT, (uint16_t)SPIF_RESET);
//   delay_us(40);   // wait for reset
//   spifDeselect();
//   return;
// }

// bool spif_read_command(uint8_t command, uint8_t *response, uint32_t length);
// bool spif_write_command(uint8_t command, uint8_t *data, uint32_t length);
// bool spif_sector_command(uint8_t command, uint32_t address);
// bool spif_read_data(uint32_t address, uint8_t *data, uint32_t data_length);

bool spif_waitReady(uint8_t status[2]) {
  // get status and read id regs
  uint8_t timeout=0xff;

  do {
    spif_read_command(SPIF_READ_STATUS_REG_1, status, 2);
    timeout--;
  } while(1 == (status[1] & 0x01) && timeout>0);

  if (timeout>0) {
    return true;
  } else {
    return false;
  }
}


void spif_init_device(void) {
  char *stat;
  uint8_t status[2] = {0, 0};
  uint8_t jedecId[4], manufId[6];
  char snpstr[32];
  bool spifReady;

  // set up chip select pin
  gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN,
    GPIO15);
  gpio_set_output_options(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ,
    GPIO15);

  // set up pins for alternate function 6: spi3
  gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO3 | GPIO4 | GPIO5);
  gpio_set_af(GPIOB, GPIO_AF6, GPIO3 | GPIO4 | GPIO5);
  // set clock and data out pins output options
  gpio_set_output_options(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ,
    GPIO3 | GPIO5);

  // enable SPI clock
  rcc_periph_clock_enable(RCC_SPI3);

  spi_init_master(
    SPIF_SPI_PORT, 
    SPI_CR1_BAUDRATE_FPCLK_DIV_32, 
    SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
    SPI_CR1_CPHA_CLK_TRANSITION_1, 
    SPI_CR1_DFF_8BIT, 
    SPI_CR1_MSBFIRST);

  spi_enable_ss_output(SPIF_SPI_PORT);

  spifDeselect();
  spi_enable(SPIF_SPI_PORT);

  // spifReset();
  delay_ms(1);

  // get status and read id regs
  spifReady = spif_waitReady(status);

  if (spifReady) {
    stat = "spi flash ready\n";
  } else {
    stat = "spi flash not found\n";
  }
  SSD1351_WriteString(-1, -1, stat, Font_7x10, SSD1351_BLUE, SSD1351_BLACK);

  spif_read_command(SPIF_READ_DEVICE_ID, manufId, sizeof(manufId));
  snprintf(snpstr, sizeof(snpstr), "manufacturer id:\n%02x %02x %02x", manufId[3], manufId[4], manufId[5]);
  SSD1351_WriteString(-1, -1, snpstr, Font_7x10, SSD1351_BLUE, SSD1351_BLACK);

  spif_read_command(SPIF_READ_JEDEC_ID, jedecId, sizeof(jedecId));
  snprintf(snpstr, sizeof(snpstr), "\njedec id:\n%02x %02x %02x", jedecId[1], jedecId[2], jedecId[3]);
  SSD1351_WriteString(-1, -1, snpstr, Font_7x10, SSD1351_BLUE, SSD1351_BLACK);
  return;
}

bool spif_command(uint8_t command) {
  // send one byte command to spi flash
  spifSelect();
  delay_us(10);
  spi_send(SPIF_SPI_PORT, (uint16_t)command);
  delay_us(10);
  spifDeselect();
  delay_us(100);
  return true;
}

bool spif_read_command(uint8_t command, uint8_t *response, uint32_t length) {
  // send command and read data back from spi flash

  uint32_t ctr;

  spifSelect();
  delay_us(10);

  spi_send(SPIF_SPI_PORT, (uint16_t)command);
  delay_us(10);

  for(ctr=0; ctr<length; ctr++) {
    spi_send(SPIF_SPI_PORT, (uint16_t)0);    // dummy write to clock data
    response[ctr] = (uint8_t)spi_read(SPIF_SPI_PORT);
  }

  spifDeselect();
  delay_us(100);
  return(true);
}


static bool _spif_write_page(uint32_t address, uint8_t *data, uint16_t data_length) {

  // write one page (or less) of data, do not cross page boundary
  uint32_t bufctr=0;
  uint8_t addr[3];

  // get status and read id regs
  uint8_t status[2];
  char *stat;
  bool spifReady;
  (void)stat, (void)spifReady, (void)status;

  // check for page boundary violation
  if (((address % SPIF_PAGE_SIZE) + data_length) > SPIF_PAGE_SIZE) {
    return false;
  }

  spif_command(SPIF_WRITE_ENABLE);

  // spifReady = spif_waitReady(status);
  // if (spifReady) {
  //   if (status[1] & 0x02) {
  //     stat = "\nwrite enabled\n";
  //   } else {
  //     stat = "\nwrite not enabled\n";
  //   }
  // } else {
  //   stat = "\nspi flash not found\n";
  // }
  // SSD1351_WriteString(-1, -1, stat, Font_7x10, SSD1351_BLUE, SSD1351_BLACK);
  // }

  // break address into bytes
  addr[0] = (uint8_t)(address >> 16); 
  addr[1] = (uint8_t)(address >> 8); 
  addr[2] = (uint8_t)(address);

  spifSelect();
  delay_us(10);
  spi_send(SPIF_SPI_PORT, SPIF_PAGE_PROGRAM);
  // 3 address bytes
  for (bufctr=0;bufctr<3;bufctr++) {
    spi_send(SPIF_SPI_PORT, addr[bufctr]);
  }
  // data bytes
  for (bufctr=0;bufctr<data_length;bufctr++) {
    spi_send(SPIF_SPI_PORT, data[bufctr]);
  }
  delay_us(10);
  spifDeselect();
  delay_us(100);
  return true;
}

bool spif_write_data(uint32_t address, uint8_t *data, uint32_t data_length) {
  // write data to spi flash
  // data must be chunked so as not to cross page boundaries when written.
  /*
  From the Page Program (02h) command datasheet description:

  The Page Program instruction allows from one byte to 256 bytes (a page) of data to 
  be programmed at previously erased (FFh) memory locations.
  ...
  If an entire 256 byte page is to be programmed, the last address byte (the 8 least significant address 
  bits) should be set to 0. If the last address byte is not zero, and the number of clocks exceeds the 
  remaining page length, the addressing will wrap to the beginning of the page. In some cases, less than 
  256 bytes (a partial page) can be programmed without having any effect on other bytes within the same 
  page. One condition to perform a partial page program is that the number of clocks cannot exceed the 
  remaining page length. If more than 256 bytes are sent to the device the addressing will wrap to the 
  beginning of the page and overwrite previously sent data.
  */

  uint8_t status[2];
  uint32_t startAddressOnPage;
  uint8_t remainingBytesOnPage;
  int32_t dataLenRemaining;
  uint32_t dataIndx;

  // find first page boundary it will write over if there is one
  startAddressOnPage = address;
  dataLenRemaining = data_length;
  dataIndx = 0;

  remainingBytesOnPage = SPIF_PAGE_SIZE - startAddressOnPage % SPIF_PAGE_SIZE;

  if (dataLenRemaining <= remainingBytesOnPage) {
    // data to write will not cross a page boundary
    _spif_write_page(startAddressOnPage, &data[dataIndx], dataLenRemaining);
    spif_waitReady(status);
  } else {
    // data will cross at least one page boundary
    _spif_write_page(startAddressOnPage, &data[dataIndx], remainingBytesOnPage);
    spif_waitReady(status);
    startAddressOnPage += remainingBytesOnPage;
    dataLenRemaining -= remainingBytesOnPage;
    dataIndx += remainingBytesOnPage;
    while (dataLenRemaining >= SPIF_PAGE_SIZE) {
      // write next full page if a full page write is necessary
      _spif_write_page(startAddressOnPage, &data[dataIndx], SPIF_PAGE_SIZE);
      spif_waitReady(status);
      startAddressOnPage += SPIF_PAGE_SIZE;
      dataLenRemaining -= SPIF_PAGE_SIZE;
      dataIndx += SPIF_PAGE_SIZE;
      }
    if (dataLenRemaining > 0) {
      // last page write if needed
      _spif_write_page(startAddressOnPage, &data[dataIndx], dataLenRemaining);
      spif_waitReady(status);
    }
  }

  return true;

}

bool spif_read_data(uint32_t address, uint8_t *data, uint32_t data_length) {
  // write data to spi flash
  int32_t bufctr=0;
  uint8_t addr[3];

  // break address into bytes
  addr[0] = (uint8_t)(address >> 16); 
  addr[1] = (uint8_t)(address >> 8); 
  addr[2] = (uint8_t)(address);

  spifSelect();
  spi_send(SPIF_SPI_PORT, SPIF_READ_DATA);

  for (bufctr=0;bufctr<3;bufctr++) {
    spi_send(SPIF_SPI_PORT, addr[bufctr]);
  }
  for(bufctr=-2; bufctr<(int32_t)data_length; bufctr++) {
    spi_send(SPIF_SPI_PORT, (uint16_t)0);    // dummy write to clock data
    if (bufctr>=0) {
      data[bufctr] = (uint8_t)spi_read(SPIF_SPI_PORT);
    } else {
      spi_read(SPIF_SPI_PORT);    // read dummy bytes
    }
  }

  spifDeselect();
  delay_us(100);
  return true;
}

bool spif_sector_command(uint8_t command, uint32_t address) {
  // erase 4k sector or 32k block or 64k block
  uint32_t bufctr=0;
  uint8_t addr[3];

  if (!(command==SPIF_SECT_ERASE_4KB || command==SPIF_BLK_ERASE_32KB || command==SPIF_BLK_ERASE_64KB)) {
    return false;
  }

  // break address into bytes
  addr[0] = (uint8_t)(address >> 16); 
  addr[1] = (uint8_t)(address >> 8); 
  addr[2] = (uint8_t)(address);

  spif_command(SPIF_WRITE_ENABLE);
  spifSelect();
  delay_us(10);
  spi_send(SPIF_SPI_PORT, command);
  // 3 address bytes
  for (bufctr=0;bufctr<3;bufctr++) {
    spi_send(SPIF_SPI_PORT, addr[bufctr]);
  }
  
  delay_us(10);
  spifDeselect();
  delay_us(100);
  return true;
}

bool spif_chipErase(void) {
  spifSelect();
  delay_us(10);

  spif_command(SPIF_WRITE_ENABLE);
  spif_command(SPIF_CHIP_ERASE1);

  spifDeselect();
  delay_us(100);
  return true;
}