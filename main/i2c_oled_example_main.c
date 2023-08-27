#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_log.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"
#include <hx711.h>

#include "esp_lcd_panel_vendor.h"

static const char *TAG = "rm.scale";

#define I2C_HOST 0
#define I2C_MAIN_SPEED (400 * 1000)
#define I2C_MAIN_SDA 4
#define I2C_MAIN_SCL 5

#define DISPLAY_RESET_PIN -1
#define DISPLAY_I2C_ADDR 0x3C
#define DISPLAY_LCD_H_RES 128
#define DISPLAY_LCD_V_RES 64
#define DISPLAY_LCD_CMD_BITS 8

#define HX_DOUT_GPIO 3
#define HX_SCK_GPIO 2

lv_disp_t *display;
hx711_t HX_HOST = {
    .dout = HX_DOUT_GPIO,
    .pd_sck = HX_SCK_GPIO,
    .gain = HX711_GAIN_A_64};

extern void displayMessage(lv_disp_t *disp, char *message);

static bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    lv_disp_t *disp = (lv_disp_t *)user_ctx;
    lvgl_port_flush_ready(disp);
    return false;
}

void hx_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Initialize hx711");
    ESP_ERROR_CHECK(hx711_init(&HX_HOST));

    // read from device
    while (1)
    {
        esp_err_t r = hx711_wait(&HX_HOST, 50);
        if (r != ESP_OK)
        {
            ESP_LOGE(TAG, "Device not found: %d (%s)\n", r, esp_err_to_name(r));
            continue;
        }

        int32_t hx_value;
        r = hx711_read_data(&HX_HOST, &hx_value);
        if (r != ESP_OK)
        {
            ESP_LOGE(TAG, "Could not read data: %d (%s)\n", r, esp_err_to_name(r));
            continue;
        }

        char str_buffer[30];
        sprintf(str_buffer, "%" PRIi32, hx_value);
        displayMessage(display, str_buffer);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
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
    ESP_ERROR_CHECK(i2c_param_config(I2C_HOST, &i2c_conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_HOST, I2C_MODE_MASTER, 0, 0, 0));

    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t io_config = {
        .dev_addr = DISPLAY_I2C_ADDR,
        .control_phase_bytes = 1,
        .lcd_cmd_bits = DISPLAY_LCD_CMD_BITS,
        .lcd_param_bits = DISPLAY_LCD_CMD_BITS,

        .dc_bit_offset = 6,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)I2C_HOST, &io_config, &io_handle));

    ESP_LOGI(TAG, "Install SSD1306 panel driver");
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .bits_per_pixel = 1,
        .reset_gpio_num = DISPLAY_RESET_PIN,
    };

    ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    ESP_LOGI(TAG, "Initialize LVGL");
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    lvgl_port_init(&lvgl_cfg);

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = DISPLAY_LCD_H_RES * DISPLAY_LCD_V_RES,
        .double_buffer = false,
        .hres = DISPLAY_LCD_H_RES,
        .vres = DISPLAY_LCD_V_RES,
        .monochrome = true,
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        }};
    display = lvgl_port_add_disp(&disp_cfg);
    /* Register done callback for IO */
    const esp_lcd_panel_io_callbacks_t cbs = {
        .on_color_trans_done = notify_lvgl_flush_ready,
    };
    esp_lcd_panel_io_register_event_callbacks(io_handle, &cbs, display);

    /* Rotation of the screen */
    lv_disp_set_rotation(display, LV_DISP_ROT_NONE);

    xTaskCreate(hx_task, "hx_task", configMINIMAL_STACK_SIZE * 5, NULL, 5, NULL);
}

void displayMessage(lv_disp_t *disp, char *message)
{
    lv_obj_clean(lv_disp_get_scr_act(disp));
    lv_obj_t *scr = lv_disp_get_scr_act(disp);

    lv_obj_t *label = lv_label_create(scr);                       /*First parameters (scr) is the parent*/
    lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR); /* Circular scroll */
    lv_label_set_text(label, message);

    lv_obj_t *footer = lv_label_create(scr);
    lv_label_set_long_mode(footer, LV_LABEL_LONG_SCROLL); // Break the text into lines
    lv_label_set_text(footer, "Rocket Monsters");

    /* Size of the screen (if you use rotation 90 or 270, please set disp->driver->ver_res) */
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 0);     // Top alignment for the first label
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, 0); // Bottom alignment for the second label
}