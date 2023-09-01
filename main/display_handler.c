#include "driver/i2c.h"
#include "ssd1306.h"

#include "display_handler.h"

static ssd1306_handle_t display_handler = NULL;

void display_init(i2c_port_t i2c_master)
{
  display_handler = ssd1306_create(i2c_master, DISPLAY_I2C_ADDR);
  ssd1306_refresh_gram(display_handler);
  ssd1306_clear_screen(display_handler, 0x00);
}

void display_message(char *message)
{

  char title_str[20], value_str[20] = {0};
  sprintf(title_str, "Rocket Monsters");
  sprintf(value_str, "%s", message);

  ssd1306_clear_screen(display_handler, 0x00);
  ssd1306_draw_string(display_handler, 0, 0, (const uint8_t *)title_str, 16, 1);
  ssd1306_draw_string(display_handler, 0, 20, (const uint8_t *)value_str, 16, 1);
  ssd1306_refresh_gram(display_handler);
}