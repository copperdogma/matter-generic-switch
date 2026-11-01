#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "driver/ledc.h"
#include "drivers/include/led_indicator.h"
#include "esp_timer.h"

#define EXPECTED_BRIGHT_DUTY 1023U

static uint32_t duty_history[128];
static size_t duty_history_count;
static uint32_t current_duty;

struct fake_esp_timer {
    esp_timer_cb_t callback;
    const char *name;
    uint64_t period_us;
    bool active;
};

static struct fake_esp_timer timer_instance;
static esp_timer_handle_t timer_handle;

esp_err_t ledc_timer_config(const ledc_timer_config_t *config)
{
    (void)config;
    return ESP_OK;
}

esp_err_t ledc_channel_config(const ledc_channel_config_t *config)
{
    (void)config;
    return ESP_OK;
}

esp_err_t ledc_set_duty(ledc_mode_t mode, ledc_channel_t channel, uint32_t duty)
{
    (void)mode;
    (void)channel;
    current_duty = duty;
    assert(duty_history_count < sizeof(duty_history) / sizeof(duty_history[0]));
    duty_history[duty_history_count++] = duty;
    return ESP_OK;
}

esp_err_t ledc_update_duty(ledc_mode_t mode, ledc_channel_t channel)
{
    (void)mode;
    (void)channel;
    return ESP_OK;
}

esp_err_t esp_timer_create(const esp_timer_create_args_t *args, esp_timer_handle_t *out_handle)
{
    timer_instance.callback = args->callback;
    timer_instance.name = args->name;
    timer_instance.period_us = 0;
    timer_instance.active = false;
    timer_handle = &timer_instance;
    *out_handle = timer_handle;
    return ESP_OK;
}

esp_err_t esp_timer_start_periodic(esp_timer_handle_t timer, uint64_t period_us)
{
    assert(timer == timer_handle);
    assert(!timer->active);
    timer->period_us = period_us;
    timer->active = true;
    return ESP_OK;
}

esp_err_t esp_timer_stop(esp_timer_handle_t timer)
{
    assert(timer == timer_handle);
    timer->active = false;
    return ESP_OK;
}

esp_err_t esp_timer_delete(esp_timer_handle_t timer)
{
    assert(timer == timer_handle);
    timer->active = false;
    timer_handle = NULL;
    return ESP_OK;
}

bool fake_esp_timer_is_active(void)
{
    return timer_handle != NULL && timer_handle->active;
}

void fake_esp_timer_fire(void)
{
    if (fake_esp_timer_is_active()) {
        timer_handle->callback(NULL);
    }
}

static size_t count_edges(size_t begin, uint32_t from, uint32_t to)
{
    size_t count = 0;
    uint32_t previous = begin == 0 ? 0 : duty_history[begin - 1];
    for (size_t i = begin; i < duty_history_count; ++i) {
        if (previous == from && duty_history[i] == to) {
            ++count;
        }
        previous = duty_history[i];
    }
    return count;
}

static void run_complete_sequence(void)
{
    const size_t history_start = duty_history_count;
    assert(led_indicator_set_blink() == ESP_OK);
    assert(fake_esp_timer_is_active());
    assert(current_duty == EXPECTED_BRIGHT_DUTY);

    size_t ticks = 0;
    while (fake_esp_timer_is_active() && ticks < 8) {
        fake_esp_timer_fire();
        ++ticks;
    }

    assert(ticks == 5);
    assert(!fake_esp_timer_is_active());
    assert(current_duty == 0);
    assert(count_edges(history_start, 0, EXPECTED_BRIGHT_DUTY) == 3);
    assert(count_edges(history_start, EXPECTED_BRIGHT_DUTY, 0) == 3);
}

int main(void)
{
    assert(led_indicator_init(5) == ESP_OK);
    assert(current_duty == 0);
    assert(!fake_esp_timer_is_active());

    // A completed sequence leaves the LED off and the periodic timer stopped.
    run_complete_sequence();
    // The next sequence starts with a fresh ON phase and still has three pulses.
    run_complete_sequence();

    // Restarting mid-sequence resets the pulse count and starts a fresh pulse.
    assert(led_indicator_set_blink() == ESP_OK);
    fake_esp_timer_fire(); // first pulse ends
    fake_esp_timer_fire(); // second pulse starts
    assert(current_duty == EXPECTED_BRIGHT_DUTY);
    const size_t restart_history_start = duty_history_count;
    assert(led_indicator_set_blink() == ESP_OK);
    assert(fake_esp_timer_is_active());
    for (size_t i = 0; i < 5; ++i) {
        fake_esp_timer_fire();
    }
    assert(!fake_esp_timer_is_active());
    assert(current_duty == 0);
    assert(count_edges(restart_history_start, EXPECTED_BRIGHT_DUTY, 0) == 3);

    // Explicit off cancels an incomplete sequence and later timer ticks do nothing.
    assert(led_indicator_set_blink() == ESP_OK);
    fake_esp_timer_fire();
    assert(fake_esp_timer_is_active());
    const size_t cancelled_history_count = duty_history_count;
    assert(led_indicator_set_off() == ESP_OK);
    assert(!fake_esp_timer_is_active());
    assert(current_duty == 0);
    fake_esp_timer_fire();
    assert(duty_history_count == cancelled_history_count + 1);
    assert(current_duty == 0);

    assert(led_indicator_deinit() == ESP_OK);
    assert(!fake_esp_timer_is_active());
    puts("LED indicator sequence tests passed");
    return 0;
}
