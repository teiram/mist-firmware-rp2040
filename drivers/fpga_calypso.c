#include <stdio.h>
#include <stdint.h>
#include "pins.h"
#include "fpga.h"
#include "cyclone_jtag.h"
#include "debug.h"

#define RESET_TIMEOUT 100000
#define CHUNKSIZE 512

int fpga_ResetButtonState() {
  return 0;
}

int fpga_initialise() {
  printf("fpga_initialise\n");
  return 0;
}

int fpga_claim(uint8_t claim) {
  printf("fpga_claim %d\n", claim);
  return 0;
}

void fpga_holdreset() {
  printf("fpga_holdreset\n");
}

int fpga_reset() {
  printf("fpga_reset\n");
  return 0;
}

int fpga_configure(void *user_data, uint8_t (*next_block)(void *, uint8_t *), uint32_t assumelength) {
  printf("fpga_configure. File size %d\n", assumelength);
  uint8_t bits[CHUNKSIZE];
  if (!next_block(user_data, bits)) {
    /* No data to read */
    return 1;
  }
  /*
  if (jtag_scan() != 0) {
    return 1;
  }
  */
  uint32_t remaining = assumelength;
  jtag_program_start();
  do {
    uint16_t chunklen = remaining > CHUNKSIZE ? CHUNKSIZE : remaining;
    jtag_program_chunk(bits, chunklen);
    remaining -= chunklen;
  } while (remaining > 0 && next_block(user_data, bits));
  
  jtag_program_postamble();
  jtag_program_end();
  printf("JTAG Programming finished\n");
  return 0;
}

