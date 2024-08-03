#include <inttypes.h>
#include <stdio.h>
#include "hardware/pio.h"
#include "pins_calypso.h"
    

#define DB9_UP          0x08
#define DB9_DOWN        0x04
#define DB9_LEFT        0x02
#define DB9_RIGHT       0x01
#define DB9_BTN1        0x10
#define DB9_BTN2        0x20


void DB9SetLegacy(uint8_t on);
void DB9Update(uint8_t joy_num, uint8_t usbjoy);

void InitDB9() {
  uint8_t lut[] = {GPIO_JRT, GPIO_JLT, GPIO_JDN, GPIO_JUP, GPIO_JF1, GPIO_JF2, GPIO_JSEL};
  for (int i = 0; i < sizeof(lut); i++) {
    gpio_init(lut[i]);
    gpio_set_dir(lut[i], GPIO_IN);
    gpio_pull_up(lut[i]);
  }
}

#define JOY_ALL   (DB9_RIGHT|DB9_LEFT|DB9_UP|DB9_DOWN|DB9_BTN1|DB9_BTN2|DB9_BTN3)

char GetDB9(char index, unsigned char *joy_map) {
  static char data = 0;
  static int counter = 4;
  if (!index) {
    if (--counter == 0) {
      data = 0;
      data |= gpio_get(GPIO_JRT)  ? 0 : DB9_RIGHT;
      data |= gpio_get(GPIO_JLT)  ? 0 : DB9_LEFT;
      data |= gpio_get(GPIO_JDN)  ? 0 : DB9_DOWN;
      data |= gpio_get(GPIO_JUP)  ? 0 : DB9_UP;
      data |= gpio_get(GPIO_JF1)  ? 0 : DB9_BTN1;
      data |= gpio_get(GPIO_JF2)  ? 0 : DB9_BTN2;
      counter = 4;
    }
  }

  *joy_map = data;
  return 1;
}

static uint8_t legacy_mode = 0;
static void SetGpio(uint8_t usbjoy, uint8_t mask, uint8_t gpio) {}

void DB9SetLegacy(uint8_t on) {}

void DB9Update(uint8_t joy_num, uint8_t usbjoy) {}
