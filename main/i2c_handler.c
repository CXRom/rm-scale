#include "i2c_handler.h"

#include "esp_err.h"
#include "esp_log.h"
#include "driver/i2c.h"

char *TAG_I2C = "rm.scale.i2c";
char *TAG_MAIN = "rm.scale";

int i2c_init(i2c_config_t *config)
{
  ESP_LOGI(TAG_I2C, "Initialize I2C bus");
  i2c_config_t i2c_config = *config;

  int result = 0;
  result = result + ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER, &i2c_config));
  result = result + ESP_ERROR_CHECK(i2c_driver_install(I2C_MASTER, i2c_config.mode, 0, 0, 0));

  return result;
}