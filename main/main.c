#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_timer.h"
#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_log.h"

#include "ble_handler.h"
#include "hx711_handler.h"
#include "display_handler.h"

const char *TAG = "rm.scale";

#define I2C_MASTER I2C_NUM_0
#define I2C_MAIN_SPEED (400 * 1000)
#define I2C_MAIN_SDA 4
#define I2C_MAIN_SCL 5

// uint8_t battery_level = 100;
// void update_battery_status(TimerHandle_t xTimer)
// {
//     if (battery_level > 0)
//     {
//         battery_level--;
//     }
//     else
//     {
//         battery_level = 100;
//     }

//     ESP_LOGI(TAG, "Battery level: %d", battery_level);
// }

void app_main(void)
{
    ESP_LOGI(TAG, "Initialize I2C bus");
    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MAIN_SDA,
        .scl_io_num = I2C_MAIN_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MAIN_SPEED,
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER, &i2c_conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_MASTER, i2c_conf.mode, 0, 0, 0));

    ESP_LOGI(TAG, "Initialize SSD1306");
    display_init(I2C_MASTER);

    ESP_LOGI(TAG, "Initialize BLE");
    ble_init();

    xTaskCreate(hx_task, "hx_task", configMINIMAL_STACK_SIZE * 5, NULL, 5, NULL);
}
