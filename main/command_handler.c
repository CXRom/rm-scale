#include "command_handler.h"

#include <string.h>
#include <freertos/FreeRTOS.h>
#include "freertos/task.h"
#include "driver/gpio.h"

const TickType_t delay_ticks = 500 / portTICK_PERIOD_MS;

void handle_command(const char *command)
{
  if (strcmp(command, "init") == 0)
  {
    gpio_set_level(12, 0);
    // Sleep for 500 milliseconds
    vTaskDelay(delay_ticks);
    // Set GPIO 12 to low
    gpio_set_level(12, 1);
  }
}
