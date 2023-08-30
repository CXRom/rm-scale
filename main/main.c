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

#define BLE_DEVICE_NAME "RM Scale"
uint8_t ble_addr_type;

#define I2C_MASTER I2C_NUM_0
#define I2C_MAIN_SPEED (400 * 1000)
#define I2C_MAIN_SDA 4
#define I2C_MAIN_SCL 5

void ble_app_advertise(void);

static int ble_gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type)
    {
    case BLE_GAP_EVENT_CONNECT:
        ESP_LOGI("GAP", "BLE_GAP_EVENT_CONNECT %s", event->connect.status == 0 ? "OK" : "FAIL");
        if (event->connect.status != 0)
        {
            ble_app_advertise();
        }
        break;
    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI("GAP", "BLE_GAP_EVENT_DISCONNECT");
        ble_app_advertise();
        break;
    case BLE_GAP_EVENT_ADV_COMPLETE:
        ESP_LOGI("GAP", "BLE_GAP_EVENT_ADV_COMPLETE");
        ble_app_advertise();
        break;
    case BLE_GAP_EVENT_SUBSCRIBE:
        ESP_LOGI("GAP", "BLE_GAP_EVENT_SUBSCRIBE");
        break;
    default:
        break;
    }

    return 0;
}

void ble_app_advertise(void)
{
    struct ble_hs_adv_fields fields;
    memset(&fields, 0, sizeof(fields));

    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_DISC_LTD;
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

    fields.name = (uint8_t *)BLE_DEVICE_NAME;
    fields.name_len = strlen(BLE_DEVICE_NAME);
    fields.name_is_complete = 1;

    ble_gap_adv_set_fields(&fields);

    struct ble_gap_adv_params adv_params;
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    ble_gap_adv_start(ble_addr_type, NULL, BLE_HS_FOREVER, &adv_params, ble_gap_event, NULL);
}

void ble_app_on_sync(void)
{
    ble_hs_id_infer_auto(0, &ble_addr_type);
    ble_app_advertise();
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
    /* Initialize NVS — it is used to store PHY calibration data */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ret = nimble_port_init();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to init nimble %d ", ret);
        return;
    }

    ble_hs_cfg.sync_cb = ble_app_on_sync;

    ble_svc_gap_device_name_set(BLE_DEVICE_NAME);
    ble_svc_gap_init();

    nimble_port_freertos_init(host_task);

    ESP_LOGI(TAG, "Initialize SSD1306");
    display_init(I2C_MASTER);

    xTaskCreate(hx_task, "hx_task", configMINIMAL_STACK_SIZE * 5, NULL, 5, NULL);
}
