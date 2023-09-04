#include "driver/i2c.h"
#include "ssd1306.h"

#ifndef __DISPLAY_HANDLER_H
#define __DISPLAY_HANDLER_H

void display_init(i2c_port_t *i2c_master_ptr);
void display_message(char *message);

#endif