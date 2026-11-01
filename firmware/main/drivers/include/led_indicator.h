#pragma once

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the LED indicator
 * 
 * @param gpio_num GPIO pin number for the LED
 * @return esp_err_t ESP_OK on success, error code otherwise
 */
esp_err_t led_indicator_init(int gpio_num);

/**
 * @brief Set the LED indicator to dim brightness
 * 
 * @return esp_err_t ESP_OK on success, error code otherwise
 */
esp_err_t led_indicator_set_dim(void);

/**
 * @brief Flash the LED three times for a button event, then turn off
 * 
 * @return esp_err_t ESP_OK on success, error code otherwise
 */
esp_err_t led_indicator_set_blink(void);

/**
 * @brief Set the LED indicator to full brightness (active)
 * 
 * @return esp_err_t ESP_OK on success, error code otherwise
 */
esp_err_t led_indicator_set_bright(void);

/**
 * @brief Turn the LED completely off
 * 
 * @return esp_err_t ESP_OK on success, error code otherwise
 */
esp_err_t led_indicator_set_off(void);

/**
 * @brief Deinitialize the LED indicator
 * 
 * @return esp_err_t ESP_OK on success, error code otherwise
 */
esp_err_t led_indicator_deinit(void);

#ifdef __cplusplus
}
#endif
