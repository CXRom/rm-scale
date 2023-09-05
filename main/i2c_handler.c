#include "i2c_handler.h"

#include "esp_err.h"
#include "esp_log.h"
#include "driver/i2c.h"

const char *TAG_I2C = "rm.scale.i2c";

int i2c_init(i2c_config_t *config, i2c_port_t port)
{
  ESP_LOGI(TAG_I2C, "Initialize I2C bus");
  int result = 0;
  i2c_param_config(port, config);
  i2c_driver_install(port, config->mode, 0, 0, 0);

  return result;
}