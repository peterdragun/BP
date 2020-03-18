// #include <stdbool.h>
// #include <stdint.h>
// #include <stdio.h>
#include <string.h>

#include "cJSON.h"
// #include "driver/gpio.h"
#include "esp_bt.h"
// #include "esp_bt_defs.h"
#include "esp_bt_main.h"
// #include "esp_event.h"
#include "esp_gap_ble_api.h"
#include "esp_gatt_common_api.h"
// #include "esp_gatt_defs.h"
#include "esp_gattc_api.h"
#include "esp_gatts_api.h"
// #include "esp_log.h"
// #include "esp_netif.h"
// #include "esp_spiffs.h"
// #include "esp_system.h"
#include "esp_vfs_fat.h"
// #include "freertos/FreeRTOS.h"
// #include "freertos/event_groups.h"
#include "lwip/apps/netbiosns.h"
#include "mdns.h"
// #include "nvs.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"
// #include "sdkconfig.h"
// #include "sdmmc_cmd.h"

#define GATTS_SERVICE_UUID_TEST_A   0x00FF
#define GATTS_CHAR_UUID_TEST_A      0xFF01
#define GATTS_NUM_HANDLE_TEST_A     4

#define GATTS_SERVICE_UUID_TEST_B   0x00EE
#define GATTS_CHAR_UUID_TEST_B      0xEE01
#define GATTS_NUM_HANDLE_TEST_B     4

#define GATTS_DEMO_CHAR_VAL_LEN_MAX 0x40
#define PREPARE_BUF_MAX_SIZE        1024

#define adv_config_flag             (1 << 0)
#define scan_rsp_config_flag        (1 << 1)

#define GATTS_PROFILE_NUM           2
#define GATTS_PROFILE_A_APP_ID      0
#define GATTS_PROFILE_B_APP_ID      1
#define GATTC_PROFILE_NUM           1
#define GATTC_PROFILE_C_APP_ID      0

#define GATTC_TAG             "SEC_GATTC_DEMO"
#define COEX_TAG              "GATTC_GATTS_COEX"
#define REMOTE_SERVICE_UUID   ESP_GATT_UUID_HEART_RATE_SVC
#define REMOTE_NOTIFY_UUID    0x2A37
#define REMOTE_NOTIFY_CHAR_UUID     0xFF01
#define NOTIFY_ENABLE               0x0001
#define INDICATE_ENABLE             0x0002
#define NOTIFY_INDICATE_DISABLE     0x0000
#define GATTS_ADV_NAME              "ESP_main_unit"

typedef struct {
    uint8_t                 *prepare_buf;
    int                      prepare_len;
} prepare_type_env_t;

static esp_gattc_char_elem_t *char_elem_result   = NULL;
static esp_gattc_descr_elem_t *descr_elem_result = NULL;

///Declare static functions
static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
static void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);
static void gattc_profile_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);

static void gatts_profile_a_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
static void gatts_profile_b_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
static void example_write_event_env(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param);
static void example_exec_write_event_env(prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param);
static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

static esp_bt_uuid_t remote_filter_service_uuid = {
    .len = ESP_UUID_LEN_16,
    .uuid = {.uuid16 = REMOTE_SERVICE_UUID,},
};

static bool connect = false;
static bool get_service = false;
static const char remote_device_name[] = "ESP_BLE_SECURITY";

static esp_ble_scan_params_t ble_scan_params = {
    .scan_type              = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type          = BLE_ADDR_TYPE_RANDOM,
    .scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval          = 0x50,
    .scan_window            = 0x30,
    .scan_duplicate         = BLE_SCAN_DUPLICATE_ENABLE
};

extern cJSON *json_resp;

#define PROFILE_NUM 1
#define PROFILE_A_APP_ID 0
#define INVALID_HANDLE   0

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

/* One gatt-based profile one app_id and one gattc_if, this array will store the gattc_if returned by ESP_GATTS_REG_EVT */
static struct gattc_profile_inst gl_profile_tab[PROFILE_NUM] = {
    [PROFILE_A_APP_ID] = {
        .gattc_cb = gattc_profile_event_handler,
        .gattc_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },
};

static esp_gatt_char_prop_t a_property = 0;
static esp_gatt_char_prop_t b_property = 0;
static prepare_type_env_t a_prepare_write_env;
static prepare_type_env_t b_prepare_write_env;
static uint8_t adv_config_done         = 0;
static uint8_t char1_str[]             = {0x11, 0x22, 0x33};

esp_attr_value_t gatts_demo_char1_val = {
    .attr_max_len = GATTS_DEMO_CHAR_VAL_LEN_MAX,
    .attr_len     = sizeof(char1_str),
    .attr_value   = char1_str,
};

static uint8_t service_uuid128[32] = {
    /* LSB <--------------------------------------------------------------------------------> MSB */
    //first uuid, 16bit, [12],[13] is the value
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xAB, 0xCD, 0x00, 0x00,
    //second uuid, 32bit, [12], [13], [14], [15] is the value
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xAB, 0xCD, 0xAB, 0xCD,
};

static esp_ble_adv_data_t adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = true,
    .min_interval = 0x20,
    .max_interval = 0x40,
    .appearance = 0x00,
    .manufacturer_len = 0, 
    .p_manufacturer_data =  NULL, 
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = 32,
    .p_service_uuid = service_uuid128,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

// scan response data
static esp_ble_adv_data_t scan_rsp_data = {
    .set_scan_rsp = true,
    .include_name = true,
    .include_txpower = true,
    .min_interval = 0x0006,
    .max_interval = 0x0010,
    .appearance = 0x00,
    .manufacturer_len = 0, 
    .p_manufacturer_data =  NULL, 
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = 0,
    .p_service_uuid = NULL,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

static esp_ble_adv_params_t adv_params = {
    .adv_int_min        = 0x20,
    .adv_int_max        = 0x40,
    .adv_type           = ADV_TYPE_IND,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

/* One gatt-based profile one app_id and one gatts_if, this array will store the gatts_if returned by ESP_GATTS_REG_EVT */
static struct gatts_profile_inst gatts_profile_tab[GATTS_PROFILE_NUM] = {
    [GATTS_PROFILE_A_APP_ID] = {
        .gatts_cb = gatts_profile_a_event_handler,
        .gatts_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },
    [GATTS_PROFILE_B_APP_ID] = {
        .gatts_cb = gatts_profile_b_event_handler,                   /* This demo does not implement, similar as profile A */
        .gatts_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },
};

// static esp_bt_uuid_t remote_filter_char_uuid = {
//     .len = ESP_UUID_LEN_16,
//     .uuid = {.uuid16 = REMOTE_NOTIFY_CHAR_UUID,},
// };

// static esp_bt_uuid_t notify_descr_uuid = {
//     .len = ESP_UUID_LEN_16,
//     .uuid = {.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG,},
// };

static const char *esp_key_type_to_str(esp_ble_key_type_t key_type)
{
   const char *key_str = NULL;
   switch(key_type) {
    case ESP_LE_KEY_NONE:
        key_str = "ESP_LE_KEY_NONE";
        break;
    case ESP_LE_KEY_PENC:
        key_str = "ESP_LE_KEY_PENC";
        break;
    case ESP_LE_KEY_PID:
        key_str = "ESP_LE_KEY_PID";
        break;
    case ESP_LE_KEY_PCSRK:
        key_str = "ESP_LE_KEY_PCSRK";
        break;
    case ESP_LE_KEY_PLK:
        key_str = "ESP_LE_KEY_PLK";
        break;
    case ESP_LE_KEY_LLK:
        key_str = "ESP_LE_KEY_LLK";
        break;
    case ESP_LE_KEY_LENC:
        key_str = "ESP_LE_KEY_LENC";
        break;
    case ESP_LE_KEY_LID:
        key_str = "ESP_LE_KEY_LID";
        break;
    case ESP_LE_KEY_LCSRK:
        key_str = "ESP_LE_KEY_LCSRK";
        break;
    default:
        key_str = "INVALID BLE KEY TYPE";
        break;

    }
     return key_str;
}

static char *esp_auth_req_to_str(esp_ble_auth_req_t auth_req)
{
   char *auth_str = NULL;
   switch(auth_req) {
    case ESP_LE_AUTH_NO_BOND:
        auth_str = "ESP_LE_AUTH_NO_BOND";
        break;
    case ESP_LE_AUTH_BOND:
        auth_str = "ESP_LE_AUTH_BOND";
        break;
    case ESP_LE_AUTH_REQ_MITM:
        auth_str = "ESP_LE_AUTH_REQ_MITM";
        break;
    case ESP_LE_AUTH_REQ_BOND_MITM:
        auth_str = "ESP_LE_AUTH_REQ_BOND_MITM";
        break;
    case ESP_LE_AUTH_REQ_SC_ONLY:
        auth_str = "ESP_LE_AUTH_REQ_SC_ONLY";
        break;
    case ESP_LE_AUTH_REQ_SC_BOND:
        auth_str = "ESP_LE_AUTH_REQ_SC_BOND";
        break;
    case ESP_LE_AUTH_REQ_SC_MITM:
        auth_str = "ESP_LE_AUTH_REQ_SC_MITM";
        break;
    case ESP_LE_AUTH_REQ_SC_MITM_BOND:
        auth_str = "ESP_LE_AUTH_REQ_SC_MITM_BOND";
        break;
    default:
        auth_str = "INVALID BLE AUTH REQ";
        break;
   }

   return auth_str;
}

#define MDNS_INSTANCE "esp home web server"

esp_err_t start_rest_server(const char *base_path);

static void initialise_mdns(void)
{
    mdns_init();
    mdns_hostname_set(CONFIG_EXAMPLE_MDNS_HOST_NAME);
    mdns_instance_name_set(MDNS_INSTANCE);

    mdns_txt_item_t serviceTxtData[] = {
        {"board", "esp32"},
        {"path", "/"}
    };

    ESP_ERROR_CHECK(mdns_service_add("ESP32-WebServer", "_http", "_tcp", 80, serviceTxtData,
                                     sizeof(serviceTxtData) / sizeof(serviceTxtData[0])));
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    initialise_mdns();
    netbiosns_init();
    netbiosns_set_name(CONFIG_EXAMPLE_MDNS_HOST_NAME);

    // init rest server
    ESP_ERROR_CHECK(example_connect());
    ESP_ERROR_CHECK(start_rest_server(CONFIG_EXAMPLE_WEB_MOUNT_POINT));

    // init BLE
    esp_err_t ret;
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(GATTC_TAG, "%s initialize controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(GATTC_TAG, "%s enable controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(GATTC_TAG, "%s init bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(GATTC_TAG, "%s enable bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    // register the  callback function to the gap module
    ret = esp_ble_gap_register_callback(esp_gap_cb);
    if (ret){
        ESP_LOGE(GATTC_TAG, "%s gap register error, error code = %x\n", __func__, ret);
        return;
    }

    // gatts register
    ret = esp_ble_gatts_register_callback(gatts_event_handler);
    if (ret) {
        ESP_LOGE(COEX_TAG, "gatts register error, error code = %x", ret);
        return;
    }

    ret = esp_ble_gatts_app_register(GATTS_PROFILE_A_APP_ID);
    if (ret) {
        ESP_LOGE(COEX_TAG, "gatts app register error, error code = %x", ret);
        return;
    }

    ret = esp_ble_gatts_app_register(GATTS_PROFILE_B_APP_ID);
    if (ret) {
        ESP_LOGE(COEX_TAG, "gatts app register error, error code = %x", ret);
        return;
    }

    // gattc register
    //register the callback function to the gattc module
    ret = esp_ble_gattc_register_callback(esp_gattc_cb);
    if(ret){
        ESP_LOGE(GATTC_TAG, "%s gattc register error, error code = %x\n", __func__, ret);
        return;
    }

    ret = esp_ble_gattc_app_register(PROFILE_A_APP_ID);
    if (ret){
        ESP_LOGE(GATTC_TAG, "%s gattc app register error, error code = %x\n", __func__, ret);
    }

    ret = esp_ble_gatt_set_local_mtu(200);
    if (ret){
        ESP_LOGE(GATTC_TAG, "set local  MTU failed, error code = %x", ret);
    }

    /* set the security iocap & auth_req & key size & init key response key parameters to the stack*/
    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_MITM_BOND;     //bonding with peer device after authentication
    esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;           //set the IO capability to No output No input
    uint8_t key_size = 16;      //the key size should be 7~16 bytes
    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t oob_support = ESP_BLE_OOB_DISABLE;
    esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_OOB_SUPPORT, &oob_support, sizeof(uint8_t));
    /* If your BLE device act as a Slave, the init_key means you hope which types of key of the master should distribute to you,
    and the response key means which key you can distribute to the Master;
    If your BLE device act as a master, the response key means you hope which types of key of the slave should distribute to you,
    and the init key means which key you can distribute to the slave. */
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));
    ESP_LOGI("app-main", "inicialization end");
}

static void gattc_profile_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
    esp_ble_gattc_cb_param_t *p_data = (esp_ble_gattc_cb_param_t *)param;

    switch (event) {
    case ESP_GATTC_REG_EVT:
        ESP_LOGI(GATTC_TAG, "REG_EVT");
        esp_ble_gap_config_local_privacy(true);
        break;
    case ESP_GATTC_CONNECT_EVT: {
        ESP_LOGI(COEX_TAG, "ESP_GATTC_CONNECT_EVT conn_id %d, if %d\n", p_data->connect.conn_id, gattc_if);
        gl_profile_tab[GATTC_PROFILE_C_APP_ID].conn_id = p_data->connect.conn_id;
        memcpy(gl_profile_tab[GATTC_PROFILE_C_APP_ID].remote_bda, p_data->connect.remote_bda, sizeof(esp_bd_addr_t));
        ESP_LOGI(COEX_TAG, "REMOTE BDA:");
        esp_log_buffer_hex(COEX_TAG, gl_profile_tab[GATTC_PROFILE_C_APP_ID].remote_bda, sizeof(esp_bd_addr_t));
        esp_err_t mtu_ret = esp_ble_gattc_send_mtu_req (gattc_if, p_data->connect.conn_id);
        if (mtu_ret) {
            ESP_LOGE(COEX_TAG, "config MTU error, error code = %x\n", mtu_ret);
        }
        break;
    }
    case ESP_GATTC_OPEN_EVT:
        if (param->open.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "open failed, error status = %x", p_data->open.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "open success");
        gl_profile_tab[PROFILE_A_APP_ID].conn_id = p_data->open.conn_id;
        memcpy(gl_profile_tab[PROFILE_A_APP_ID].remote_bda, p_data->open.remote_bda, sizeof(esp_bd_addr_t));
        ESP_LOGI(GATTC_TAG, "REMOTE BDA:");
        esp_log_buffer_hex(GATTC_TAG, gl_profile_tab[PROFILE_A_APP_ID].remote_bda, sizeof(esp_bd_addr_t));
        esp_err_t mtu_ret = esp_ble_gattc_send_mtu_req (gattc_if, p_data->open.conn_id);
        if (mtu_ret){
            ESP_LOGE(GATTC_TAG, "config MTU error, error code = %x", mtu_ret);
        }
        break;
    case ESP_GATTC_DIS_SRVC_CMPL_EVT:
        if (param->dis_srvc_cmpl.status != ESP_GATT_OK) {
            ESP_LOGE(COEX_TAG, "discover service failed, status %d\n", param->dis_srvc_cmpl.status);
            break;
        }
        ESP_LOGI(COEX_TAG, "discover service complete conn_id %d\n", param->dis_srvc_cmpl.conn_id);
        esp_ble_gattc_search_service(gattc_if, param->cfg_mtu.conn_id, &remote_filter_service_uuid);
        break;
    case ESP_GATTC_CFG_MTU_EVT:
        if (param->cfg_mtu.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG,"config mtu failed, error status = %x", param->cfg_mtu.status);
        }
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_CFG_MTU_EVT, Status %d, MTU %d, conn_id %d", param->cfg_mtu.status, param->cfg_mtu.mtu, param->cfg_mtu.conn_id);
        esp_ble_gattc_search_service(gattc_if, param->cfg_mtu.conn_id, &remote_filter_service_uuid);
        break;
    case ESP_GATTC_SEARCH_RES_EVT: {
        ESP_LOGI(GATTC_TAG, "SEARCH RES: conn_id = %x is primary service %d", p_data->search_res.conn_id, p_data->search_res.is_primary);
        ESP_LOGI(GATTC_TAG, "start handle %d end handle %d current handle value %d", p_data->search_res.start_handle, p_data->search_res.end_handle, p_data->search_res.srvc_id.inst_id);
        if (p_data->search_res.srvc_id.uuid.len == ESP_UUID_LEN_16 && p_data->search_res.srvc_id.uuid.uuid.uuid16 == REMOTE_SERVICE_UUID) {
            ESP_LOGI(GATTC_TAG, "UUID16: %x", p_data->search_res.srvc_id.uuid.uuid.uuid16);
            get_service = true;
            gl_profile_tab[PROFILE_A_APP_ID].service_start_handle = p_data->search_res.start_handle;
            gl_profile_tab[PROFILE_A_APP_ID].service_end_handle = p_data->search_res.end_handle;
        }
        break;
    }
    case ESP_GATTC_SEARCH_CMPL_EVT:
        if (p_data->search_cmpl.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "search service failed, error status = %x", p_data->search_cmpl.status);
            break;
        }
        if(p_data->search_cmpl.searched_service_source == ESP_GATT_SERVICE_FROM_REMOTE_DEVICE) {
            ESP_LOGI(GATTC_TAG, "Get service information from remote device");
        } else if (p_data->search_cmpl.searched_service_source == ESP_GATT_SERVICE_FROM_NVS_FLASH) {
            ESP_LOGI(GATTC_TAG, "Get service information from flash");
        } else {
            ESP_LOGI(GATTC_TAG, "unknown service source");
        }
        if (get_service){
            uint16_t count  = 0;
            uint16_t offset = 0;
            esp_gatt_status_t ret_status = esp_ble_gattc_get_attr_count(gattc_if,
                                                                        gl_profile_tab[PROFILE_A_APP_ID].conn_id,
                                                                        ESP_GATT_DB_CHARACTERISTIC,
                                                                        gl_profile_tab[PROFILE_A_APP_ID].service_start_handle,
                                                                        gl_profile_tab[PROFILE_A_APP_ID].service_end_handle,
                                                                        INVALID_HANDLE,
                                                                        &count);
            if (ret_status != ESP_GATT_OK){
                ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_attr_count error, %d", __LINE__);
            }
            if (count > 0){
                char_elem_result = (esp_gattc_char_elem_t *)malloc(sizeof(esp_gattc_char_elem_t) * count);
                if (!char_elem_result){
                    ESP_LOGE(GATTC_TAG, "gattc no mem");
                }else{
                    ret_status = esp_ble_gattc_get_all_char(gattc_if,
                                                            gl_profile_tab[PROFILE_A_APP_ID].conn_id,
                                                            gl_profile_tab[PROFILE_A_APP_ID].service_start_handle,
                                                            gl_profile_tab[PROFILE_A_APP_ID].service_end_handle,
                                                            char_elem_result,
                                                            &count,
                                                            offset);
                    if (ret_status != ESP_GATT_OK){
                        ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_all_char error, %d", __LINE__);
                    }
                    if (count > 0){

                        for (int i = 0; i < count; ++i)
                        {
                            if (char_elem_result[i].uuid.len == ESP_UUID_LEN_16 && char_elem_result[i].uuid.uuid.uuid16 == REMOTE_NOTIFY_UUID && (char_elem_result[i].properties & ESP_GATT_CHAR_PROP_BIT_NOTIFY))
                            {
                                gl_profile_tab[PROFILE_A_APP_ID].notify_char_handle = char_elem_result[i].char_handle;
                                esp_ble_gattc_register_for_notify (gattc_if,
                                                                   gl_profile_tab[PROFILE_A_APP_ID].remote_bda,
                                                                   char_elem_result[i].char_handle);
                                break;
                            }
                        }
                    }
                }
                free(char_elem_result);
            }
        }

        break;
    case ESP_GATTC_REG_FOR_NOTIFY_EVT: {
        if (p_data->reg_for_notify.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "reg for notify failed, error status = %x", p_data->reg_for_notify.status);
            break;
        }

            uint16_t count = 0;
            uint16_t offset = 0;
            uint16_t notify_en = 1;
            esp_gatt_status_t ret_status = esp_ble_gattc_get_attr_count(gattc_if,
                                                                        gl_profile_tab[PROFILE_A_APP_ID].conn_id,
                                                                        ESP_GATT_DB_DESCRIPTOR,
                                                                        gl_profile_tab[PROFILE_A_APP_ID].service_start_handle,
                                                                        gl_profile_tab[PROFILE_A_APP_ID].service_end_handle,
                                                                        p_data->reg_for_notify.handle,
                                                                        &count);
            if (ret_status != ESP_GATT_OK){
                ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_attr_count error, %d", __LINE__);
            }
            if (count > 0){
                descr_elem_result = malloc(sizeof(esp_gattc_descr_elem_t) * count);
                if (!descr_elem_result){
                    ESP_LOGE(GATTC_TAG, "malloc error, gattc no mem");
                }else{
                    ret_status = esp_ble_gattc_get_all_descr(gattc_if,
                                                             gl_profile_tab[PROFILE_A_APP_ID].conn_id,
                                                             p_data->reg_for_notify.handle,
                                                             descr_elem_result,
                                                             &count,
                                                             offset);
                if (ret_status != ESP_GATT_OK){
                    ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_all_descr error, %d", __LINE__);
                }

                    for (int i = 0; i < count; ++i)
                    {
                        if (descr_elem_result[i].uuid.len == ESP_UUID_LEN_16 && descr_elem_result[i].uuid.uuid.uuid16 == ESP_GATT_UUID_CHAR_CLIENT_CONFIG)
                        {
                            esp_ble_gattc_write_char_descr (gattc_if,
                                                            gl_profile_tab[PROFILE_A_APP_ID].conn_id,
                                                            descr_elem_result[i].handle,
                                                            sizeof(notify_en),
                                                            (uint8_t *)&notify_en,
                                                            ESP_GATT_WRITE_TYPE_RSP,
                                                            ESP_GATT_AUTH_REQ_NONE);

                            break;
                        }
                    }
                }
                free(descr_elem_result);
            }

        break;
    }
    case ESP_GATTC_NOTIFY_EVT:
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_NOTIFY_EVT, receive notify value:");
        esp_log_buffer_hex(GATTC_TAG, p_data->notify.value, p_data->notify.value_len);
        break;
    case ESP_GATTC_WRITE_DESCR_EVT:
        if (p_data->write.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "write descr failed, error status = %x", p_data->write.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "write descr success");
        break;
    case ESP_GATTC_SRVC_CHG_EVT: {
        esp_bd_addr_t bda;
        memcpy(bda, p_data->srvc_chg.remote_bda, sizeof(esp_bd_addr_t));
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_SRVC_CHG_EVT, bd_addr:");
        esp_log_buffer_hex(GATTC_TAG, bda, sizeof(esp_bd_addr_t));
        break;
    }
    case ESP_GATTC_WRITE_CHAR_EVT:
        if (p_data->write.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "write char failed, error status = %x", p_data->write.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "Write char success ");
        break;
    case ESP_GATTC_DISCONNECT_EVT:
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_DISCONNECT_EVT, reason = 0x%x", p_data->disconnect.reason);
        connect = false;
        get_service = false;
        break;
    default:
        break;
    }
}

static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    uint8_t *adv_name = NULL;
    uint8_t adv_name_len = 0;
    switch (event) {
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        adv_config_done &= (~adv_config_flag);
        if (adv_config_done == 0) {
            esp_ble_gap_start_advertising(&adv_params);
        }
        break;
    case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
        adv_config_done &= (~scan_rsp_config_flag);
        if (adv_config_done == 0) {
            esp_ble_gap_start_advertising(&adv_params);
        }
        break;

    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        //advertising start complete event to indicate advertising start successfully or failed
        if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(COEX_TAG, "Advertising start failed\n");
        }
        ESP_LOGI(COEX_TAG, "Advertising start successfully\n");
        break;
    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(COEX_TAG, "Advertising stop failed\n");
        } else {
            ESP_LOGI(COEX_TAG, "Stop adv successfully\n");
        }
        break;
    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
         ESP_LOGI(COEX_TAG, "update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d\n",
                  param->update_conn_params.status,
                  param->update_conn_params.min_int,
                  param->update_conn_params.max_int,
                  param->update_conn_params.conn_int,
                  param->update_conn_params.latency,
                  param->update_conn_params.timeout);
        break;
    case ESP_GAP_BLE_SET_LOCAL_PRIVACY_COMPLETE_EVT:
        if (param->local_privacy_cmpl.status != ESP_BT_STATUS_SUCCESS){
            ESP_LOGE(GATTC_TAG, "config local privacy failed, error code =%x", param->local_privacy_cmpl.status);
            break;
        }
        esp_err_t scan_ret = esp_ble_gap_set_scan_params(&ble_scan_params);
        if (scan_ret){
            ESP_LOGE(GATTC_TAG, "set scan params error, error code = %x", scan_ret);
        }
        break;
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
        //the unit of the duration is second
        // uint32_t duration = 30;
        // esp_ble_gap_start_scanning(duration);
        break;
    }
    case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
        //scan start complete event to indicate scan start successfully or failed
        if (param->scan_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(GATTC_TAG, "scan start failed, error status = %x", param->scan_start_cmpl.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "Scan start success");
        break;
    case ESP_GAP_BLE_PASSKEY_REQ_EVT:                           /* passkey request event */
        /* Call the following function to input the passkey which is displayed on the remote device */
        //esp_ble_passkey_reply(gl_profile_tab[PROFILE_A_APP_ID].remote_bda, true, 0x00);
        ESP_LOGI(GATTC_TAG, "ESP_GAP_BLE_PASSKEY_REQ_EVT");
        break;
    case ESP_GAP_BLE_OOB_REQ_EVT: {
        ESP_LOGI(GATTC_TAG, "ESP_GAP_BLE_OOB_REQ_EVT");
        uint8_t tk[16] = {1}; //If you paired with OOB, both devices need to use the same tk
        esp_ble_oob_req_reply(param->ble_security.ble_req.bd_addr, tk, sizeof(tk));
        break;
    }
    case ESP_GAP_BLE_LOCAL_IR_EVT:                               /* BLE local IR event */
        ESP_LOGI(GATTC_TAG, "ESP_GAP_BLE_LOCAL_IR_EVT");
        break;
    case ESP_GAP_BLE_LOCAL_ER_EVT:                               /* BLE local ER event */
        ESP_LOGI(GATTC_TAG, "ESP_GAP_BLE_LOCAL_ER_EVT");
        break;
    case ESP_GAP_BLE_SEC_REQ_EVT:
        /* send the positive(true) security response to the peer device to accept the security request.
        If not accept the security request, should send the security response with negative(false) accept value*/
        esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
        break;
    case ESP_GAP_BLE_NC_REQ_EVT:
        /* The app will receive this evt when the IO has DisplayYesNO capability and the peer device IO also has DisplayYesNo capability.
        show the passkey number to the user to confirm it with the number displayed by peer device. */
        esp_ble_confirm_reply(param->ble_security.ble_req.bd_addr, true);
        ESP_LOGI(GATTC_TAG, "ESP_GAP_BLE_NC_REQ_EVT, the passkey Notify number:%d", param->ble_security.key_notif.passkey);
        break;
    case ESP_GAP_BLE_PASSKEY_NOTIF_EVT:  ///the app will receive this evt when the IO  has Output capability and the peer device IO has Input capability.
        ///show the passkey number to the user to input it in the peer device.
        ESP_LOGI(GATTC_TAG, "The passkey Notify number:%06d", param->ble_security.key_notif.passkey);
        break;
    case ESP_GAP_BLE_KEY_EVT:
        //shows the ble key info share with peer device to the user.
        ESP_LOGI(GATTC_TAG, "key type = %s", esp_key_type_to_str(param->ble_security.ble_key.key_type));
        break;
    case ESP_GAP_BLE_AUTH_CMPL_EVT: {
        esp_bd_addr_t bd_addr;
        memcpy(bd_addr, param->ble_security.auth_cmpl.bd_addr, sizeof(esp_bd_addr_t));
        ESP_LOGI(GATTC_TAG, "remote BD_ADDR: %08x%04x",\
                (bd_addr[0] << 24) + (bd_addr[1] << 16) + (bd_addr[2] << 8) + bd_addr[3],
                (bd_addr[4] << 8) + bd_addr[5]);
        ESP_LOGI(GATTC_TAG, "address type = %d", param->ble_security.auth_cmpl.addr_type);
        ESP_LOGI(GATTC_TAG, "pair status = %s",param->ble_security.auth_cmpl.success ? "success" : "fail");
        if (!param->ble_security.auth_cmpl.success) {
            ESP_LOGI(GATTC_TAG, "fail reason = 0x%x",param->ble_security.auth_cmpl.fail_reason);
        } else {
            ESP_LOGI(GATTC_TAG, "auth mode = %s",esp_auth_req_to_str(param->ble_security.auth_cmpl.auth_mode));    
        }
        break;
    }
    case ESP_GAP_BLE_SCAN_RESULT_EVT: {
        esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;
        cJSON *json_obj = cJSON_CreateObject();
        switch (scan_result->scan_rst.search_evt) {
        case ESP_GAP_SEARCH_INQ_RES_EVT:
            esp_log_buffer_hex(GATTC_TAG, scan_result->scan_rst.bda, 6);
            ESP_LOGI(GATTC_TAG, "Searched Adv Data Len %d, Scan Response Len %d", scan_result->scan_rst.adv_data_len, scan_result->scan_rst.scan_rsp_len);
            adv_name = esp_ble_resolve_adv_data(scan_result->scan_rst.ble_adv,
                                                ESP_BLE_AD_TYPE_NAME_CMPL, &adv_name_len);
            char address[20];
            uint8_t *a = scan_result->scan_rst.bda;
            sprintf(address, "%02x %02x %02x %02x %02x %02x", a[0], a[1], a[2], a[3], a[4], a[5]);
            cJSON_AddStringToObject(json_obj, "address", (const char*)address);
            cJSON_AddStringToObject(json_obj, "name", (const char*)adv_name);
            cJSON_AddItemToArray(json_resp, json_obj);
            ESP_LOGI(GATTC_TAG, "Searched Device Name Len %d", adv_name_len);
            esp_log_buffer_char(GATTC_TAG, adv_name, adv_name_len);
            ESP_LOGI(GATTC_TAG, "\n");
            if (adv_name != NULL) {
                if (strlen(remote_device_name) == adv_name_len && strncmp((char *)adv_name, remote_device_name, adv_name_len) == 0) {
                    ESP_LOGI(GATTC_TAG, "searched device %s\n", remote_device_name);
                    if (connect == false) {
                        connect = true;
                        ESP_LOGI(GATTC_TAG, "connect to the remote device.");
                        esp_ble_gap_stop_scanning();
                        esp_ble_gattc_open(gl_profile_tab[PROFILE_A_APP_ID].gattc_if, scan_result->scan_rst.bda, scan_result->scan_rst.ble_addr_type, true);
                    }
                }
            }
            break;
        case ESP_GAP_SEARCH_INQ_CMPL_EVT:
            break;
        default:
            break;
        }
        break;
    }

    case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
        if (param->scan_stop_cmpl.status != ESP_BT_STATUS_SUCCESS){
            ESP_LOGE(GATTC_TAG, "Scan stop failed, error status = %x", param->scan_stop_cmpl.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "Stop scan successfully");
        break;

    default:
        break;
    }
}

static void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
    ESP_LOGI(GATTC_TAG, "EVT %d, gattc if %d", event, gattc_if);

    /* If event is register event, store the gattc_if for each profile */
    if (event == ESP_GATTC_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            gl_profile_tab[param->reg.app_id].gattc_if = gattc_if;
        } else {
            ESP_LOGI(GATTC_TAG, "Reg app failed, app_id %04x, status %d",
                    param->reg.app_id,
                    param->reg.status);
            return;
        }
    }

    /* If the gattc_if equal to profile A, call profile A cb handler,
     * so here call each profile's callback */
    do {
        int idx;
        for (idx = 0; idx < PROFILE_NUM; idx++) {
            if (gattc_if == ESP_GATT_IF_NONE || /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
                    gattc_if == gl_profile_tab[idx].gattc_if) {
                if (gl_profile_tab[idx].gattc_cb) {
                    gl_profile_tab[idx].gattc_cb(event, gattc_if, param);
                }
            }
        }
    } while (0);
}

static void gatts_profile_b_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    switch (event) {
    case ESP_GATTS_REG_EVT:
        ESP_LOGI(COEX_TAG, "REGISTER_APP_EVT, status %d, app_id %d\n", param->reg.status, param->reg.app_id);
        gatts_profile_tab[GATTS_PROFILE_B_APP_ID].service_id.is_primary = true;
        gatts_profile_tab[GATTS_PROFILE_B_APP_ID].service_id.id.inst_id = 0x00;
        gatts_profile_tab[GATTS_PROFILE_B_APP_ID].service_id.id.uuid.len = ESP_UUID_LEN_16;
        gatts_profile_tab[GATTS_PROFILE_B_APP_ID].service_id.id.uuid.uuid.uuid16 = GATTS_SERVICE_UUID_TEST_B;

        esp_ble_gatts_create_service(gatts_if, &gatts_profile_tab[GATTS_PROFILE_B_APP_ID].service_id, GATTS_NUM_HANDLE_TEST_B);
        break;
    case ESP_GATTS_READ_EVT: {
        ESP_LOGI(COEX_TAG, "GATT_READ_EVT, conn_id %d, trans_id %d, handle %d\n", param->read.conn_id, param->read.trans_id, param->read.handle);
        esp_gatt_rsp_t rsp;
        memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
        rsp.attr_value.handle = param->read.handle;
        rsp.attr_value.len = 4;
        rsp.attr_value.value[0] = 0xde;
        rsp.attr_value.value[1] = 0xed;
        rsp.attr_value.value[2] = 0xbe;
        rsp.attr_value.value[3] = 0xef;
        esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id,
                                    ESP_GATT_OK, &rsp);
        break;
    }
    case ESP_GATTS_WRITE_EVT: {
        ESP_LOGI(COEX_TAG, "GATT_WRITE_EVT, conn_id %d, trans_id %d, handle %d\n", param->write.conn_id, param->write.trans_id, param->write.handle);
        if (!param->write.is_prep) {
            ESP_LOGI(COEX_TAG, "GATT_WRITE_EVT, value len %d, value :", param->write.len);
            esp_log_buffer_hex(COEX_TAG, param->write.value, param->write.len);
            if (gatts_profile_tab[GATTS_PROFILE_B_APP_ID].descr_handle == param->write.handle && param->write.len == 2) {
                uint16_t descr_value= param->write.value[1]<<8 | param->write.value[0];
                if (descr_value == NOTIFY_ENABLE) {
                    if (b_property & ESP_GATT_CHAR_PROP_BIT_NOTIFY) {
                        ESP_LOGI(COEX_TAG, "notify enable\n");
                        uint8_t notify_data[15];
                        for (int i = 0; i < sizeof(notify_data); ++ i) {
                            notify_data[i] = i%0xff;
                        }
                        //the size of notify_data[] need less than MTU size
                        esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, gatts_profile_tab[GATTS_PROFILE_B_APP_ID].char_handle,
                                                sizeof(notify_data), notify_data, false);
                    }
                } else if (descr_value == INDICATE_ENABLE) {
                    if (b_property & ESP_GATT_CHAR_PROP_BIT_INDICATE) {
                        ESP_LOGI(COEX_TAG, "indicate enable\n");
                        uint8_t indicate_data[15];
                        for (int i = 0; i < sizeof(indicate_data); ++ i) {
                            indicate_data[i] = i%0xff;
                        }
                        //the size of indicate_data[] need less than MTU size
                        esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, gatts_profile_tab[GATTS_PROFILE_B_APP_ID].char_handle,
                                                    sizeof(indicate_data), indicate_data, true);
                    }
                } else if (descr_value == NOTIFY_INDICATE_DISABLE) {
                    ESP_LOGI(COEX_TAG, "notify/indicate disable \n");
                } else {
                    ESP_LOGE(COEX_TAG, "unknown value\n");
                }

            }
        }
        example_write_event_env(gatts_if, &b_prepare_write_env, param);
        break;
    }
    case ESP_GATTS_EXEC_WRITE_EVT:
        ESP_LOGI(COEX_TAG,"ESP_GATTS_EXEC_WRITE_EVT\n");
        esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
        example_exec_write_event_env(&b_prepare_write_env, param);
        break;
    case ESP_GATTS_MTU_EVT:
        ESP_LOGI(COEX_TAG, "ESP_GATTS_MTU_EVT, MTU %d\n", param->mtu.mtu);
        break;
    case ESP_GATTS_UNREG_EVT:
        break;
    case ESP_GATTS_CREATE_EVT:
        ESP_LOGI(COEX_TAG, "CREATE_SERVICE_EVT, status %d,  service_handle %d\n", param->create.status, param->create.service_handle);
        gatts_profile_tab[GATTS_PROFILE_B_APP_ID].service_handle = param->create.service_handle;
        gatts_profile_tab[GATTS_PROFILE_B_APP_ID].char_uuid.len = ESP_UUID_LEN_16;
        gatts_profile_tab[GATTS_PROFILE_B_APP_ID].char_uuid.uuid.uuid16 = GATTS_CHAR_UUID_TEST_B;

        esp_ble_gatts_start_service(gatts_profile_tab[GATTS_PROFILE_B_APP_ID].service_handle);
        b_property = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
        esp_err_t add_char_ret =esp_ble_gatts_add_char( gatts_profile_tab[GATTS_PROFILE_B_APP_ID].service_handle, &gatts_profile_tab[GATTS_PROFILE_B_APP_ID].char_uuid,
                                                        ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                                        b_property,
                                                        NULL, NULL);
        if (add_char_ret) {
            ESP_LOGE(COEX_TAG, "add char failed, error code =%x\n",add_char_ret);
        }
        break;
    case ESP_GATTS_ADD_INCL_SRVC_EVT:
        break;
    case ESP_GATTS_ADD_CHAR_EVT:
        ESP_LOGI(COEX_TAG, "ADD_CHAR_EVT, status %d,  attr_handle %d, service_handle %d\n",
                 param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle);

        gatts_profile_tab[GATTS_PROFILE_B_APP_ID].char_handle = param->add_char.attr_handle;
        gatts_profile_tab[GATTS_PROFILE_B_APP_ID].descr_uuid.len = ESP_UUID_LEN_16;
        gatts_profile_tab[GATTS_PROFILE_B_APP_ID].descr_uuid.uuid.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
        esp_ble_gatts_add_char_descr(gatts_profile_tab[GATTS_PROFILE_B_APP_ID].service_handle, &gatts_profile_tab[GATTS_PROFILE_B_APP_ID].descr_uuid,
                                     ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                     NULL, NULL);
        break;
    case ESP_GATTS_ADD_CHAR_DESCR_EVT:
        gatts_profile_tab[GATTS_PROFILE_B_APP_ID].descr_handle = param->add_char_descr.attr_handle;
        ESP_LOGI(COEX_TAG, "ADD_DESCR_EVT, status %d, attr_handle %d, service_handle %d\n",
                 param->add_char_descr.status, param->add_char_descr.attr_handle, param->add_char_descr.service_handle);
        break;
    case ESP_GATTS_DELETE_EVT:
        break;
    case ESP_GATTS_START_EVT:
        ESP_LOGI(COEX_TAG, "SERVICE_START_EVT, status %d, service_handle %d\n",
                 param->start.status, param->start.service_handle);
        break;
    case ESP_GATTS_STOP_EVT:
        break;
    case ESP_GATTS_CONNECT_EVT:
        ESP_LOGI(COEX_TAG, "CONNECT_EVT, conn_id %d, remote %02x:%02x:%02x:%02x:%02x:%02x\n",
                 param->connect.conn_id,
                 param->connect.remote_bda[0], param->connect.remote_bda[1], param->connect.remote_bda[2],
                 param->connect.remote_bda[3], param->connect.remote_bda[4], param->connect.remote_bda[5]);
                 gatts_profile_tab[GATTS_PROFILE_B_APP_ID].conn_id = param->connect.conn_id;
        esp_ble_gap_start_advertising(&adv_params);
        break;
    case ESP_GATTS_CONF_EVT:
        ESP_LOGI(COEX_TAG, "ESP_GATTS_CONF_EVT status %d attr_handle %d\n", param->conf.status, param->conf.handle);
        if (param->conf.status != ESP_GATT_OK) {
            esp_log_buffer_hex(COEX_TAG, param->conf.value, param->conf.len);
        }
    break;
    case ESP_GATTS_DISCONNECT_EVT:
    case ESP_GATTS_OPEN_EVT:
    default:
        break;
    }
}
static void example_write_event_env(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param) {
    esp_gatt_status_t status = ESP_GATT_OK;
    if (param->write.need_rsp) {
        if (param->write.is_prep) {
            if (prepare_write_env->prepare_buf == NULL) {
                prepare_write_env->prepare_buf = (uint8_t *)malloc(PREPARE_BUF_MAX_SIZE*sizeof(uint8_t));
                prepare_write_env->prepare_len = 0;
                if (prepare_write_env->prepare_buf == NULL) {
                    ESP_LOGE(COEX_TAG, "Gatt_server prep no mem\n");
                    status = ESP_GATT_NO_RESOURCES;
                }
            } else {
                if(param->write.offset > PREPARE_BUF_MAX_SIZE) {
                    status = ESP_GATT_INVALID_OFFSET;
                } else if ((param->write.offset + param->write.len) > PREPARE_BUF_MAX_SIZE) {
                    status = ESP_GATT_INVALID_ATTR_LEN;
                }
            }

            esp_gatt_rsp_t *gatt_rsp = (esp_gatt_rsp_t *)malloc(sizeof(esp_gatt_rsp_t));
            gatt_rsp->attr_value.len = param->write.len;
            gatt_rsp->attr_value.handle = param->write.handle;
            gatt_rsp->attr_value.offset = param->write.offset;
            gatt_rsp->attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
            memcpy(gatt_rsp->attr_value.value, param->write.value, param->write.len);
            esp_err_t response_err = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, gatt_rsp);
            if (response_err != ESP_OK) {
               ESP_LOGE(COEX_TAG, "Send response error\n");
            }
            free(gatt_rsp);
            if (status != ESP_GATT_OK) {
                return;
            }
            memcpy(prepare_write_env->prepare_buf + param->write.offset,
                   param->write.value,
                   param->write.len);
            prepare_write_env->prepare_len += param->write.len;

        } else {
            esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, NULL);
        }
    }
}

static void example_exec_write_event_env(prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param) {
    if (param->exec_write.exec_write_flag == ESP_GATT_PREP_WRITE_EXEC) {
        esp_log_buffer_hex(COEX_TAG, prepare_write_env->prepare_buf, prepare_write_env->prepare_len);
    } else {
        ESP_LOGI(COEX_TAG,"ESP_GATT_PREP_WRITE_CANCEL\n");
    }
    if (prepare_write_env->prepare_buf) {
        free(prepare_write_env->prepare_buf);
        prepare_write_env->prepare_buf = NULL;
    }
    prepare_write_env->prepare_len = 0;
}

static void gatts_profile_a_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    switch (event) {
    case ESP_GATTS_REG_EVT:
        ESP_LOGI(COEX_TAG, "REGISTER_APP_EVT, status %d, app_id %d\n", param->reg.status, param->reg.app_id);
        gatts_profile_tab[GATTS_PROFILE_A_APP_ID].service_id.is_primary = true;
        gatts_profile_tab[GATTS_PROFILE_A_APP_ID].service_id.id.inst_id = 0x00;
        gatts_profile_tab[GATTS_PROFILE_A_APP_ID].service_id.id.uuid.len = ESP_UUID_LEN_16;
        gatts_profile_tab[GATTS_PROFILE_A_APP_ID].service_id.id.uuid.uuid.uuid16 = GATTS_SERVICE_UUID_TEST_A;

        esp_err_t set_dev_name_ret = esp_ble_gap_set_device_name(GATTS_ADV_NAME);
        if (set_dev_name_ret) {
            ESP_LOGE(COEX_TAG, "set device name failed, error code = %x\n", set_dev_name_ret);
        }
#ifdef CONFIG_SET_RAW_ADV_DATA
        esp_err_t raw_adv_ret = esp_ble_gap_config_adv_data_raw(raw_adv_data, sizeof(raw_adv_data));
        if (raw_adv_ret) {
            ESP_LOGE(COEX_TAG, "config raw adv data failed, error code = %x \n", raw_adv_ret);
        }
        adv_config_done |= adv_config_flag;
        esp_err_t raw_scan_ret = esp_ble_gap_config_scan_rsp_data_raw(raw_scan_rsp_data, sizeof(raw_scan_rsp_data));
        if (raw_scan_ret) {
            ESP_LOGE(COEX_TAG, "config raw scan rsp data failed, error code = %x\n", raw_scan_ret);
        }
        adv_config_done |= scan_rsp_config_flag;
#else
        //config adv data
        esp_err_t ret = esp_ble_gap_config_adv_data(&adv_data);
        if (ret) {
            ESP_LOGE(COEX_TAG, "config adv data failed, error code = %x\n", ret);
        }
        adv_config_done |= adv_config_flag;
        //config scan response data
        ret = esp_ble_gap_config_adv_data(&scan_rsp_data);
        if (ret) {
            ESP_LOGE(COEX_TAG, "config scan response data failed, error code = %x\n", ret);
        }
        adv_config_done |= scan_rsp_config_flag;

#endif
        esp_ble_gatts_create_service(gatts_if, &gatts_profile_tab[GATTS_PROFILE_A_APP_ID].service_id, GATTS_NUM_HANDLE_TEST_A);
        break;
    case ESP_GATTS_READ_EVT: {
        ESP_LOGI(COEX_TAG, "GATT_READ_EVT, conn_id %d, trans_id %d, handle %d\n", param->read.conn_id, param->read.trans_id, param->read.handle);
        esp_gatt_rsp_t rsp;
        memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
        rsp.attr_value.handle = param->read.handle;
        rsp.attr_value.len = 4;
        rsp.attr_value.value[0] = 0xde;
        rsp.attr_value.value[1] = 0xed;
        rsp.attr_value.value[2] = 0xbe;
        rsp.attr_value.value[3] = 0xef;
        esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id,
                                    ESP_GATT_OK, &rsp);
        break;
    }
    case ESP_GATTS_WRITE_EVT: {
        ESP_LOGI(COEX_TAG, "GATT_WRITE_EVT, conn_id %d, trans_id %d, handle %d\n", param->write.conn_id, param->write.trans_id, param->write.handle);
        if (!param->write.is_prep) {
            ESP_LOGI(COEX_TAG, "GATT_WRITE_EVT, value len %d, value :", param->write.len);
            esp_log_buffer_hex(COEX_TAG, param->write.value, param->write.len);
            if (gatts_profile_tab[GATTS_PROFILE_A_APP_ID].descr_handle == param->write.handle && param->write.len == 2) {
                uint16_t descr_value = param->write.value[1]<<8 | param->write.value[0];
                if (descr_value == NOTIFY_ENABLE) {
                    if (a_property & ESP_GATT_CHAR_PROP_BIT_NOTIFY) {
                        ESP_LOGI(COEX_TAG, "notify enable\n");
                        uint8_t notify_data[15];
                        for (int i = 0; i < sizeof(notify_data); ++ i) {
                            notify_data[i] = i%0xff;
                        }
                        //the size of notify_data[] need less than MTU size
                        esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, gatts_profile_tab[GATTS_PROFILE_A_APP_ID].char_handle,
                                                sizeof(notify_data), notify_data, false);
                    }
                } else if (descr_value == INDICATE_ENABLE) {
                    if (a_property & ESP_GATT_CHAR_PROP_BIT_INDICATE) {
                        ESP_LOGI(COEX_TAG, "indicate enable\n");
                        uint8_t indicate_data[15];
                        for (int i = 0; i < sizeof(indicate_data); ++ i) {
                            indicate_data[i] = i%0xff;
                        }
                        //the size of indicate_data[] need less than MTU size
                        esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, gatts_profile_tab[GATTS_PROFILE_A_APP_ID].char_handle,
                                                    sizeof(indicate_data), indicate_data, true);
                    }
                } else if (descr_value == NOTIFY_INDICATE_DISABLE) {
                    ESP_LOGI(COEX_TAG, "notify/indicate disable \n");
                } else {
                    ESP_LOGE(COEX_TAG, "unknown descr value\n");
                    esp_log_buffer_hex(COEX_TAG, param->write.value, param->write.len);
                }

            }
        }
        example_write_event_env(gatts_if, &a_prepare_write_env, param);
        break;
    }
    case ESP_GATTS_EXEC_WRITE_EVT:
        ESP_LOGI(COEX_TAG,"ESP_GATTS_EXEC_WRITE_EVT\n");
        esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
        example_exec_write_event_env(&a_prepare_write_env, param);
        break;
    case ESP_GATTS_MTU_EVT:
        ESP_LOGI(COEX_TAG, "ESP_GATTS_MTU_EVT, MTU %d\n", param->mtu.mtu);
        break;
    case ESP_GATTS_CREATE_EVT:
        ESP_LOGI(COEX_TAG, "CREATE_SERVICE_EVT, status %d,  service_handle %d\n", param->create.status, param->create.service_handle);
        gatts_profile_tab[GATTS_PROFILE_A_APP_ID].service_handle = param->create.service_handle;
        gatts_profile_tab[GATTS_PROFILE_A_APP_ID].char_uuid.len = ESP_UUID_LEN_16;
        gatts_profile_tab[GATTS_PROFILE_A_APP_ID].char_uuid.uuid.uuid16 = GATTS_CHAR_UUID_TEST_A;

        esp_ble_gatts_start_service(gatts_profile_tab[GATTS_PROFILE_A_APP_ID].service_handle);
        a_property = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
        esp_err_t add_char_ret = esp_ble_gatts_add_char(gatts_profile_tab[GATTS_PROFILE_A_APP_ID].service_handle, &gatts_profile_tab[GATTS_PROFILE_A_APP_ID].char_uuid,
                                                        ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                                        a_property,
                                                        &gatts_demo_char1_val, NULL);
        if (add_char_ret) {
            ESP_LOGE(COEX_TAG, "add char failed, error code =%x\n",add_char_ret);
        }
        break;
    case ESP_GATTS_ADD_CHAR_EVT: {
        ESP_LOGI(COEX_TAG, "ADD_CHAR_EVT, status %d,  attr_handle %d, service_handle %d\n",
                param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle);
        gatts_profile_tab[GATTS_PROFILE_A_APP_ID].char_handle = param->add_char.attr_handle;
        gatts_profile_tab[GATTS_PROFILE_A_APP_ID].descr_uuid.len = ESP_UUID_LEN_16;
        gatts_profile_tab[GATTS_PROFILE_A_APP_ID].descr_uuid.uuid.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;

        esp_err_t add_descr_ret = esp_ble_gatts_add_char_descr(gatts_profile_tab[GATTS_PROFILE_A_APP_ID].service_handle, &gatts_profile_tab[GATTS_PROFILE_A_APP_ID].descr_uuid,
                                                                ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, NULL, NULL);
        if (add_descr_ret) {
            ESP_LOGE(COEX_TAG, "add char descr failed, error code =%x\n", add_descr_ret);
        }
        break;
    }
    case ESP_GATTS_ADD_CHAR_DESCR_EVT:
        gatts_profile_tab[GATTS_PROFILE_A_APP_ID].descr_handle = param->add_char_descr.attr_handle;
        ESP_LOGI(COEX_TAG, "ADD_DESCR_EVT, status %d, attr_handle %d, service_handle %d\n",
                 param->add_char_descr.status, param->add_char_descr.attr_handle, param->add_char_descr.service_handle);
        break;
    case ESP_GATTS_START_EVT:
        ESP_LOGI(COEX_TAG, "SERVICE_START_EVT, status %d, service_handle %d\n",
                 param->start.status, param->start.service_handle);
        break;
    case ESP_GATTS_CONNECT_EVT: {
        
        ESP_LOGI(COEX_TAG, "ESP_GATTS_CONNECT_EVT, conn_id %d, remote %02x:%02x:%02x:%02x:%02x:%02x\n",
                 param->connect.conn_id,
                 param->connect.remote_bda[0], param->connect.remote_bda[1], param->connect.remote_bda[2],
                 param->connect.remote_bda[3], param->connect.remote_bda[4], param->connect.remote_bda[5]);
        gatts_profile_tab[GATTS_PROFILE_A_APP_ID].conn_id = param->connect.conn_id;
        esp_ble_gap_start_advertising(&adv_params);
        break;
    }
    case ESP_GATTS_DISCONNECT_EVT:
        ESP_LOGI(COEX_TAG, "ESP_GATTS_DISCONNECT_EVT, disconnect reason 0x%x\n", param->disconnect.reason);
        esp_ble_gap_start_advertising(&adv_params);
        break;
    case ESP_GATTS_CONF_EVT:
        ESP_LOGI(COEX_TAG, "ESP_GATTS_CONF_EVT, status %d attr_handle %d\n", param->conf.status, param->conf.handle);
        if (param->conf.status != ESP_GATT_OK) {
            esp_log_buffer_hex(COEX_TAG, param->conf.value, param->conf.len);
        }
        break;
    case ESP_GATTS_OPEN_EVT:
    default:
        break;
    }
}

static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    /* If event is register event, store the gatts_if for each profile */
    if (event == ESP_GATTS_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            gatts_profile_tab[param->reg.app_id].gatts_if = gatts_if;
        } else {
            ESP_LOGI(COEX_TAG, "Reg app failed, app_id %04x, status %d\n",
                    param->reg.app_id,
                    param->reg.status);
            return;
        }
    }

    /* If the gatts_if equal to profile A, call profile A cb handler,
     * so here call each profile's callback */
    do {
        int idx;
        for (idx = 0; idx < GATTS_PROFILE_NUM; idx ++) {
            if (gatts_if == ESP_GATT_IF_NONE || /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
                    gatts_if == gatts_profile_tab[idx].gatts_if) {
                if (gatts_profile_tab[idx].gatts_cb) {
                    gatts_profile_tab[idx].gatts_cb(event, gatts_if, param);
                }
            }
        }
    } while (0);
}
