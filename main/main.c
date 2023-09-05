#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_timer.h"

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"

#include "ble_handler.h"
#include "hx711_handler.h"
#include "display_handler.h"

const char *TAG_MAIN = "rm.scale";

#define I2C_MASTER I2C_NUM_0
#define I2C_MAIN_SPEED (400 * 1000)
#define I2C_MAIN_SDA 4
#define I2C_MAIN_SCL 5

i2c_config_t i2c_conf = {
    .mode = I2C_MODE_MASTER,
    .sda_io_num = I2C_MAIN_SDA,
    .scl_io_num = I2C_MAIN_SCL,
    .sda_pullup_en = GPIO_PULLUP_ENABLE,
    .scl_pullup_en = GPIO_PULLUP_ENABLE,
    .master.clk_speed = I2C_MAIN_SPEED,
};

int command_request_cb(char *message)
{
    ESP_LOGI(TAG_MAIN, "message: %s", message);
    return 0;
}

void app_main(void)
{
    i2c_init(&i2c_conf, I2C_MASTER);
    display_init(&i2c_conf);

    ble_init();
    ble_config_request_handler(command_request_cb);

    esp_rom_gpio_pad_select_gpio(12);
    gpio_set_direction(12, GPIO_MODE_OUTPUT);
    gpio_set_level(12, 1);

    xTaskCreate(hx_task, "hx_task", configMINIMAL_STACK_SIZE * 5, NULL, 5, NULL);
}
