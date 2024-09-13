#include "cyclone_jtag.h"
#include "hardware/gpio.h"
#include "pins.h"

#define TDO GPIO_JTAG_TDO
#define TCK GPIO_JTAG_TCK
#define TDI GPIO_JTAG_TDI
#define TMS GPIO_JTAG_TMS

#define MAX_IR_CHAINLENGTH 100


typedef struct {    
  uint8_t onebit:1;     
  uint16_t manuf:11; 
  uint16_t size:9;  
  uint8_t family:7; 
  uint8_t rev:4; 
} jtag_code_t;

typedef union {
    uint32_t code;
    jtag_code_t b;
} idcode_t;

void jtag_enable() {
    gpio_init(TDO);
    gpio_init(TDI);
    gpio_init(TCK);
    gpio_init(TMS);

    gpio_set_dir(TDO, GPIO_IN);
    gpio_set_dir(TDI, GPIO_OUT);
    gpio_set_dir(TCK, GPIO_OUT);
    gpio_set_dir(TMS, GPIO_OUT);
}

void jtag_disable() {
    gpio_put(TDI, true);
    gpio_put(TCK, false);
    gpio_put(TMS, true);

    gpio_set_dir(TDI, GPIO_IN);
    gpio_set_dir(TCK, GPIO_IN);
    gpio_set_dir(TMS, GPIO_IN);
    gpio_set_input_enabled(TDI, false);
    gpio_set_input_enabled(TCK, false);
    gpio_set_input_enabled(TMS, false);
    gpio_set_input_enabled(TDO, false);

    
    gpio_set_pulls(TDI, false, true);
    gpio_set_pulls(TCK, false, true);
    gpio_set_pulls(TMS, false, true);
    gpio_set_pulls(TDO, false, true);
    
    /*
    gpio_deinit(TDO);
    gpio_deinit(TDI);
    gpio_deinit(TCK);
    gpio_deinit(TMS);
    */
}

static void jtag_tck() {
    gpio_put(TCK, 1);
    gpio_put(TCK, 0);
}

static void jtag_tms_push(bool value) {
    gpio_put(TMS, value);
    jtag_tck();
}

static void jtag_tdi_push(bool value) {
    gpio_put(TDI, value);
    jtag_tck();
}

static void jtag_reset() {
    gpio_put(TMS, 1);
          
    // go to reset state   
    for (int i=0; i<10; i++) {
        jtag_tck();
    }
}

static void jtag_enter_select_dr() {
    jtag_tms_push(0);
    jtag_tms_push(1); 
} 
 
static void jtag_enter_shift_ir() {
    jtag_tms_push(1);
    jtag_tms_push(0);
    jtag_tms_push(0);   
}

static void jtag_enter_shift_dr() {
    jtag_tms_push(0);
    jtag_tms_push(0);
}

static void jtag_exit_shift() {
    jtag_tms_push(1);
    jtag_tms_push(1);
    jtag_tms_push(1);
}

static void jtag_read_data(idcode_t* value, int length) {
    int bitoffset = 0;
    uint32_t temp;

    length -= 1;
    while (length--) {
        gpio_put(TCK, 1);
        temp = gpio_get(TDO) << bitoffset;
        value->code |= temp;
        gpio_put(TCK, 0);
        bitoffset++;
    }
    gpio_put(TMS, 1);
    gpio_put(TCK, 1);
    temp = gpio_get(TDO) << bitoffset;
    value->code |= temp;
    gpio_put(TCK, 0);

    /* Back to Select-DR */
    jtag_tms_push(1);
    jtag_tms_push(1);
}

static void jtag_read_dr(int length) {
    jtag_enter_shift_dr();
    idcode_t value = {0};
    jtag_read_data(&value, length);
}

static int jtag_get_chain_length() {
    int i;
    gpio_put(TDI, 0);
    for (i = 0; i < MAX_IR_CHAINLENGTH; i++) {
        jtag_tms_push(0);
    }
    gpio_put(TCK, 0);
    gpio_put(TDI, 1);
    for (int i = 0; i < MAX_IR_CHAINLENGTH; i++) {
        gpio_put(TCK, 1);
        if (gpio_get(TDO)) break;
        gpio_put(TCK, 0);
    }
    jtag_exit_shift();
    return i;
}

void jtag_program_start() {
    jtag_enable();
    jtag_reset();
    jtag_enter_select_dr();
    jtag_enter_shift_ir();

    jtag_tdi_push(0);
    jtag_tdi_push(1);
    jtag_tdi_push(0);
    jtag_tdi_push(0);

    jtag_tdi_push(0);
    jtag_tdi_push(0);
    jtag_tdi_push(0);
    jtag_tdi_push(0);
    jtag_tdi_push(0);
    gpio_put(TDI, 0);

    jtag_tms_push(1);
    jtag_tms_push(1);
 
    gpio_put(TDI, 1);
    jtag_tms_push(1);

    jtag_tms_push(0);
    jtag_tms_push(0);
    
    gpio_put(TDI, 1);

    for (int n = 0; n < 5000; n++) {
        jtag_tck();
    }
    gpio_put(TDI, 0);
}

void jtag_program_end() {
    // Exit DR state
    jtag_tms_push(1);
    // Update DR state
    jtag_tms_push(0);
    // Run/Idle state
    jtag_enter_select_dr();
    jtag_enter_shift_ir();
    // Shift IR state
    
    jtag_tdi_push(0);
    jtag_tdi_push(0);
    jtag_tdi_push(1);
    jtag_tdi_push(0);
    jtag_tdi_push(0);
    jtag_tdi_push(0);
    jtag_tdi_push(0);
    jtag_tdi_push(0);
    jtag_tdi_push(0);
    gpio_put(TDI, 0);
    jtag_tms_push(1);
    // Exit IR state
    jtag_tms_push(1);
    jtag_tms_push(1);
    // Select DR scan state
    jtag_tms_push(1);
    jtag_tms_push(0);
    jtag_tms_push(0);
    // Shift IR state
    jtag_tdi_push(1);
    jtag_tdi_push(1);
    jtag_tdi_push(0);
    jtag_tdi_push(0);
    jtag_tdi_push(0);
    jtag_tdi_push(0);
    jtag_tdi_push(0);
    jtag_tdi_push(0);
    jtag_tdi_push(0);
    gpio_put(TDI, 0);
    jtag_tms_push(1);
    //Exit IR state
    jtag_tms_push(1);
    jtag_tms_push(0);
    //Idle state
    for (int n = 0; n < 200; n++) {
        jtag_tck();
    }
    jtag_reset();
    jtag_disable();
}

void jtag_program_postamble() {
    for (int i = 0; i < 127; i++) {
        jtag_tdi_push(1);
    }
    gpio_put(TDI, 1);
    jtag_tms_push(1);
}

void jtag_program_chunk(uint8_t *data, uint16_t length) {
    for (int i = 0; i < length; i++) {
        uint8_t value = data[i];
        for (int j = 0; j < 8; j++) {
            jtag_tdi_push(value & 1);
            value >>= 1;
        }
    }
}

/* TODO: This must be wrong. Not needed so far anyways */
int jtag_scan() {
    jtag_reset();
    jtag_enter_select_dr();
    jtag_enter_shift_ir();
    int irlen = jtag_get_chain_length();
    jtag_enter_shift_dr();
    int device_count = jtag_get_chain_length();

    if (irlen < MAX_IR_CHAINLENGTH && device_count < MAX_IR_CHAINLENGTH) {
        jtag_reset();
        jtag_enter_select_dr();
        jtag_read_dr(32 * device_count);
        return 0;
    } else {
        return 1;
    }
}
