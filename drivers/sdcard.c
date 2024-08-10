#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <pico/time.h>
#include "hardware/spi.h"
#include "hardware/clocks.h"
#include "hardware/structs/clocks.h"
#include "hardware/resets.h"

#include "pico/bootrom.h"
#include "pico/stdlib.h"

#include "sdcard.h"

#include "pins.h"
// #define DEBUG
#include "debug.h"

static int is_sdhc = 0;
static int cs_pin = PICO_SD_CS_PIN;

int sd_set_cs_pin(int pin) {
  cs_pin = pin;
}

int sd_is_sdhc() {
  return is_sdhc;
}

static void sd_set_highspeed(spi_inst_t *spi, int on);
static int sd_init_card_nosel(spi_inst_t *spi);

static uint8_t sd_spin(spi_inst_t *spi) {
  uint8_t out = 0xff;
  spi_read_blocking(spi, 0xff, &out, 1);
  return out;
}

static void sd_presignal(spi_inst_t *spi) {
  uint8_t sd_presignal_data[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
  gpio_put(cs_pin, 1);
//  gpio_set_function(PICO_DEFAULT_SPI_TX_PIN, GPIO_FUNC_NULL);
//  gpio_set_dir(PICO_DEFAULT_SPI_TX_PIN, GPIO_OUT);
//  gpio_put(PICO_DEFAULT_SPI_TX_PIN, true);
  spi_write_blocking(spi, sd_presignal_data, sizeof sd_presignal_data);
//  gpio_set_function(PICO_DEFAULT_SPI_TX_PIN, GPIO_FUNC_SPI);
}

static void spi_read8_blocking_align(spi_inst_t *spi, uint8_t *dst, size_t len) {
  *dst = 0xff;
  uint16_t timeout = 512;
  while ((*dst) == 0xff && --timeout) {
    spi_read_blocking(spi, 0xff, dst, 1);
  }
  if ((*dst) == 0xff) {
    debug(("SPI timeout!!!\n"));
    return;
  }
  if (len > 1) {
    spi_read_blocking(spi, 0xff, dst + 1, len - 1);
  }
#ifdef DEBUG
  printf("raw read\n");
  hexdump(dst, len);
#endif
}

static uint8_t get_next_byte(spi_inst_t *spi) {
  uint8_t buf[1];
  spi_read_blocking(spi, 0xff, buf, sizeof buf);
  return buf[0];
}

static uint8_t wait_ready(spi_inst_t *spi) {
  uint8_t buf = 0x00;
  int timeout = 2000;
  do {
    spi_read_blocking(spi, 0xff, &buf, 1);
    printf("buf is 0x%02x\n", buf);
    timeout --;
  } while (buf != 0xff && timeout);
  return buf;
}

static void get_bytes(spi_inst_t *spi, uint8_t *block, int len) {
  spi_read_blocking(spi, 0xff, block, len);
}

static void sd_select(spi_inst_t *spi, uint8_t ncs) {
  uint8_t spin;
  spin = sd_spin(spi); debug(("spin1 %02X\n", spin));
  gpio_put(cs_pin, ncs);
  spin = sd_spin(spi); debug(("spin2 %02X\n", spin));
}

#define NOCS    1
#define CS      2


static uint8_t sd_cmd(spi_inst_t *spi, uint8_t cmd[], int cmdlen, uint8_t buf[], int buflen, uint16_t good, int retries, int nocs) {
  uint8_t spin = 0xff, ch;
  uint8_t good1 = good & 0xff;
  uint8_t good2 = good >> 8;
  
  good2 = good2 ? good2 : good1;
  while (retries --) {
    sd_select(spi, 0);
    
    if ((ch=wait_ready(spi)) != 0xff) {
      gpio_put(cs_pin, 1);
      debug(("Retry %02X\n", ch));
      continue;
    }
    
    spi_write_blocking(spi, cmd, cmdlen);
    spi_read8_blocking_align(spi, buf, buflen);

    if (buf[0] == 0xff || buf[0] == good1 || buf[0] == good2) break;
    if ((nocs & CS) == CS) sd_select(spi, 1);
  }

  if (buf == NULL) {
    return 0x00;
  }
  if ((nocs & NOCS) != NOCS || (nocs & CS) == CS) {
    sd_select(spi, 1);
  }

#ifdef DEBUG
  hexdump(cmd, cmdlen);
  printf(" - ");
  hexdump(buf, buflen);
#endif
  return buf[0];
}

#define DEFAULT_RETRIES   10


static uint8_t sd_cxd(spi_inst_t *spi, uint8_t cmd[], uint8_t buf[]) {
  uint8_t buf1[1];
	sd_select(spi, 0);
  if (sd_cmd(spi, cmd, 6, buf1, sizeof buf1, 0x00, DEFAULT_RETRIES, 1) != 0x00) {
    sd_select(spi, 1);
    return 1;
  }

  int timeout = 20;
  while (timeout--) {
    uint8_t status = get_next_byte(spi);
    printf("status %02X\n", status);
    if (status == 0xfe) {
      break;
    }
  }

  if (timeout <= 0) {
    sd_select(spi, 1);
    printf("Timeout in cxd\n");
    return 1;
  }

  get_bytes(spi, buf, 16);
#ifdef DEBUG
  hexdump(buf, 16);
#endif
  timeout = 20;
  while (--timeout) {
    uint8_t status = get_next_byte(spi);
    printf("status after %02X\n", status);
    if (status == 0xff) {
      break;
    }
  }
  if (timeout == 0) {
    printf("Timeout after cxd\n");
  }
  sd_select(spi, 1);
  return 0;
}

uint8_t sd_cmd9(spi_inst_t *spi, uint8_t buf[]) { // csd
  uint8_t cmd[] = {0x49, 0x00, 0x00, 0x00, 0x00, 0xaf};
  return sd_cxd(spi, cmd, buf);
}

uint8_t sd_cmd10(spi_inst_t *spi, uint8_t buf[]) {
  uint8_t cmd[] = {0x4a, 0x00, 0x00, 0x00, 0x00, 0x1b};
  return sd_cxd(spi, cmd, buf);
}

static uint8_t sd_cmd1(spi_inst_t *spi) {
  uint8_t cmd[] = {0x50, 0x00, 0x00, 0x02, 0x00, 0xff};
  uint8_t buf[1];
  debug(("sd_cmd1\n"));
  return sd_cmd(spi, cmd, sizeof cmd, buf, sizeof buf, 0x00, DEFAULT_RETRIES, 0);
}

static uint8_t sd_reset(spi_inst_t *spi) {
  uint8_t cmd[] = {0x40, 0x00, 0x00, 0x00, 0x00, 0x95};
  uint8_t buf[1];
  return sd_cmd(spi, cmd, sizeof cmd, buf, sizeof buf, 0x01, DEFAULT_RETRIES, 0);
}

static uint8_t sd_init(spi_inst_t *spi) {
  uint8_t cmd[] = {0xff, 0x41, 0x00, 0x00, 0x00, 0x00, 0xff};
  uint8_t buf[1];
  return sd_cmd(spi, cmd, sizeof cmd, buf, sizeof buf, 0x01, DEFAULT_RETRIES, 0);
}

static uint8_t sd_cmd8(spi_inst_t *spi) {
  uint8_t cmd[] = {0xff, 0x48, 0x00, 0x00, 0x01, 0xAA, 0x87};
  debug(("sd_cmd8\n"));
  uint8_t buf[5];
  return sd_cmd(spi, cmd, sizeof cmd, buf, sizeof buf, 0x01, DEFAULT_RETRIES, 0);
}

static uint8_t sd_cmd55(spi_inst_t *spi) {
  uint8_t cmd[] = {0xff, 0x77, 0x00, 0x00, 0x00, 0x00, 0xff};
  uint8_t buf[1];
  return sd_cmd(spi, cmd, sizeof cmd, buf, sizeof buf, 0x00, DEFAULT_RETRIES, 0);
}

static uint8_t spi_spin = 0xff;

static uint8_t sd_cmd41(spi_inst_t *spi) {
  uint8_t cmd[] = {0xff, 0x69, 0x40, 0x00, 0x00, 0x00, 0x87};
  uint8_t buf[1];
  debug(("sd_cmd41\n"));
  return sd_cmd(spi, cmd, sizeof cmd, buf, sizeof buf, 0x00, 100, 0);
}


static uint8_t sd_cmd58(spi_inst_t *spi) {
  uint8_t cmd[] = {0xff, 0x7a, 0x00, 0x00, 0x00, 0x00, 0x75};
  uint8_t buf[5];
  debug(("sd_cmd58\n"));
  if (sd_cmd(spi, cmd, sizeof cmd, buf, sizeof buf, 0x00, DEFAULT_RETRIES, 0) == 0x00) {
    return buf[1];
  }
  return 0xff;
}

static uint8_t sd_cmd59(spi_inst_t *spi) {
  uint8_t cmd[] = {0x7b, 0x00, 0x00, 0x00, 0x00, 0xff};
  uint8_t buf[8];
  return sd_cmd(spi, cmd, sizeof cmd, buf, sizeof buf, 0x01, DEFAULT_RETRIES, 0);
}

#if 0
static uint8_t sd_cmd9(pio_spi_inst_t *spi) {
  uint8_t cmd[] = {0x49, 0x00, 0x00, 0x00, 0x00, 0xff};
  uint8_t buf[8];
  return sd_cmd(spi, cmd, sizeof cmd, buf, sizeof buf, 0x01, DEFAULT_RETRIES, 0);
}
#endif

uint8_t sd_writesector(spi_inst_t *spi, uint32_t lba, uint8_t *data) {
  uint8_t cmd[] = {0xff, 0x58, 0x00, 0x00, 0x00, 0x00, 0xff};
  uint8_t writegap[] = {0xff, 0xfe};
  uint8_t crc[] = {0xff, 0xff, 0xff};
  uint8_t buf[1];

  hexdump(data, 512);

  if (!is_sdhc) lba <<= 9;

  cmd[2] = (lba >> 24);
  cmd[3] = (lba >> 16) & 0xff;
  cmd[4] = (lba >> 8) & 0xff;
  cmd[5] = lba & 0xff;

	gpio_put(cs_pin, 0);
  sd_select(spi, 0);
  
  if (sd_cmd(spi, cmd, sizeof cmd, buf, sizeof buf, 0x00, DEFAULT_RETRIES, 1)!=0x00) {
    sd_select(spi, 1);
	  gpio_put(cs_pin, 1);
    debug(("Write failed 1 %02X\n", buf[0]));
    return 1;
  }
  
  spi_write_blocking(spi, writegap, sizeof writegap);
  spi_write_blocking(spi, data, 512);
  spi_write_blocking(spi, crc, sizeof crc);

  int timeout = 100000;
  while (timeout--) {
    uint8_t status = get_next_byte(spi);
    debug(("status %02X\n", status));
    if (status != 0x00) {
      break;
    }
  }
  
  sd_select(spi, 1);
	gpio_put(cs_pin, 1);
  return timeout > 0 ? 0 : 1;
}  

// #define SD_NO_CRC

// #define debug(a) printf a
static uint8_t sd_readsector_ll(spi_inst_t *spi, uint32_t lba, uint8_t *sector) {
  uint8_t cmd[] = {0x51, 0x00, 0x05, 0x00, 0x00, 0xff};
  uint8_t buf[1];
  uint8_t spin;

  if (!is_sdhc) lba <<= 9;
  
  cmd[1] = (lba >> 24);
  cmd[2] = (lba >> 16) & 0xff;
  cmd[3] = (lba >> 8) & 0xff;
  cmd[4] = lba & 0xff;

  // also 0x04 as expected code
  sd_select(spi, 0);
  uint8_t result = sd_cmd(spi, cmd, sizeof cmd, buf, sizeof buf, 0x00, DEFAULT_RETRIES, 1);
  if (result != 0x00 && result != 04) {
    sd_select(spi, 1);
    debug(("read failed 1 %02X\n", result));
    return 1;
  }
  
  int timeout = 50000;
  while (get_next_byte(spi) != 0xfe && timeout--)
    ;
  debug(("timeout after read = %d\n", timeout));
  
  if (timeout <= 1) {
    sd_select(spi, 1);
    debug(("Read failed 2\n"));
    return 1;
  }
  
#ifdef SD_DIRECT_MODE_GPIO
  if (!sector) gpio_put(SD_DIRECT_MODE_GPIO, 0);
#endif
  get_bytes(spi, sector, 512);
  
  uint8_t crc[2];
  get_bytes(spi, crc, sizeof crc);
#ifdef SD_DIRECT_MODE_GPIO
  if (!sector) gpio_put(SD_DIRECT_MODE_GPIO, 1);
#endif
  sd_select(spi, 1);
  
  uint16_t crcw = (crc[0] << 8) | crc[1];
  
#ifndef SD_NO_CRC
  if (sector != NULL && crc16iv(sector, 512, 0) != crcw) {
    debug(("Read failed crc 3 [%02X %02X] != %04X\n", crc[0], crc[1], crc16iv(sector, 512, 0)));
    hexdump(sector, 512);
    return 1;
  }
#endif

#ifdef DEBUG
  debug(("crc: %02X%02X\n", crc[0], crc[1]));
//   hexdump(sector, 512);
#endif
  
  return 0;
}
// #undef debug

#ifdef TEST_RETRIES
int xretryused = 0;
int xretry2used = 0;
#endif

uint8_t sd_readsector(spi_inst_t *spi, uint32_t lba, uint8_t *sector) {
  int retries, resets = 3;
  
  debug(("SD: lba %08X ", lba));

	gpio_put(cs_pin, 0);
  do {
    retries = 10;
    while (sd_readsector_ll(spi, lba, sector) && retries --) {
#ifdef TEST_RETRIES
      xretry2used ++;
#endif
      tight_loop_contents();
    }
  
    if (retries >= 0) {
	    gpio_put(cs_pin, 1);
      debug(("\n"));
      return 0;
    }
#ifdef TEST_RETRIES
    xretryused ++;
#endif
    // 3 retries failed, try to reset card and try again
    sd_init_card_nosel(spi);
  } while (resets--);

  // failed
	gpio_put(cs_pin, 1);
  debug((" Failed!\n"));
  return 1;
}

// 694000000077

static int sd_init_card_ll(spi_inst_t *spi) {
  is_sdhc = 0;
  sd_presignal(spi);
  printf("After sd_presignal\n");
  if (wait_ready(spi) != 0xff) {
    return 0;
  }
  printf("After wait_ready\n");
  sd_select(spi, 0);
  if (sd_reset(spi) == 0x01) {
//     if (sd_init(spi) == 0x01) {
      if (sd_cmd8(spi) == 0x01) {
        int retries = 5;
        while (retries --) {
          if (sd_cmd55(spi) == 0x01) {
            if (sd_cmd41(spi) == 0x00) {
              debug(("cmd41 got zero\n"));
              break;
            }
          }
        }
        
        if (retries > 0) {
          if (sd_cmd58(spi) & 0x40) {
            printf("SDHC CARD!\n");
            is_sdhc = 1;
            // it's an sdhc card
            sd_select(spi, 1);
            return 1;
          } else if (sd_cmd1(spi) == 0x00) {
            printf("SD CARD!\n");
            // it's not - so set blocksize
            sd_select(spi, 1);
            return 1;
          }
        }
      }
//     }
  } else {
    printf("sd_reset failed\n");
  }
  return 0;
}


static int sd_init_card_nosel(spi_inst_t *spi) {
  int timeout = 10;
  sd_set_highspeed(spi, 0);
  while (!sd_init_card_ll(spi) && timeout --)
    ;
  
  if (timeout > 0) {
    sd_set_highspeed(spi, 1);
  }
  
  return timeout > 0 ? 0 : 1;
}

int sd_init_card(spi_inst_t *spi) {
  int r;

	gpio_put(cs_pin, 0);
  r = sd_init_card_nosel(spi);
	gpio_put(cs_pin, 1);
  return r;
}

static uint spi_offset = 0;
void sd_hw_kill(spi_inst_t *spi) {
  spi_deinit(spi);
  gpio_set_dir(PICO_DEFAULT_SPI_CSN_PIN, GPIO_IN);
}

static spi_inst_t* spi = spi0;

spi_inst_t *sd_hw_init() {
  static int firsttime = 1;
  printf("Initializing SPI PIO\n");
  gpio_init(PICO_DEFAULT_SPI_TX_PIN);
  gpio_set_function(PICO_DEFAULT_SPI_TX_PIN, GPIO_FUNC_SPI);
  gpio_init(PICO_DEFAULT_SPI_SCK_PIN);
  gpio_set_function(PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI);
  gpio_set_drive_strength(PICO_DEFAULT_SPI_SCK_PIN, GPIO_DRIVE_STRENGTH_12MA);
  gpio_init(PICO_DEFAULT_SPI_RX_PIN);
  gpio_set_function(PICO_DEFAULT_SPI_RX_PIN, GPIO_FUNC_SPI);
  spi_init(spi, 400 * 1000);
  spi_set_format(spi, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
  gpio_init(PICO_DEFAULT_SPI_CSN_PIN);
  gpio_set_dir(PICO_DEFAULT_SPI_CSN_PIN, GPIO_OUT);
  return spi;
}

static void sd_set_highspeed(spi_inst_t *spi, int on) {
 printf("sd_set_highspeed %d\n", on);
 spi_set_baudrate(spi, on ? 25000000 : 400000);
 }

