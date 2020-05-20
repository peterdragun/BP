/**
* @file  esp_main.h
*
* @brief Main part of program and BLE communication
* @author Peter Dragun (xdragu01)
*/

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
#include "esp_sntp.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"

#include "rest_server.h"
#include "alarm.h"
#include "wifi_connect.h"
#include "sensors.h"

// BLE
#define GATTS_NUM_HANDLE            4 /*!< Default number of handle requested*/
#define GATTS_NUM_HANDLE_SETUP      6 /*!< Number of handle requested for setup sevice*/

#define adv_config_flag             (1 << 0) /*!< Advetising config flag*/
#define scan_rsp_config_flag        (1 << 1) /*!< Scan response config flag*/

#define GATTS_PROFILE_NUM           3 /*!< Number of GATTS services*/
#define GATTS_PROFILE_STATUS        0 /*!< Status service index*/
#define GATTS_PROFILE_SENSOR        1 /*!< Sensor service index*/
#define GATTS_PROFILE_SETUP         2 /*!< Setup service index*/
#define GATTC_PROFILE_NUM           1 /*!< Number of client profiles */
#define GATTC_PROFILE               0 /*!< Client profile index*/

#define BLE_SECURITY_SYSTEM         "BLE_SECURITY_SYSTEM" /*!< Tag for logging */
#define REMOTE_SERVICE_UUID         0x1800 /*!< Generic Access UUID */
#define REMOTE_NOTIFY_UUID          0x2A37 /*!< */
#define REMOTE_NOTIFY_CHAR_UUID     0x2A00 /*!< Device name UUID */
#define GATTS_ADV_NAME              "ESP_main_unit" /*!< Advertising name */

#define PROFILE_NUM                 1 /*!< Number of client profiles*/
#define PROFILE_A_APP_ID            0 /*!< Index of client profile*/
#define INVALID_HANDLE              0 /*!< Invalid client handle*/

#define NUMBER_OF_UUIDS             3 /*!< Number of services */

// global variables
extern ledc_channel_config_t ledc_channel[LEDC_CH_NUM];
extern char expected_code[19];
extern state_enum_t *security_state;
extern TaskHandle_t xHandle_alarm;
extern TaskHandle_t xHandle_search;
extern TaskHandle_t xHandle_increment;
extern uint8_t new_address[6];
extern esp_ip4_addr_t s_ip_addr;
extern sensor_t sensors[MAX_NUMBER_OF_SENSORS];
extern uint8_t number_of_sensors;
extern uint8_t unknown_sensor_type;
extern const char sensors_nvs_key[5][3];
extern time_t last_alarm;

/**
 * @brief Instance of server profile
 */
struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;        /**< GATTS callback function*/
    uint16_t gatts_if;              /**< GATTS interface*/
    uint16_t app_id;                /**< Application ID*/
    uint16_t conn_id;               /**< Connection ID*/
    uint16_t service_handle;        /**< Service handle*/
    esp_gatt_srvc_id_t service_id;  /**< Service UUID*/
    uint16_t char_handle;           /**< Characteristic handle*/
    esp_bt_uuid_t char_uuid;        /**< Characteristic UUID*/
    esp_gatt_perm_t perm;           /**< Permissions*/
    esp_gatt_char_prop_t property;  /**< Characteristics property*/
    uint16_t descr_handle;          /**< Descriptor handle*/
    esp_bt_uuid_t descr_uuid;       /**< Descriptor UUID*/
};

/**
 * @brief Instance of client profile
 */
struct gattc_profile_inst {
    esp_gattc_cb_t gattc_cb;        /**< GATTC callback function*/
    uint16_t gattc_if;              /**< GATTC interface*/
    uint16_t app_id;                /**< Application ID*/
    uint16_t conn_id;               /**< Connection ID*/
    uint16_t service_start_handle;  /**< Service start handle*/
    uint16_t service_end_handle;    /**< Service end handle*/
    uint16_t char_handle;           /**< Characteristics handle*/
    esp_bd_addr_t remote_bda;       /**< Remote bluetooth address*/
};

/**
 * @brief Load configuration from NVS, initialize BLE structures, sync time
 */
void app_main();

#endif //ESP_MAIN_H
