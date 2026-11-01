/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <app/server/CommissioningWindowManager.h>
#include <app/server/Server.h>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_matter.h>
#include <esp_matter_ota.h>
#include <nvs_flash.h>
#include <esp_event.h>
#include <esp_wifi.h>
#include <esp_netif.h>
#include <app/clusters/switch-server/CodegenIntegration.h>

// Include the sdkconfig.h file to access Kconfig values
#include "sdkconfig.h"

#include <app_openthread_config.h>
#include "app_reset.h"
#include "utils/common_macros.h"

// Button component direct include
#include "iot_button.h"
#include "button_gpio.h"

// For VID/PID and Onboarding Codes (official example method)
#include <setup_payload/OnboardingCodesUtil.h>

// Attempting to include ESP32 specific config to resolve GetVendorId issue
#include <platform/ConfigurationManager.h> // Base interface
#include <platform/ESP32/ESP32Config.h>    // ESP32 specific implementations

// drivers implemented by this example
#include "drivers/include/led_indicator.h"  // Still using LED helper

#include <esp_matter_event.h>
#include <esp_console.h>

static const char *TAG = "app_main";

static void configure_wifi_selection()
{
    ESP_ERROR_CHECK(esp_netif_init());
    esp_err_t err = esp_event_loop_create_default();
    ESP_ERROR_CHECK(err == ESP_ERR_INVALID_STATE ? ESP_OK : err);
    wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    // USB-powered button: prioritize timely packet delivery over modem sleep.
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));

    wifi_config_t saved{};
    ESP_ERROR_CHECK(esp_wifi_get_config(WIFI_IF_STA, &saved));
    // Preserve credentials/security settings. Search all matching APs by signal,
    // rather than accepting the first match from the legacy fast-scan profile.
    // This also updates already-commissioned devices; CHIP's Kconfig setting
    // alone only affects newly supplied network credentials.
    if (saved.sta.ssid[0] != 0 &&
        (saved.sta.scan_method != WIFI_ALL_CHANNEL_SCAN || saved.sta.sort_method != WIFI_CONNECT_AP_BY_SIGNAL)) {
        saved.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
        saved.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &saved));
    }
    ESP_LOGI(TAG, "Wi-Fi selection: all-channel signal ranking; saved BSSID pin=%s",
             saved.sta.bssid_set ? "yes" : "no");
    // Matter initializes the station netif and starts/manages the connection.
}

// Observe the SDK's connection manager; do not compete with it by reconnecting here.
static void wifi_diagnostic_event(void *, esp_event_base_t, int32_t id, void *data)
{
    if (id == WIFI_EVENT_STA_DISCONNECTED) {
        auto *event = static_cast<wifi_event_sta_disconnected_t *>(data);
        ESP_LOGW(TAG, "Wi-Fi disconnected: reason=%u, RSSI=%d dBm", event->reason, event->rssi);
    } else if (id == WIFI_EVENT_STA_CONNECTED) {
        ESP_LOGI(TAG, "Wi-Fi associated; waiting for IP connectivity");
    }
}

static void wifi_diagnostic_task(void *)
{
    for (;;) {
        wifi_ap_record_t ap{};
        if (esp_wifi_sta_get_ap_info(&ap) == ESP_OK) {
            esp_netif_ip_info_t ip{};
            auto *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
            const bool ipv4_ready = netif && esp_netif_get_ip_info(netif, &ip) == ESP_OK && ip.ip.addr != 0;
            ESP_LOGI(TAG, "Wi-Fi link: RSSI=%d dBm, channel=%u, IPv4=%s", ap.rssi, ap.primary,
                     ipv4_ready ? "ready" : "waiting");
        } else {
            ESP_LOGW(TAG, "Wi-Fi link: not associated (SDK manages reconnection)");
        }
        vTaskDelay(pdMS_TO_TICKS(30000));
    }
}

using namespace esp_matter;
using namespace esp_matter::attribute;
using namespace esp_matter::endpoint;
using namespace chip::app::Clusters;

static void open_commissioning_window_if_necessary()
{
    VerifyOrReturn(chip::Server::GetInstance().GetFabricTable().FabricCount() == 0);

    chip::CommissioningWindowManager & commissionMgr = chip::Server::GetInstance().GetCommissioningWindowManager();
    VerifyOrReturn(commissionMgr.IsCommissioningWindowOpen() == false);

    // After removing last fabric, this example does not remove the Wi-Fi credentials
    // and still has IP connectivity so, only advertising on DNS-SD.
    CHIP_ERROR err = commissionMgr.OpenBasicCommissioningWindow(chip::System::Clock::Seconds16(300),
                                    chip::CommissioningWindowAdvertisement::kDnssdOnly);
    if (err != CHIP_NO_ERROR)
    {
        ESP_LOGE(TAG, "Failed to open commissioning window, err:%" CHIP_ERROR_FORMAT, err.Format());
    }
}

static void app_event_cb(const ChipDeviceEvent *event, intptr_t arg)
{
    switch (event->Type) {
    case chip::DeviceLayer::DeviceEventType::kCommissioningComplete:
        ESP_LOGI(TAG, "Commissioning complete");
        break;

    case chip::DeviceLayer::DeviceEventType::kFailSafeTimerExpired:
        ESP_LOGI(TAG, "Commissioning failed, fail safe timer expired");
        break;

    case chip::DeviceLayer::DeviceEventType::kFabricRemoved:
        ESP_LOGI(TAG, "Fabric removed successfully");
        open_commissioning_window_if_necessary();
        break;

    case chip::DeviceLayer::DeviceEventType::kBLEDeinitialized:
        ESP_LOGI(TAG, "BLE deinitialized and memory reclaimed");
        break;

    default:
        break;
    }
}

// This callback is invoked when clients interact with the Identify Cluster.
// In the callback implementation, an endpoint can identify itself. (e.g., by flashing an LED or light).
static esp_err_t app_identification_cb(identification::callback_type_t type, uint16_t endpoint_id, uint8_t effect_id,
                                       uint8_t effect_variant, void *priv_data)
{
    ESP_LOGI(TAG, "Identification callback: type: %u, effect: %u, variant: %u", type, effect_id, effect_variant);
    return ESP_OK;
}

// This callback is called for every attribute update. The callback implementation shall
// handle the desired attributes and return an appropriate error code. If the attribute
// is not of your interest, please do not return an error code and strictly return ESP_OK.
static esp_err_t app_attribute_update_cb(attribute::callback_type_t type, uint16_t endpoint_id, uint32_t cluster_id,
                                         uint32_t attribute_id, esp_matter_attr_val_t *val, void *priv_data)
{
    if (type == PRE_UPDATE) { // Or POST_UPDATE, depending on when you want to see the value
        if (cluster_id == OccupancySensing::Id && attribute_id == OccupancySensing::Attributes::PIROccupiedToUnoccupiedDelay::Id) {
            ESP_LOGW(TAG, "app_attribute_update_cb: PIROccupiedToUnoccupiedDelay update received for ep %u. New value: %u (type: %d)", 
                     endpoint_id, val->val.u16, val->type);
        } else if (cluster_id == OccupancySensing::Id) {
             ESP_LOGI(TAG, "app_attribute_update_cb: OccupancySensing cluster (ID: %lu) attribute (ID: %lu) update for ep %u. Value type: %d", 
                     cluster_id, attribute_id, endpoint_id, val->type);
        }
    }
    // Since this is just a sensor and we don't expect any writes on our temperature sensor,
    // so, return success.
    return ESP_OK;
}

static esp_err_t factory_reset_button_register()
{
    // Create button configurations
    button_config_t button_config = {
        .long_press_time = 5000,     // 5 seconds for long press
        .short_press_time = 50,      // 50ms for short press
    };
    
    button_gpio_config_t gpio_config = {
        .gpio_num = 9,               // GPIO 9 (BOOT button on ESP32-C3 SuperMini)
        .active_level = 0,           // Active low
        .enable_power_save = false,  // No power save
        .disable_pull = false,       // Use internal pull-up
    };
    
    button_handle_t push_button = NULL;
    
    // Create the GPIO button device
    esp_err_t err = iot_button_new_gpio_device(&button_config, &gpio_config, &push_button);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create button device: %s", esp_err_to_name(err));
        return err;
    }
    
    return app_reset_button_register(push_button);
}

/* -------------------------------------------------------------------------- */
/* Generic Switch button callback                                             */
/* -------------------------------------------------------------------------- */

static uint16_t g_switch_endpoint_id = 0;

// Switch attributes are now owned by a server cluster, not the legacy writer.
// Call with the Matter stack lock held, after server startup.
static void set_switch_position(uint8_t position)
{
    auto *server = Switch::FindClusterOnEndpoint(g_switch_endpoint_id);
    if (server == nullptr || server->SetCurrentPosition(position) != CHIP_NO_ERROR) {
        ESP_LOGE(TAG, "Failed to set Switch CurrentPosition to %u", position);
    }
}

static esp_err_t register_switch_button()
{
    button_config_t cfg = {
        .long_press_time = 1000,
        .short_press_time = 150,
    };

    button_gpio_config_t gpio_cfg = {
        .gpio_num = 3,          // GPIO 3 wired to push-button
        .active_level = 0,      // Active-low (internal pull-up)
        .enable_power_save = false,
        .disable_pull = false,
    };

    button_handle_t hbtn = NULL;
    if (iot_button_new_gpio_device(&cfg, &gpio_cfg, &hbtn) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create generic-switch button device");
        return ESP_FAIL;
    }

    // Only momentary press/release: every physical release is one Home action.
    // No local lamp state, gesture timeout, or multi-click classification.
    esp_err_t err = iot_button_register_cb(hbtn, BUTTON_PRESS_DOWN, nullptr, [](void*, void*) {
        chip::DeviceLayer::StackLock lock;
        set_switch_position(1);
        esp_matter::cluster::switch_cluster::event::send_initial_press(g_switch_endpoint_id, 1);
    }, nullptr);
    if (err != ESP_OK) {
        iot_button_delete(hbtn);
        return err;
    }

    err = iot_button_register_cb(hbtn, BUTTON_PRESS_UP, nullptr, [](void*, void*) {
        ESP_LOGI(TAG, "Generic Switch: Single press");
        {
            chip::DeviceLayer::StackLock lock;
            set_switch_position(0);
            esp_matter::cluster::switch_cluster::event::send_short_release(g_switch_endpoint_id, 1);
        }
        led_indicator_set_blink();
    }, nullptr);
    if (err != ESP_OK) {
        iot_button_delete(hbtn);
    }
    return err;
}

// Simple factory reset trigger - will reset after 10 seconds
static void trigger_factory_reset_timer(void)
{
    ESP_LOGW(TAG, "=== FACTORY RESET TRIGGERED ===");
    ESP_LOGW(TAG, "Device will reset in 10 seconds...");
    ESP_LOGW(TAG, "Unplug power now if you want to cancel!");
    
    vTaskDelay(pdMS_TO_TICKS(10000)); // Wait 10 seconds
    
    ESP_LOGI(TAG, "Starting factory reset NOW");
    esp_matter::factory_reset();
}

// Console command for factory reset
static int factory_reset_cmd(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "confirm") == 0) {
        // Start the reset in a new task
        xTaskCreate([](void*){ trigger_factory_reset_timer(); vTaskDelete(NULL); }, 
                   "factory_reset", 4096, NULL, 5, NULL);
        return 0;
    } else {
        printf("Usage: factory_reset confirm\n");
        printf("WARNING: This will erase all pairing data!\n");
        return 1;
    }
}

static void register_factory_reset_console_cmd()
{
    esp_console_cmd_t cmd = {
        .command = "factory_reset",
        .help = "Perform factory reset (use 'factory_reset confirm')",
        .hint = NULL,
        .func = &factory_reset_cmd,
    };
    esp_console_cmd_register(&cmd);
}

extern "C" void app_main()
{
    /* Initialize the ESP NVS layer */
    // Never erase existing credentials/fabrics as an automatic recovery action.
    ESP_ERROR_CHECK(nvs_flash_init());
    configure_wifi_selection();

    /* Initialize console for factory reset command */
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    esp_console_dev_uart_config_t uart_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    esp_console_new_repl_uart(&uart_config, &repl_config, &repl);
    register_factory_reset_console_cmd();
    esp_console_start_repl(repl);

    /* Initialize push button on the dev-kit to reset the device */
    esp_err_t err = factory_reset_button_register();
    ABORT_APP_ON_FAILURE(ESP_OK == err, ESP_LOGE(TAG, "Failed to initialize reset button, err:%d", err));

    // Initialize LED indicator
    err = led_indicator_init(CONFIG_LED_INDICATOR_GPIO_NUM);
    ABORT_APP_ON_FAILURE(err == ESP_OK, ESP_LOGE(TAG, "Failed to initialize LED indicator, err:%d", err));

    // Boot LED behavior: on 3s, blink 3 times, then off
    led_indicator_set_bright();
    vTaskDelay(pdMS_TO_TICKS(3000));
    led_indicator_set_blink();
    // Wait long enough for 3 blinks to complete (BLINK_COUNT * 2 * period)
    vTaskDelay(pdMS_TO_TICKS(3 * 2 * 150 + 200));
    led_indicator_set_off();

    /* Create a Matter node and add the mandatory Root Node device type on endpoint 0 */
    node::config_t node_config{}; // Explicitly zero-initialize

    // Preserve the existing example-provider identity and commissioning profile.
    // Persisted Wi-Fi/fabric state stays in NVS; do not import factory data or
    // override device identity as part of an application update.

    node_t *node = node::create(&node_config, app_attribute_update_cb, app_identification_cb);
    ABORT_APP_ON_FAILURE(node != nullptr, ESP_LOGE(TAG, "Failed to create Matter node"));

    // ------------------------------------------------------------------
    // Create Generic Switch endpoint
    // ------------------------------------------------------------------

    endpoint::generic_switch::config_t switch_cfg;
    // Matter 1.6 requires explicit, conforming feature selection at creation.
    switch_cfg.switch_cluster.feature_flags =
        cluster::switch_cluster::feature::momentary_switch::get_id() |
        cluster::switch_cluster::feature::momentary_switch_release::get_id();
    endpoint_t *switch_ep = endpoint::generic_switch::create(node, &switch_cfg, ENDPOINT_FLAG_NONE, NULL);
    ABORT_APP_ON_FAILURE(switch_ep != nullptr, ESP_LOGE(TAG, "Failed to create generic_switch endpoint"));

    g_switch_endpoint_id = endpoint::get_id(switch_ep);

#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
    /* Set OpenThread platform config */
    esp_openthread_platform_config_t config = {
        .radio_config = ESP_OPENTHREAD_DEFAULT_RADIO_CONFIG(),
        .host_config = ESP_OPENTHREAD_DEFAULT_HOST_CONFIG(),
        .port_config = ESP_OPENTHREAD_DEFAULT_PORT_CONFIG(),
    };
    set_openthread_platform_config(&config);
#endif

    /* Matter start */
    err = esp_matter::start(app_event_cb);
    ABORT_APP_ON_FAILURE(err == ESP_OK, ESP_LOGE(TAG, "Failed to start Matter, err:%d", err));

    // Register input only after the native Switch server exists.
    err = register_switch_button();
    ABORT_APP_ON_FAILURE(err == ESP_OK, ESP_LOGE(TAG, "Failed to register switch button"));
    ESP_LOGI(TAG, "Momentary button ready on endpoint %u; no simulated lamp endpoint", g_switch_endpoint_id);

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_diagnostic_event, nullptr));
    ABORT_APP_ON_FAILURE(xTaskCreate(wifi_diagnostic_task, "wifi_diagnostics", 3072, nullptr, 1, nullptr) == pdPASS,
                         ESP_LOGE(TAG, "Failed to create Wi-Fi diagnostics task"));

    // PrintOnboardingCodes will log the necessary VID/PID and commissioning info
    chip::DeviceLayer::StackLock lock; // RAII lock for Matter stack
    PrintOnboardingCodes(chip::RendezvousInformationFlags(chip::RendezvousInformationFlag::kBLE).Set(chip::RendezvousInformationFlag::kOnNetwork));
}
