# Description
This is a fork of zxmicrojack [mist-firmware-rp2040](https://github.com/ZXMicroJack/mist-firmware-rp2040) to support the middleboard [Calypso](https://github.com/teiram/calypso-cyc1000-board). It is a preliminary work to validate the board design and eventually will be replaced by a custom firmware. 
# Build instructions
Even though the original firmware was intended to be built under Docker, I found it more convenient to build with local tools. You will just need the Pico SDK properly installed (refer to the instructions provided by Raspberry to do so) and CMake. Microjack chose to use the Build directory to place source code so in order to not pollute it with the built artifacts, I followed this approach:
- Create a folder at the source top level named dist
- Change into the dist directory
- Execute: cmake ../build
- Now the dist folder will hold all your generated makefiles and artifacts so it would be easy to exclude them from version control
- In order to generate the artifact for the calypso, just run inside the dist directory: make mist-calypso
