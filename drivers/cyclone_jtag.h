#ifndef CYCLONE_JTAG_H
#define CYCLONE_JTAG_H
#include <inttypes.h>

void jtag_enable();
void jtag_disable();
void jtag_program_start();
void jtag_program_end();
void jtag_program_postamble();
void jtag_program_chunk(uint8_t *data, uint16_t length);
int jtag_scan();

#endif /* CYCLONE_JTAG_H */
