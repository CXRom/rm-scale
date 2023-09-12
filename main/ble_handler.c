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
CommandHandler command_main_handler;
void ble_app_advertise(void);

// Service UUIDs
#define GENERIC_ACCESS_SERVICE_UUID 0x1800
// Characteristic UUIDs
#define DEVICE_INFO_MANUFACTURER_NAME_UUID 0x2A29
#define DEVICE_DISPATCH_COMMAND_UUID 0x3000

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

static int command_dispatch_callback(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
  const char *command = (const char *)ctxt->om->om_data;
  ESP_LOGI("GATT", "dispatch command *%s*", command);
  command_main_handler(command);
  return 0;
}

static int device_info_callback(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
  ESP_LOGI("GATT", "dispatch device info");
  const char *value = "Rocket Monsters Scale";
  return os_mbuf_append(ctxt->om, value, strlen(value));
}

static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(GENERIC_ACCESS_SERVICE_UUID),
        .characteristics = (struct ble_gatt_chr_def[]){
            {
                .uuid = BLE_UUID16_DECLARE(DEVICE_INFO_MANUFACTURER_NAME_UUID), // Manufacturer Name String
                .flags = BLE_GATT_CHR_F_READ,
                .access_cb = device_info_callback,
            },
            // Command dispatcher
            {
                .uuid = BLE_UUID16_DECLARE(DEVICE_DISPATCH_COMMAND_UUID),
                .flags = BLE_GATT_CHR_F_WRITE,
                .access_cb = command_dispatch_callback,
            },
            {0}, /* No more characteristics in this service */
        },
    },
    {0}, /* No more services */
};

void ble_app_on_sync(void)
{
  ble_hs_id_infer_auto(0, &ble_addr_type);
  ble_app_advertise();
}

void ble_init(CommandHandler command_handler)
{
  ESP_LOGI(TAG_BLE, "Initialize BLE");
  /* Initialize NVS — it is used to store PHY calibration data */
  command_main_handler = command_handler;
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

  nimble_port_freertos_init(host_task);
}