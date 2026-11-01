#pragma once

typedef int esp_err_t;

#define ESP_OK 0
#define ESP_FAIL (-1)
#define ESP_ERR_INVALID_STATE 0x103

#include <stdlib.h>
#define ESP_ERROR_CHECK(expr) do { if ((expr) != ESP_OK) abort(); } while (0)
