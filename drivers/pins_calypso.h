#ifndef PINS_CALYPSO_H
#define PINS_CALYPSO_H

#undef PICO_DEFAULT_SPI_SCK_PIN
#undef PICO_DEFAULT_SPI_TX_PIN
#undef PICO_DEFAULT_SPI_RX_PIN
#undef PICO_DEFAULT_SPI_CSN_PIN

#define GPIO_MIST_CSN 	6
#define GPIO_MIST_SS2 	7
#define GPIO_MIST_SS3 	16
#define GPIO_MIST_SS4 	13

#define MIST_CSN    6 // user io
#define MIST_SS2    7 // data io
#define MIST_SS3    16 // osd
#define MIST_SS4    13 // dmode?

#define GPIO_PS2_CLK	10
#define GPIO_PS2_DATA	11
#define GPIO_PS2_CLK2	8
#define GPIO_PS2_DATA2	9

#define GPIO_JTAG_TCK	17
#define GPIO_JTAG_TDO	18
#define GPIO_JTAG_TDI	19
#define GPIO_JTAG_TMS	20

#define GPIO_JRT        29
#define GPIO_JLT        23
#define GPIO_JDN        27
#define GPIO_JUP        28
#define GPIO_JF1        26
#define GPIO_JF2        22
#define GPIO_JSEL       21

#define GPIO_SPI_SCK     14
#define GPIO_SPI_MOSI    15
#define GPIO_SPI_MISO    12

#define PICO_SD_CLK_PIN 	2
#define PICO_SD_CMD_PIN 	3
#define PICO_SD_DAT0_PIN 	4
#define PICO_SD_CS_PIN 	5

#define PICO_DEFAULT_SPI_SCK_PIN 2
#define PICO_DEFAULT_SPI_TX_PIN  3
#define PICO_DEFAULT_SPI_RX_PIN  4
#define PICO_DEFAULT_SPI_CSN_PIN 5

#define GPIO_DEBUG_TX_PIN	1


#endif /* PINS_CALYPSO_H */
