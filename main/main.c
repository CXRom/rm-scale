#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include "freertos/task.h"
#include "esp_timer.h"
#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_log.h"

#include "nvs_flash.h"
#include "esp_nimble_hci.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"

#include "hx711_handler.h"
#include "display_handler.h"

const char *TAG = "rm.scale";

#define I2C_MASTER I2C_NUM_0
#define I2C_MAIN_SPEED (400 * 1000)
#define I2C_MAIN_SDA 4
#define I2C_MAIN_SCL 5

void ble_store_util_sync_cb(void)
{
    ble_addr_t addr;
    ble_hs_id_gen_rnd(1, &addr);
    ble_hs_id_set_rnd(addr.val);

    uint8_t uuid128[16];
    memset(uuid128, 0x11, sizeof(uuid128));
    ble_ibeacon_set_adv_data(uuid128, 2, 10, -50);

    struct ble_gap_adv_params adv_params = (struct ble_gap_adv_params){0};
    ble_gap_adv_start(BLE_OWN_ADDR_RANDOM, NULL, BLE_HS_FOREVER, &adv_params, NULL, NULL);
}

void host_task(void *param)
{
    nimble_port_run();
}

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

    ESP_LOGI(TAG, "Initialize BLE");
    nvs_flash_init();
    int ret = nimble_port_init();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "nimble_port_init() failed with error: %d", ret);
        return;
    }

    ble_hs_cfg.sync_cb = ble_store_util_sync_cb;
    nimble_port_freertos_init(host_task);

    ESP_LOGI(TAG, "Initialize SSD1306");
    display_init(I2C_MASTER);

    xTaskCreate(hx_task, "hx_task", configMINIMAL_STACK_SIZE * 5, NULL, 5, NULL);
}
