// cmd.h

// tinySSB for ESP32
// 2022-2023 <christian.tschudin@unibas.ch>

#ifndef _INCLUDE_CMD_H
#define _INCLUDE_CMD_H

#include "tinySSBlib.h"

void listDir(fs::FS &fs, const char *dirname, uint8_t levels);
void cmd_rx(String cmd);

#endif

// eof
