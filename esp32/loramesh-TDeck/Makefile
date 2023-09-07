# Makefile

# ** usage:

# make [BOARD=..] [SERIAL=/dev/tty..] {firmware} {flush}

#    known boards are: TBeam, TDeck, heltec
#    serial device: we have some heuristics for setting it automatically


# ** examples:

# make BOARD=heltec firmware     --> creates 'build/firmware-heltec.bin'
# make BOARD=heltec flash

#    these two phases can be combined:

# make BOARD=heltec firmware flash


# ** special flag for old TBeam:

#   make BOARD=TBeam FLAG=TBEAM_07 firmware flash


# ---------------------------------------------------------------------------

# adjust the following: places where the arduino-cli executable etc sit

ARDUINO_CLI=$(shell which arduino-cli;)
ARDUINO_DATA=$(shell arduino-cli config dump |awk '/data/ {print $$2}';)
ARDUINO_ESP_DIR=$(ARDUINO_DATA)/packages/esp32

# ---------------------------------------------------------------------------

OS=$(shell uname;)
ifneq ($(OS), Darwin)
  ESPTOOL_EXT=.py
endif

ESPTOOL=$(shell find $(ARDUINO_ESP_DIR)/tools -name "esptool$(ESPTOOL_EXT)";)
BOOTAPP0PATH=$(shell find $(ARDUINO_ESP_DIR)/hardware -name "boot_app0.bin";)

ifdef BOARD
else
  $(info ">> please specify a BOARD")
  stop
endif

MAIN=$(shell ls *.ino)

BAUD=921600
FLASH_BIN=firmware-$(BOARD).bin

FLAGS_ALL=-DUTC_OFFSET=\"$(shell date +%z)\"

ifeq ($(BOARD),heltec)
  CHIP=esp32
  FQBN=esp32:esp32:heltec_wifi_lora_32_V2
  PARTITIONS_CSV=partitions-heltec.csv
  MAX_UPLOAD=1572864 # 1.5MB
  FLASH_SIZE=8MB
  FLASH_OFFS=0x1000
else ifeq ($(BOARD),TBeam)
  CHIP=esp32
  FQBN=esp32:esp32:t-beam
  PARTITIONS_CSV=partitions-tbeam.csv
  MAX_UPLOAD=1572864 # 1.5MB
  FLASH_SIZE=4MB
  FLASH_OFFS=0x1000
  FLAGS_BOARD=-DTINYSSB_BOARD_TBEAM -DESP32 -DCORE_DEBUG_LEVEL=0 -DARDUINO=10607 -DARDUINO_TBEAM_USE_RADIO_SX1262 -DARDUINO_ARCH_ESP32 -DARDUINO_BOARD=TBEAM_USE_RADIO_SX1262 -DARDUINO_VARIANT=tbeam -DARDUINO_USB_CDC_ON_BOOT=0
else ifeq ($(BOARD),TDeck)
  CHIP=esp32s3
  FQBN=esp32:esp32:esp32s3:PSRAM=opi,FlashSize=16M,CDCOnBoot=cdc
  PARTITIONS_CSV=partitions-tdeck.csv
  MAX_UPLOAD=2097152 # 2MB
  FLASH_SIZE=16MB
  FLASH_OFFS=0x0
  FLAGS_BOARD=-DTINYSSB_BOARD_TDECK -DESP32 -DCORE_DEBUG_LEVEL=0 -DARDUINO_RUNNING_CORE=1 -DARDUINO_EVENT_RUNNING_CORE=1 -DBOARD_HAS_PSRAM -DARDUINO_USB_MODE=1 -DARDUINO_USB_CDC_ON_BOOT=1 -DARDUINO_USB_MSC_ON_BOOT=0 -DARDUINO_USB_DFU_ON_BOOT=0
else ifeq ($(BOARD),TWrist)
  BAUD=460800
  CHIP=esp32
  FQBN=esp32:esp32:esp32
  PARTITIONS_CSV=partitions-twrist.csv
  MAX_UPLOAD=1572864 # 1.5MB
  FLASH_SIZE=4MB
  FLASH_OFFS=0x1000
  FLAGS_BOARD=-DTINYSSB_BOARD_TWRIST -DESP32 -DCORE_DEBUG_LEVEL=0 -DARDUINO_RUNNING_CORE=1 -DARDUINO_EVENT_RUNNING_CORE=1 -DARDUINO_USB_CDC_ON_BOOT=0
else
  $(info ">> unknown board $(BOARD)")
  stop
endif

ifndef SERIAL
  ifeq ($(BOARD),heltec) # heltec LORA32
    SERIAL = $(shell ls /dev/tty.usbserial-????)
  else ifeq ($(BOARD),TDeck)
    SERIAL = $(shell ls /dev/tty.usbmodem*)
  else ifeq ($(BOARD),TWrist)
    SERIAL = $(shell ls /dev/tty.usbserial-???????????)
  else # T-Beam
    SERIAL= $(shell ls /dev/tty* | grep 'ACM\|USB\|wchusb')
  endif

  # ? kill any screen/miniterm program currently having the serial port open ?
  # $(foreach var,$(SERIAL), fuser -f $(var);) ...
endif

EXTRA_CPP_FLAGS=--build-property compiler.cpp.extra_flags="$(FLAGS_ALL) $(FLAGS_BOARD) $(FLAG)"

# ----------------------------------------------------------------------

all:
	$(info )
	$(info "usage:  make [BOARD=TBeam|TDeck|heltec] firmware flash")
	$(info )

monitor:
	$(info use CTRL-A CTRL-\ to quit the screen program)
	screen -h 1024 $(SERIAL) 115200

# ---------------------------------------------------------------------------

firmware:
	rm -f partitions.csv
	ln -s src/$(PARTITIONS_CSV) partitions.csv
	mkdir -p build
	$(ARDUINO_CLI) compile -v \
		--build-path ./build \
		--fqbn $(FQBN) $(MAIN) \
		--build-property build.partitions=$(PARTITIONS_CSV) \
		--build-property upload.maximum_size=$(MAX_UPLOAD) \
		$(EXTRA_CPP_FLAGS) \
		&& \
	$(ESPTOOL) \
		--chip $(CHIP) merge_bin --flash_mode dio --flash_freq 80m \
		--flash_size $(FLASH_SIZE) \
		$(FLASH_OFFS) ./build/$(MAIN).bootloader.bin \
		0x8000        ./build/$(MAIN).partitions.bin \
		0xe000        $(BOOTAPP0PATH) \
		0x10000       ./build/$(MAIN).bin \
		-o ./build/$(FLASH_BIN)

flash:
	 $(foreach var,$(SERIAL), \
		$(ESPTOOL) \
		--chip $(CHIP) \
		--port $(var) --baud $(BAUD) \
		--before default_reset --after hard_reset \
		write_flash -z --flash_mode dio --flash_freq 80m \
		--flash_size $(FLASH_SIZE) \
		0x0 ./build/$(FLASH_BIN);)

clean:
	rm -f *~
	rm -f partitions.csv
	rm -rf build/*

# eof
