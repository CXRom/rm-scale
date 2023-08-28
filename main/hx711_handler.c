#include "hx711_handler.h"

#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "hx711.h"

#include "display_handler.h"

#define HX_DOUT_GPIO 3
#define HX_SCK_GPIO 2

const char *TAG_SCALE = "rm.scale";

hx711_t HX_HOST = {
    .dout = HX_DOUT_GPIO,
    .pd_sck = HX_SCK_GPIO,
    .gain = HX711_GAIN_A_64};

void hx_task(void *pvParameters)
{
  ESP_LOGI(TAG_SCALE, "Initialize hx711");
  ESP_ERROR_CHECK(hx711_init(&HX_HOST));

  // read from device
  while (true)
  {
    esp_err_t r = hx711_wait(&HX_HOST, 100);
    if (r != ESP_OK)
    {
      ESP_LOGE(TAG_SCALE, "Device not found: %d (%s)\n", r, esp_err_to_name(r));
      continue;
    }

    int32_t hx_value;
    r = hx711_read_data(&HX_HOST, &hx_value);
    if (r != ESP_OK)
    {
      ESP_LOGE(TAG_SCALE, "Could not read data: %d (%s)\n", r, esp_err_to_name(r));
      continue;
    }

    ESP_LOGI(TAG_SCALE, "%" PRIi32, hx_value);

    char message[20];
    sprintf(message, "%" PRIi32, hx_value);
    display_message(message);

    vTaskDelay(pdMS_TO_TICKS(50));
  }
}