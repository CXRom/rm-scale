#include "driver/i2c.h"
#include "ssd1306.h"

#ifndef __DISPLAY_HANDLER_H
#define __DISPLAY_HANDLER_H

#define DISPLAY_RESET_PIN -1
#define DISPLAY_I2C_ADDR 0x3C
#define DISPLAY_LCD_H_RES 128
#define DISPLAY_LCD_V_RES 64
#define DISPLAY_LCD_CMD_BITS 8

void display_init(i2c_port_t i2c_master);
void display_message(char *message);

#endif