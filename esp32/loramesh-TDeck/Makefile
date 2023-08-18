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
BOOTAPP0PATH=/Users/tschudin/Library/Arduino15/packages/esp32/hardware/esp32/2.0.11/tools/partitions/boot_app0.bin

# name of the ino file:
MAIN=loramesh-TDeck.ino

FLASH_OFFS=0x0
FLASH_BIN=firmware-$(BOARD).bin

ifeq ($(BOARD),heltec)
  CHIP=esp32
  FQBN=esp32:esp32:heltec_wifi_lora_32_V2
  PARTITIONS_CSV=partitions-6.5M.csv
  MAX_UPLOAD=1572864 # 1.5MB
  FLASH_SIZE=8MB
else ifeq ($(BOARD),TDeck)
  CHIP=esp32s3
  FQBN=esp32:esp32:esp32s3:PSRAM=opi,FlashSize=16M,CDCOnBoot=cdc
  PARTITIONS_CSV=partitions-tdeck.csv
  MAX_UPLOAD=2097152 # 2MB
  FLASH_SIZE=16MB
  FLASH_OFFS=0x10000
  FLASH_BIN=$(MAIN).bin
else    # default is T-Beam
  CHIP=esp32
  BOARD=TBeam
  FQBN=esp32:esp32:t-beam
  PARTITIONS_CSV=partitions-2.5M.csv
  MAX_UPLOAD=1572864 # 1.5MB
  FLASH_SIZE=4MB
endif

ifndef SERIAL
  ifeq ($(BOARD),heltec) # heltec LORA32
    SERIAL = $(shell ls /dev/tty.usbserial-*)
  else ifeq ($(BOARD),TDeck)
    SERIAL = $(shell ls /dev/tty.usbmodem*)
    # $(foreach var,$(SERIAL), fuser -f $(var);)
  else # T-Beam
    SERIAL = $(shell ls /dev/tty.wchusbserial*)
  endif
endif

ifdef FLAG                 #use for sending an extra #def flag to be appended to files, such as FLAG=-DTBEAM_07
  EXTRA_CPP_FLAGS=--build-property compiler.cpp.extra_flags=$(FLAG)
else
  EXTRA_CPP_FLAGS=--build-property compiler.cpp.extra_flags="-DESP32 -DBOARD_HAS_PSRAM -DARDUINO_USB_MODE=1 -DARDUINO_USB_CDC_ON_BOOT=1 -DARDUINO_USB_MSC_ON_BOOT=0 -DARDUINO_USB_DFU_ON_BOOT=0"
endif

EXTRA_BUILD_FLAGS=--build-property build.extra_flags="-DUTC_OFFSET=\"$(shell date +%z)\""

# ----------------------------------------------------------------------

all:
	$(info Either use "make compile && make upload" or "make firmware && make flash")
	$(info ...option: "BOARD=heltec", default is T-Beam)


compile:
	rm -f partitions.csv
	ln -s $(PARTITIONS_CSV) partitions.csv
	$(BINDIR)/arduino-cli compile -v --fqbn $(FQBN) $(MAIN) \
		--build-properties build.partitions=partitions.csv,upload.maximum_size=1572864 \
		$(EXTRA_CPP_FLAG)

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
		--build-path ./build \
		--fqbn $(FQBN) $(MAIN) \
		--build-property build.partitions=$(PARTITIONS_CSV) \
		--build-property upload.maximum_size=$(MAX_UPLOAD) \
		$(EXTRA_BUILD_FLAGS) \
		$(EXTRA_CPP_FLAGS) \
		&& \
	$(ESPTOOL) \
		--chip $(CHIP) merge_bin --flash_mode dio --flash_freq 80m \
		--flash_size $(FLASH_SIZE) \
		0x0     ./build/$(MAIN).bootloader.bin \
		0x8000  ./build/$(MAIN).partitions.bin \
		0xe000  $(BOOTAPP0PATH) \
		0x10000 ./build/$(MAIN).bin \
		-o ./build/firmware-$(BOARD).bin

flash:
	 $(foreach var,$(SERIAL), \
		/Users/tschudin/Library/Arduino15/packages/esp32/tools/esptool_py/4.5.1/esptool \
		--chip $(CHIP) \
		--port $(var) --baud 921600 \
		--before default_reset --after hard_reset \
		write_flash -z --flash_mode dio --flash_freq 80m \
		--flash_size $(FLASH_SIZE) \
		 $(FLASH_OFFS) ./build/$(FLASH_BIN);)


clean:
	rm -f *~
	rm -f partitions.csv
	rm -rf build/*

# eof
