#include "ble_handler.h"

#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_nimble_hci.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include <string.h>
#include "driver/gpio.h"
#include "freertos/task.h"

static const char *TAG_BLE = "BLE_HANDLER";

#define BLE_DEVICE_NAME "RM Scale"
uint8_t ble_addr_type;
void ble_app_advertise(void);

// static TimerHandle_t timer_handler;

// Service UUIDs
#define COMMAND_DATA_SERVICE_UUID 0x1800
// #define BATTERY_SERVICE_UUID 0x180F
// Characteristic UUIDs
#define COMMAND_REQUEST_UUID 0x36, 0xEB, 0xF1, 0x6B, 0x68, 0x4B, 0xEE, 0x11, 0xC1, 0xBA, 0xAC, 0x68, 0x4D, 0x57, 0x34, 0x87
#define COMMAND_RESPONSE_UUID 0xF0, 0x50, 0x35, 0xA4, 0x4B, 0x68, 0x11, 0xEE, 0xA3, 0xE2, 0xD5, 0xAE, 0x87, 0x34, 0x57, 0x4D
// #define BATERY_LEVEL_UUID 0x2A19

const TickType_t delay_ticks = 500 / portTICK_PERIOD_MS;

command_request_handler *command_handler;

static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(COMMAND_DATA_SERVICE_UUID),
        .characteristics = (struct ble_gatt_chr_def[]){
            {
                .uuid = BLE_UUID128_DECLARE(COMMAND_REQUEST_UUID), // Manufacturer Name String
                .flags = BLE_GATT_CHR_F_WRITE,
                .access_cb = command_request_cb,
            },
            {
                .uuid = BLE_UUID128_DECLARE(COMMAND_RESPONSE_UUID), // Model Number String
                .flags = BLE_GATT_CHR_F_WRITE,
                .access_cb = command_response_cb,
            },
            {0}, /* No more characteristics in this service */
        },
    },
    {0}, /* No more services */
};

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
    // xTimerStart(timer_handler, 0);
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

void host_task(void *param)
{
  nimble_port_run();
}

static int command_request_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
  const char *data = (const char *)ctxt->om->om_data;

  return command_handler(data);

    // if (strcmp(my_string, "init") == 0)
  // {
  //   gpio_set_level(12, 0);
  //   // Sleep for 500 milliseconds
  //   vTaskDelay(delay_ticks);
  //   // Set GPIO 12 to low
  //   gpio_set_level(12, 1);
  // }

  // ESP_LOGI("GATT", "Incoming message: %.*s", ctxt->om->om_len, ctxt->om->om_data);
  // return 0;
}

static int command_response_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
  uint8_t battery_level = 80;
  return os_mbuf_append(ctxt->om, &battery_level, sizeof(battery_level));
}

void ble_app_on_sync(void)
{
  ble_hs_id_infer_auto(0, &ble_addr_type);
  ble_app_advertise();
}

void ble_init()
{
  ESP_LOGI(TAG_BLE, "Initialize BLE");
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
    ESP_LOGE(TAG_BLE, "Failed to init nimble %d ", ret);
    return;
  }

  ble_hs_cfg.sync_cb = ble_app_on_sync;

  ble_svc_gap_device_name_set(BLE_DEVICE_NAME);
  ble_svc_gap_init();
  ble_svc_gatt_init();

  ble_gatts_count_cfg(gatt_svr_svcs);
  ble_gatts_add_svcs(gatt_svr_svcs);

  // timer_handler = xTimerCreate("timer_handler", pdMS_TO_TICKS(1000), pdTRUE, NULL, update_battery_status);

  nimble_port_freertos_init(host_task);
}

void ble_config_request_handler(command_request_handler *handler)
{
  command_handler = handler;
}
