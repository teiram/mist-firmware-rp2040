###############################################################################
# Binary for Calypso board

set(calypso_driver_src
  ../drivers/fpga_calypso.c
  ../drivers/cyclone_jtag.c
  ../drivers/pio_spi.c
  ../drivers/ps2.c
  ../drivers/sdcard.c
  ../drivers/bitfile.c
  ../drivers/midi.c
  ../drivers/debug.c
  ../drivers/fifo.c
  ../drivers/kbd.c
  ../drivers/joystick_calypso.c
  ../drivers/crc16.c
)

add_executable(mist-calypso
  ${calypso_driver_src}
  ${main_src}
  ${picosynth_src}
  ${usb_src}
  )

target_link_libraries(mist-calypso
  pico_stdlib 
  mistfirmwareusb 
  hardware_spi 
  hardware_pio 
  hardware_flash 
  hardware_i2c 
  pico_multicore
  ${picosynth_lib}
  tinyusb_host 
  tinyusb_board)
target_include_directories(mist-calypso PUBLIC ../usbhost)
target_include_directories(mist-calypso PUBLIC ../picosynth)
add_compile_options(-O2)
pico_generate_pio_header(mist-calypso ../drivers/fpga.pio)
pico_generate_pio_header(mist-calypso ../drivers/spi.pio)
pico_generate_pio_header(mist-calypso ../drivers/jamma.pio)
pico_generate_pio_header(mist-calypso ../drivers/jammadb9.pio)
pico_generate_pio_header(mist-calypso ../drivers/audio_i2s.pio)
pico_generate_pio_header(mist-calypso ../drivers/ps2.pio)
pico_generate_pio_header(mist-calypso ../drivers/ps2tx.pio)
pico_enable_stdio_usb(mist-calypso 0)
pico_enable_stdio_uart(mist-calypso 1)
pico_add_extra_outputs(mist-calypso)
example_auto_set_url(mist-calypso)
target_compile_options(mist-calypso PUBLIC
  "-DVDATE=\"\""
  "-Dhexdump=driver_hexdump"
  "-DPS2HOST"
  "-DUSB"
  "-DMIST_USB"
  "-DCALYPSO"
#  "-DUSB_STORAGE"
#  "-DPICOSYNTH"
   "-DUSB_DEBUG_OFF"
#   "-DDEBUG"
#  "-DBUFFER_FPGA"
)

