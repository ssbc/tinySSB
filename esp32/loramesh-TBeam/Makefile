# Makefile

# usage:
#   make compile [SERIAL=/dev/tty...] [BOARD=heltec]    default is for T-Beam
#   make upload  [SERIAL=/dev/tty...]
#   make monitor [SERIAL=/dev/tty...]

# or this usage, which leaves the firmware in the build directory:
#   make firmware [BOARD=heltec]   --> build/firmware-TBeam.bin
#   make flash    [BOARD=heltec]


# place where the arduino-cli executable etc sit:

BINDIR=/opt/homebrew/bin
ESPTOOL=/Users/tschudin/Library/Arduino15/packages/esp32/tools/esptool_py/4.5.1/esptool
BOOTAPP0PATH=/Users/tschudin/Library/Arduino15/packages/esp32/hardware/esp32/2.0.9/tools/partitions/boot_app0.bin

# name of the ino file:
MAIN=loramesh-TBeam.ino

ifeq ($(BOARD),heltec)
  FQBN=esp32:esp32:heltec_wifi_lora_32_V2
  PARTITIONS_CSV=partitions-6.5M.csv
  FLASH_SIZE=8MB
else
  BOARD=TBeam
  FQBN=esp32:esp32:t-beam   # default is T-Beam
  PARTITIONS_CSV=partitions-2.5M.csv
  FLASH_SIZE=4MB
endif

ifndef SERIAL
  ifeq ($(BOARD),heltec) # heltec LORA32
    SERIAL = $(shell ls /dev/tty.usbserial-*)
  else # T-Beam	
    SERIAL= $(shell ls /dev/tty* | grep 'ACM\|USB\|wchusb')
  endif
endif

ifdef FLAG                 #use for sending an extra #def flag to be appended to files, such as FLAG=TBEAM_07
  EXTRA_FLAG=,compiler.cpp.extra_flag=-D$(FLAG)
endif

# ----------------------------------------------------------------------

all:
	$(info Either use "make compile && make upload" or "make firmware && make flash")
	$(info ...option: "BOARD=heltec", default is T-Beam)


compile:
	rm -f partitions.csv
	ln -s $(PARTITIONS_CSV) partitions.csv
	$(BINDIR)/arduino-cli compile -v --fqbn $(FQBN) $(MAIN) \
		--build-properties build.partitions=partitions.csv,upload.maximum_size=1572864$(EXTRA_FLAG)

upload:
	$(foreach var,$(SERIAL), $(BINDIR)/arduino-cli upload -v --fqbn $(FQBN) -p $(var) .;)

monitor:
	$(info use CTRL-A CTRL-\ to quit the screen program)
	screen $(SERIAL) 115200

# ---------------------------------------------------------------------------

firmware:
	mkdir -p build
	rm -f partitions.csv
	ln -s $(PARTITIONS_CSV) partitions.csv
	$(BINDIR)/arduino-cli compile -v \
		--fqbn $(FQBN) loramesh-TBeam.ino \
		--build-property build.partitions=$(PARTITIONS_CSV) \
		--build-property upload.maximum_size=1572864 \
		--build-path ./build   && \
	$(ESPTOOL) \
		--chip esp32 merge_bin --flash_mode dio --flash_freq 80m \
		--flash_size $(FLASH_SIZE) \
		0x1000 ./build/loramesh-TBeam.ino.bootloader.bin \
		0x8000 ./build/loramesh-TBeam.ino.partitions.bin \
		0xe000 $(BOOTAPP0PATH) \
		0x10000 ./build/loramesh-TBeam.ino.bin \
		-o ./build/firmware-$(BOARD).bin

flash:
	$(foreach var,$(SERIAL), \
		/Users/tschudin/Library/Arduino15/packages/esp32/tools/esptool_py/4.5.1/esptool \
		--chip esp32 \
		--port $(var) --baud 921600 \
		--before default_reset --after hard_reset \
		write_flash -z --flash_mode dio --flash_freq 80m \
		--flash_size $(FLASH_SIZE) \
		0x0 ./build/firmware-$(BOARD).bin;)

clean:
	rm -f *~
	rm -f partitions.csv
	rm -rf build/*

# eof
