#ifndef ESP_MAIN_H
#define ESP_MAIN_H

#include "cJSON.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatt_common_api.h"
#include "esp_gattc_api.h"
#include "esp_gatts_api.h"
#include "esp_vfs_fat.h"
#include "lwip/apps/netbiosns.h"
#include "mdns.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"

#include "rest_server.h"
#include "alarm.h"

// BLE
#define GATTS_NUM_HANDLE_TEST_A     4

#define GATTS_DEMO_CHAR_VAL_LEN_MAX 0x40
#define PREPARE_BUF_MAX_SIZE        1024

#define adv_config_flag             (1 << 0)
#define scan_rsp_config_flag        (1 << 1)

#define GATTS_PROFILE_NUM           2
#define GATTS_PROFILE_A_APP_ID      0
#define GATTS_PROFILE_B_APP_ID      1
#define GATTC_PROFILE_NUM           1
#define GATTC_PROFILE_C_APP_ID      0

#define BLE_SECURITY_SYSTEM         "BLE_SECURITY_SYSTEM"
#define REMOTE_SERVICE_UUID         ESP_GATT_UUID_HEART_RATE_SVC
#define REMOTE_NOTIFY_UUID          0x2A37
#define REMOTE_NOTIFY_CHAR_UUID     0xFF01
#define NOTIFY_ENABLE               0x0001
#define INDICATE_ENABLE             0x0002
#define NOTIFY_INDICATE_DISABLE     0x0000
#define GATTS_ADV_NAME              "ESP_main_unit"

#define PROFILE_NUM                 1
#define PROFILE_A_APP_ID            0
#define INVALID_HANDLE              0

#define MDNS_INSTANCE "esp_home"

extern ledc_channel_config_t ledc_channel[LEDC_CH_NUM];
extern char expected_code[19];
extern state_enum_t *security_state;
extern TaskHandle_t xHandle_alarm;
extern TaskHandle_t xHandle_search;

typedef struct {
    uint8_t                 *prepare_buf;
    int                      prepare_len;
} prepare_type_env_t;

struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    uint16_t char_handle;
    esp_bt_uuid_t char_uuid;
    esp_gatt_perm_t perm;
    esp_gatt_char_prop_t property;
    uint16_t descr_handle;
    esp_bt_uuid_t descr_uuid;
};

struct gattc_profile_inst {
    esp_gattc_cb_t gattc_cb;
    uint16_t gattc_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_start_handle;
    uint16_t service_end_handle;
    uint16_t notify_char_handle;
    esp_bd_addr_t remote_bda;
};

void app_main();

#endif
