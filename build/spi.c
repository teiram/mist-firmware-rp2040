#include <stdio.h>
#include <string.h>

#include "spi.h"
#include "hardware.h"

// #undef spi_init
#include "hardware/spi.h"
#include "pico/bootrom.h"
#include "pico/stdlib.h"
#include "hardware/pio.h"

// TODO MJ interface to FPGA via SPI and all the nCS for different bits.

#ifndef CALYPSO
#define MIST_CSN    17 // user io
#define MIST_SS2    20 // data io
#define MIST_SS3    21 // osd
// #define MIST_SS4    24 // dmode?
// #ifdef ZXUNO
// #define MIST_SS4    19 // dmode?
//TODO MJ this will probably have to change GPIO19 / P48 seems to be busy
// #else
#define MIST_SS4    22 // dmode?
// #endif
#else
#include "drivers/pins_calypso.h"
#endif

#define SPI_SLOW_BAUD   500000
#define SPI_SDC_BAUD   24000000
#define SPI_MMC_BAUD   16000000
// #define SPI_SLOW_BAUD   500000
// #define SPI_SDC_BAUD   500000
// #define SPI_MMC_BAUD   500000

static unsigned char spi_speed;

// void test_UserIOSPI(uint8_t datain) {
//   uint8_t data[6];
// // int spi_write_read_blocking (spi_inst_t *spi, const uint8_t *src, uint8_t *dst, size_t len)
// //   uint8_t cmd[] = {0x1a, 0x00, 0x00, 0x00, 0xff, 0xff};
//   uint8_t cmd[] = {0x02, 0xff};
//
//   gpio_put(MIST_CSN, 0);
//
//   cmd[1] = datain;
//
//   memset(data, 0xff, sizeof data);
//   spi_write_read_blocking(spi0, cmd, data, sizeof cmd);
//   printf("Returns: ");
//   for (int i=0; i<sizeof data; i++) {
//     printf("%02X ", data[i]);
//   }
//   printf("\n");
//   gpio_put(MIST_CSN, 1);
// }



// void test_UserIOKill() {
//   spi_deinit(spi0);
// }

//#define spi spi1
#define spi spi0
void mist_spi_init() {
  uint8_t csn_lut[] = {MIST_CSN, MIST_SS2, MIST_SS3, MIST_SS4};

  for (int i=0; i<sizeof csn_lut; i++) {
    gpio_init(csn_lut[i]);
    gpio_put(csn_lut[i], 1);
    gpio_set_dir(csn_lut[i], GPIO_OUT);
  }

// #ifdef ZXUNO
//   uint8_t spi_pins[] = {16, 18, 22};
// #else
#ifndef CALYPSO
  uint8_t spi_pins[] = {16, 18, 19};
#else
  uint8_t spi_pins[] = {PICO_DEFAULT_SPI_TX_PIN, PICO_DEFAULT_SPI_RX_PIN, PICO_DEFAULT_SPI_SCK_PIN};
  //uint8_t spi_pins[] = {GPIO_SPI_SCK, GPIO_SPI_MOSI, GPIO_SPI_MISO};
#endif /* ifndef CALYPSO */
  /*
   * TODO: Centralized SPI shared with SD 
  for (int i=0; i<sizeof spi_pins; i++) {
    gpio_init(spi_pins[i]);
    gpio_set_function(spi_pins[i], GPIO_FUNC_SPI);
  }
  spi_init(spi, SPI_SLOW_BAUD); // 500khz
  spi_set_format(spi, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
  */
  spi_speed = SPI_SLOW_CLK_VALUE;
}

RAMFUNC void spi_wait4xfer_end() {
  while (spi_is_busy(spi)) tight_loop_contents;
}


void EnableFpga() {
  gpio_put(MIST_SS2, 0);
}

void DisableFpga() {
  spi_wait4xfer_end();
  gpio_put(MIST_SS2, 1);
}

void EnableOsd() {
  gpio_put(MIST_SS3, 0);
}

void DisableOsd() {
  spi_wait4xfer_end();
  gpio_put(MIST_SS3, 1);
}

void EnableIO() {
  gpio_put(MIST_CSN, 0);
}

void DisableIO() {
  spi_wait4xfer_end();
  gpio_put(MIST_CSN, 1);
}

void EnableDMode() {
  // spi_set_format(spi0, 8, SPI_CPOL_1, SPI_CPHA_0, SPI_MSB_FIRST);
  gpio_put(MIST_SS4, 0);
}

void DisableDMode() {
  // spi_set_format(spi0, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
  spi_wait4xfer_end();
  gpio_put(MIST_SS4, 1);
}

// void spi_max_start() {
// }
//
// void spi_max_end() {
//     spi_wait4xfer_end();
// }

void spi_block(unsigned short num) {
  uint8_t out, in;
  out = 0xff;
  while (num--) {
    spi_write_read_blocking (spi, &out, &in, 1);
  }
}

RAMFUNC void spi_read(char *addr, uint16_t len) {
  spi_read_blocking (spi, 0x00, addr, len);
}

RAMFUNC void spi_block_read(char *addr) {
  spi_read(addr, 512);
}

void spi_write(const char *addr, uint16_t len) {
  spi_write_blocking (spi, addr, len);
}

unsigned char SPI(unsigned char outByte) {
  uint8_t dst;
  spi_write_read_blocking (spi, &outByte, &dst, 1);
  return dst;
}

void spi_block_write(const char *addr) {
  spi_write(addr, 512);
}

void spi_slow() {
  spi_set_baudrate (spi, SPI_SLOW_BAUD);
  spi_speed = SPI_SLOW_CLK_VALUE;
}

void spi_fast() {
  spi_set_baudrate (spi, SPI_SDC_BAUD);
  spi_speed = SPI_SDC_CLK_VALUE;
}

void spi_fast_mmc() {
  spi_set_baudrate (spi, SPI_MMC_BAUD);
  spi_speed = SPI_MMC_CLK_VALUE;
}

unsigned char spi_get_speed() {
  return spi_speed;
}

void spi_set_speed(unsigned char speed) {
  switch (speed) {
    case SPI_SLOW_CLK_VALUE:
      spi_slow();
      break;

    case SPI_SDC_CLK_VALUE:
      spi_fast();
      break;

    default:
      spi_fast_mmc();
  }
}

/* generic helper */
unsigned char spi_in() {
  return SPI(0);
}

void spi8(unsigned char parm) {
  SPI(parm);
}

void spi16(unsigned short parm) {
  SPI(parm >> 8);
  SPI(parm >> 0);
}

void spi16le(unsigned short parm) {
  SPI(parm >> 0);
  SPI(parm >> 8);
}

void spi24(unsigned long parm) {
  SPI(parm >> 16);
  SPI(parm >> 8);
  SPI(parm >> 0);
}

void spi32(unsigned long parm) {
  SPI(parm >> 24);
  SPI(parm >> 16);
  SPI(parm >> 8);
  SPI(parm >> 0);
}

// little endian: lsb first
void spi32le(unsigned long parm) {
  SPI(parm >> 0);
  SPI(parm >> 8);
  SPI(parm >> 16);
  SPI(parm >> 24);
}

void spi_n(unsigned char value, unsigned short cnt) {
  while(cnt--) 
    SPI(value);
}

/* OSD related SPI functions */
void spi_osd_cmd_cont(unsigned char cmd) {
  EnableOsd();
  SPI(cmd);
}

void spi_osd_cmd(unsigned char cmd) {
  spi_osd_cmd_cont(cmd);
  DisableOsd();
}

void spi_osd_cmd8_cont(unsigned char cmd, unsigned char parm) {
  EnableOsd();
  SPI(cmd);
  SPI(parm);
}

void spi_osd_cmd8(unsigned char cmd, unsigned char parm) {
  spi_osd_cmd8_cont(cmd, parm);
  DisableOsd();
}

void spi_osd_cmd32_cont(unsigned char cmd, unsigned long parm) {
  EnableOsd();
  SPI(cmd);
  spi32(parm);
}

void spi_osd_cmd32(unsigned char cmd, unsigned long parm) {
  spi_osd_cmd32_cont(cmd, parm);
  DisableOsd();
}

void spi_osd_cmd32le_cont(unsigned char cmd, unsigned long parm) {
  EnableOsd();
  SPI(cmd);
  spi32le(parm);
}

void spi_osd_cmd32le(unsigned char cmd, unsigned long parm) {
  spi_osd_cmd32le_cont(cmd, parm);
  DisableOsd();
}

/* User_io related SPI functions */
void spi_uio_cmd_cont(unsigned char cmd) {
  EnableIO();
  SPI(cmd);
}

void spi_uio_cmd(unsigned char cmd) {
  spi_uio_cmd_cont(cmd);
  DisableIO();
}

void spi_uio_cmd8_cont(unsigned char cmd, unsigned char parm) {
  EnableIO();
  SPI(cmd);
  SPI(parm);
}

void spi_uio_cmd8(unsigned char cmd, unsigned char parm) {
  spi_uio_cmd8_cont(cmd, parm);
  DisableIO();
}

void spi_uio_cmd32(unsigned char cmd, unsigned long parm) {
  EnableIO();
  SPI(cmd);
  SPI(parm);
  SPI(parm>>8);
  SPI(parm>>16);
  SPI(parm>>24);
  DisableIO();
}

void spi_uio_cmd64(unsigned char cmd, unsigned long long parm) {
  EnableIO();
  SPI(cmd);
  SPI(parm);
  SPI(parm>>8);
  SPI(parm>>16);
  SPI(parm>>24);
  SPI(parm>>32);
  SPI(parm>>40);
  SPI(parm>>48);
  SPI(parm>>56);
  DisableIO();
}
