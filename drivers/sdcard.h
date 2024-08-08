#ifndef _SDCARD_H
#define _SDCARD_H

void hexdump(uint8_t *buf, int len);

int sd_is_sdhc();
uint8_t sd_writesector(spi_inst_t *spi, uint32_t lba, uint8_t *data);
uint8_t sd_readsector(spi_inst_t *spi, uint32_t lba, uint8_t *sector);
int sd_init_card(spi_inst_t *spi);
void sd_hw_kill(spi_inst_t *spi);
spi_inst_t *sd_hw_init();

uint8_t sd_cmd9(spi_inst_t *spi, uint8_t buf[]); // CSD
uint8_t sd_cmd10(spi_inst_t *spi, uint8_t buf[]); // CID

#endif
