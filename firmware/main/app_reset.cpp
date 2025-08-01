/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

/* It is recommended to copy this code in your example so that you can modify as per your application's needs,
 * especially for the indicator calbacks, button_factory_reset_pressed_cb() and button_factory_reset_released_cb().
 */

#include <esp_log.h>
#include <esp_matter.h>
#include "iot_button.h"

static const char *TAG = "app_reset";
static bool perform_factory_reset = false;

static void button_factory_reset_pressed_cb(void *arg, void *data)
{
    ESP_LOGI(TAG, "Button pressed - long press detected!");
    if (!perform_factory_reset) {
        ESP_LOGI(TAG, "Factory reset triggered. Release the button to start factory reset.");
        perform_factory_reset = true;
    }
}

static void button_factory_reset_released_cb(void *arg, void *data)
{
    ESP_LOGI(TAG, "Button released!");
    if (perform_factory_reset) {
        ESP_LOGI(TAG, "Starting factory reset");
        esp_matter::factory_reset();
        perform_factory_reset = false;
    }
}

// Simple callback to debug any button press
static void button_any_press_cb(void *arg, void *data)
{
    ESP_LOGI(TAG, "Button pressed (any press detected)!");
}

esp_err_t app_reset_button_register(void *handle)
{
    if (!handle) {
        ESP_LOGE(TAG, "Handle cannot be NULL");
        return ESP_ERR_INVALID_ARG;
    }
    button_handle_t button_handle = (button_handle_t)handle;
    esp_err_t err = ESP_OK;
    
    // Add debug callback for any press
    err |= iot_button_register_cb(button_handle, BUTTON_PRESS_DOWN, NULL, button_any_press_cb, NULL);
    
    err |= iot_button_register_cb(button_handle, BUTTON_LONG_PRESS_HOLD, NULL, button_factory_reset_pressed_cb, NULL);
    err |= iot_button_register_cb(button_handle, BUTTON_PRESS_UP, NULL, button_factory_reset_released_cb, NULL);
    return err;
}
