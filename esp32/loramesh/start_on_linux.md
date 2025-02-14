# How to compile and flash tinySSB to your Lilygo T-Beam device

(or: how to start tinySSB dev on MacOS and Linux)

Connect your T-Beam via USB to your laptop, then execute the following commands

```
% curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh
% arduino-cli version  # this is a smoke test - I have 1.0.4
% git clone https://github.com/ssbc/tinySSB.git
% cd tinySSB/esp32/loramesh
% git checkout cft-twatch
% make BOARD=TBeam firmware flash
```
