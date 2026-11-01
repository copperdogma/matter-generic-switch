#pragma once

#include <stdint.h>
#include "esp_err.h"

typedef int ledc_timer_t;
typedef int ledc_mode_t;
typedef int ledc_channel_t;
typedef int ledc_timer_bit_t;
typedef int ledc_clk_cfg_t;

#define LEDC_TIMER_0 0
#define LEDC_LOW_SPEED_MODE 0
#define LEDC_CHANNEL_0 0
#define LEDC_TIMER_10_BIT 10
#define LEDC_AUTO_CLK 0

typedef struct {
    ledc_timer_bit_t duty_resolution;
    uint32_t freq_hz;
    ledc_mode_t speed_mode;
    ledc_timer_t timer_num;
    ledc_clk_cfg_t clk_cfg;
} ledc_timer_config_t;

typedef struct {
    ledc_channel_t channel;
    uint32_t duty;
    int gpio_num;
    ledc_mode_t speed_mode;
    int hpoint;
    ledc_timer_t timer_sel;
} ledc_channel_config_t;

esp_err_t ledc_timer_config(const ledc_timer_config_t *config);
esp_err_t ledc_channel_config(const ledc_channel_config_t *config);
esp_err_t ledc_set_duty(ledc_mode_t mode, ledc_channel_t channel, uint32_t duty);
esp_err_t ledc_update_duty(ledc_mode_t mode, ledc_channel_t channel);
