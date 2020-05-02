#include "esp_main.h"

char wifi_ssid[32] = "Heslo za pivo";
char wifi_pass[64] = "neviem12";

scan_enum_t scan_type = Just_scan;
scan_enum_t *scan_type_ptr = &scan_type;

uint8_t unknown_sensor[6];
sensor_t sensors[MAX_NUMBER_OF_SENSORS];
uint8_t number_of_sensors = 0;
TaskHandle_t xHandle_increment_beeps;

const char sensors_nvs_key[5][3] = {"s1\0", "s2\0", "s3\0", "s4\0", "s5\0" };

static esp_gattc_char_elem_t *char_elem_result   = NULL;
static esp_gattc_descr_elem_t *descr_elem_result = NULL;

///Declare functions
static void gattc_profile_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);
static void gatts_profile_status_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
static void gatts_profile_sensor_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
static void gatts_profile_IP_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
static void example_write_event_env(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param);
static void example_exec_write_event_env(prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param);

static esp_bt_uuid_t remote_filter_service_uuid = {
    .len = ESP_UUID_LEN_16,
    .uuid = {.uuid16 = REMOTE_SERVICE_UUID,},
};

static bool connect = false;
static bool get_service = false;

static esp_ble_scan_params_t ble_scan_params = {
    .scan_type              = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type          = BLE_ADDR_TYPE_RANDOM,
    .scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval          = 0x50,
    .scan_window            = 0x30,
    .scan_duplicate         = BLE_SCAN_DUPLICATE_ENABLE
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
static prepare_type_env_t b_prepare_write_env;
static uint8_t adv_config_done         = 0;
static uint8_t char1_str[]             = {0x11, 0x22, 0x33};

esp_attr_value_t gatts_demo_char1_val = {
    .attr_max_len = GATTS_DEMO_CHAR_VAL_LEN_MAX,
    .attr_len     = sizeof(char1_str),
    .attr_value   = char1_str,
};

static uint8_t service_uuid128[NUMBER_OF_UUIDS * ESP_UUID_LEN_128] = {
    /* LSB <--------------------------------------------------------------------------------> MSB */
    0x89, 0x38, 0xc2, 0xef, 0x89, 0x67, 0x41, 0xaf, 0xae, 0x4d, 0xaa, 0xfe, 0x6d, 0xb3, 0xdb, 0x56,
    0xae, 0x28, 0x74, 0xca, 0xad, 0xa5, 0x86, 0xac, 0x9b, 0x46, 0x84, 0x39, 0x1e, 0x37, 0x4a, 0x53,
    0xb3, 0x50, 0xf9, 0xdc, 0xf5, 0x78, 0x62, 0x85, 0x4e, 0x4b, 0xce, 0xff, 0xff, 0xec, 0xa2, 0x1b,
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
    .service_uuid_len = NUMBER_OF_UUIDS * ESP_UUID_LEN_128,
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
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY, // ADV_FILTER_ALLOW_SCAN_WLST_CON_WLST
};

/* One gatt-based profile one app_id and one gatts_if, this array will store the gatts_if returned by ESP_GATTS_REG_EVT */
static struct gatts_profile_inst gatts_profile_tab[GATTS_PROFILE_NUM] = {
    [GATTS_PROFILE_A_APP_ID] = {
        .gatts_cb = gatts_profile_status_event_handler,
        .gatts_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },
    [GATTS_PROFILE_B_APP_ID] = {
        .gatts_cb = gatts_profile_sensor_event_handler,
        .gatts_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },
    [GATTS_PROFILE_C_APP_ID] = {
        .gatts_cb = gatts_profile_IP_event_handler,
        .gatts_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },
};

esp_err_t compare_uuids(uint8_t *uuid1, uint8_t *uuid2){
    if(sizeof(uuid1) != sizeof(uuid2)){
        return ESP_FAIL;
    }
    for (int i = 0; i < sizeof(uuid1); i++){
        if(uuid1[i] != uuid2[i]){
            return ESP_FAIL;
        }
    }
    return ESP_OK;
}

static esp_err_t record_sensor(uint8_t *address){
    for (int i = 0; i < number_of_sensors; i++){
        if(compare_uuids(sensors[i].address, address) == ESP_OK){
            sensors[i].missed_beeps = 0;
            return ESP_OK;
        }
    }
    for (int i = 0; i < 6; i++){
        unknown_sensor[i] = address[i];
    }
    return ESP_FAIL;
}

esp_err_t add_new_sensor(uint8_t *address){
    if (number_of_sensors == MAX_NUMBER_OF_SENSORS){
        return ESP_FAIL;
    }
    for (int i = 0; i < number_of_sensors; i++){
        if (compare_uuids(address, sensors[i].address)==ESP_OK){
            return ESP_FAIL;
        }
    }
    nvs_handle_t nvs_handle;
    uint64_t number = 0;
    esp_err_t ret = nvs_open("storage", NVS_READWRITE, &nvs_handle);
    if (ret == ESP_OK) {
        ESP_LOGI(REST_TAG, "Writing sensor address to NVS memory... ");
        for (uint8_t i = 0; i < 6; i++){
            printf("%x\n", address[i]);
            number |= address[i];
            number = number << 8;
            sensors[number_of_sensors].address[i] = address[i];
        }
        ret = nvs_set_u64(nvs_handle, sensors_nvs_key[number_of_sensors], number);
        if(ret == ESP_OK){
            nvs_set_u8(nvs_handle, "sensors_cnt", number_of_sensors+1);
            nvs_commit(nvs_handle);
        }
        nvs_close(nvs_handle);
        ESP_LOGI(REST_TAG, "Done");
    }
    sensors[number_of_sensors].missed_beeps = 0;
    number_of_sensors++;
    return ESP_OK;
}

esp_err_t remove_sensor(uint8_t *address){
    int idx;
    for (int i = 0; i < number_of_sensors; i++){
        if (compare_uuids(address, sensors[i].address)==ESP_OK){
            idx = i;
            break;
        }
        return ESP_FAIL;
    }
    nvs_handle_t nvs_handle;
    uint64_t number = 0;
    esp_err_t ret = nvs_open("storage", NVS_READWRITE, &nvs_handle);
    if (ret == ESP_OK) {
        if (idx != number_of_sensors-1){
            for (uint8_t i = 0; i < 6; i++){
                printf("%x\n", address[i]);
                number |= address[i];
                number = number << 8;
                sensors[number_of_sensors].address[i] = address[i];
            }
            ret = nvs_set_u64(nvs_handle, sensors_nvs_key[idx], number);
        }
        nvs_set_u8(nvs_handle, "sensors_cnt", --number_of_sensors);
        nvs_erase_key(nvs_handle, sensors_nvs_key[number_of_sensors]);
        nvs_commit(nvs_handle);
        nvs_close(nvs_handle);
    }
    esp_ble_remove_bond_device(address);
    return ESP_OK;
}

void increment_sensor_beeps_task(){
    uint8_t wait_time;
    while (1) {
        ESP_LOGI("incrementing task", "start");
        for(uint8_t i = 0; i < number_of_sensors; i++){
            sensors[i].missed_beeps++;
            if (sensors[i].missed_beeps++ >= 4){
                //BAD -alarm or smth
                ESP_LOGE("incrementing task", "senozor sa nehlasi!!!!!");
            }
        }
        wait_time = 40;
        if(*security_state > Activating){
            wait_time = 20;
        }
        vTaskDelay(wait_time*1000 / portTICK_PERIOD_MS);
    }
}

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


static void gattc_profile_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
    esp_ble_gattc_cb_param_t *p_data = (esp_ble_gattc_cb_param_t *)param;

    switch (event) {
    case ESP_GATTC_REG_EVT:
        ESP_LOGI(BLE_SECURITY_SYSTEM, "REG_EVT");
        esp_ble_gap_config_local_privacy(true);
        break;
    case ESP_GATTC_CONNECT_EVT: {
        ESP_LOGI(BLE_SECURITY_SYSTEM, "ESP_GATTC_CONNECT_EVT conn_id %d, if %d\n", p_data->connect.conn_id, gattc_if);
        gl_profile_tab[GATTC_PROFILE_D_APP_ID].conn_id = p_data->connect.conn_id;
        memcpy(gl_profile_tab[GATTC_PROFILE_D_APP_ID].remote_bda, p_data->connect.remote_bda, sizeof(esp_bd_addr_t));
        ESP_LOGI(BLE_SECURITY_SYSTEM, "REMOTE BDA:");
        esp_log_buffer_hex(BLE_SECURITY_SYSTEM, gl_profile_tab[GATTC_PROFILE_D_APP_ID].remote_bda, sizeof(esp_bd_addr_t));
        esp_err_t mtu_ret = esp_ble_gattc_send_mtu_req (gattc_if, p_data->connect.conn_id);
        if (mtu_ret) {
            ESP_LOGE(BLE_SECURITY_SYSTEM, "config MTU error, error code = %x\n", mtu_ret);
        }
        break;
    }
    case ESP_GATTC_OPEN_EVT:
        if (param->open.status != ESP_GATT_OK){
            ESP_LOGE(BLE_SECURITY_SYSTEM, "open failed, error status = %x", p_data->open.status);
            break;
        }
        ESP_LOGI(BLE_SECURITY_SYSTEM, "open success");
        gl_profile_tab[PROFILE_A_APP_ID].conn_id = p_data->open.conn_id;
        memcpy(gl_profile_tab[PROFILE_A_APP_ID].remote_bda, p_data->open.remote_bda, sizeof(esp_bd_addr_t));
        ESP_LOGI(BLE_SECURITY_SYSTEM, "REMOTE BDA:");
        esp_log_buffer_hex(BLE_SECURITY_SYSTEM, gl_profile_tab[PROFILE_A_APP_ID].remote_bda, sizeof(esp_bd_addr_t));
        esp_err_t mtu_ret = esp_ble_gattc_send_mtu_req (gattc_if, p_data->open.conn_id);
        if (mtu_ret){
            ESP_LOGE(BLE_SECURITY_SYSTEM, "config MTU error, error code = %x", mtu_ret);
        }
        break;
    case ESP_GATTC_DIS_SRVC_CMPL_EVT:
        if (param->dis_srvc_cmpl.status != ESP_GATT_OK) {
            ESP_LOGE(BLE_SECURITY_SYSTEM, "discover service failed, status %d\n", param->dis_srvc_cmpl.status);
            break;
        }
        ESP_LOGI(BLE_SECURITY_SYSTEM, "discover service complete conn_id %d\n", param->dis_srvc_cmpl.conn_id);
        esp_ble_gattc_search_service(gattc_if, param->cfg_mtu.conn_id, &remote_filter_service_uuid);
        break;
    case ESP_GATTC_CFG_MTU_EVT:
        if (param->cfg_mtu.status != ESP_GATT_OK){
            ESP_LOGE(BLE_SECURITY_SYSTEM,"config mtu failed, error status = %x", param->cfg_mtu.status);
        }
        ESP_LOGI(BLE_SECURITY_SYSTEM, "ESP_GATTC_CFG_MTU_EVT, Status %d, MTU %d, conn_id %d", param->cfg_mtu.status, param->cfg_mtu.mtu, param->cfg_mtu.conn_id);
        esp_ble_gattc_search_service(gattc_if, param->cfg_mtu.conn_id, &remote_filter_service_uuid);
        break;
    case ESP_GATTC_SEARCH_RES_EVT: {
        ESP_LOGI(BLE_SECURITY_SYSTEM, "SEARCH RES: conn_id = %x is primary service %d", p_data->search_res.conn_id, p_data->search_res.is_primary);
        ESP_LOGI(BLE_SECURITY_SYSTEM, "start handle %d end handle %d current handle value %d", p_data->search_res.start_handle, p_data->search_res.end_handle, p_data->search_res.srvc_id.inst_id);
        if (p_data->search_res.srvc_id.uuid.len == ESP_UUID_LEN_16 && p_data->search_res.srvc_id.uuid.uuid.uuid16 == REMOTE_SERVICE_UUID) {
            ESP_LOGI(BLE_SECURITY_SYSTEM, "UUID16: %x", p_data->search_res.srvc_id.uuid.uuid.uuid16);
            get_service = true;
            gl_profile_tab[PROFILE_A_APP_ID].service_start_handle = p_data->search_res.start_handle;
            gl_profile_tab[PROFILE_A_APP_ID].service_end_handle = p_data->search_res.end_handle;
        }
        break;
    }
    case ESP_GATTC_SEARCH_CMPL_EVT:
        if (p_data->search_cmpl.status != ESP_GATT_OK){
            ESP_LOGE(BLE_SECURITY_SYSTEM, "search service failed, error status = %x", p_data->search_cmpl.status);
            break;
        }
        if(p_data->search_cmpl.searched_service_source == ESP_GATT_SERVICE_FROM_REMOTE_DEVICE) {
            ESP_LOGI(BLE_SECURITY_SYSTEM, "Get service information from remote device");
        } else if (p_data->search_cmpl.searched_service_source == ESP_GATT_SERVICE_FROM_NVS_FLASH) {
            ESP_LOGI(BLE_SECURITY_SYSTEM, "Get service information from flash");
        } else {
            ESP_LOGI(BLE_SECURITY_SYSTEM, "unknown service source");
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
                ESP_LOGE(BLE_SECURITY_SYSTEM, "esp_ble_gattc_get_attr_count error, %d", __LINE__);
            }
            if (count > 0){
                char_elem_result = (esp_gattc_char_elem_t *)malloc(sizeof(esp_gattc_char_elem_t) * count);
                if (!char_elem_result){
                    ESP_LOGE(BLE_SECURITY_SYSTEM, "gattc no mem");
                }else{
                    ret_status = esp_ble_gattc_get_all_char(gattc_if,
                                                            gl_profile_tab[PROFILE_A_APP_ID].conn_id,
                                                            gl_profile_tab[PROFILE_A_APP_ID].service_start_handle,
                                                            gl_profile_tab[PROFILE_A_APP_ID].service_end_handle,
                                                            char_elem_result,
                                                            &count,
                                                            offset);
                    if (ret_status != ESP_GATT_OK){
                        ESP_LOGE(BLE_SECURITY_SYSTEM, "esp_ble_gattc_get_all_char error, %d", __LINE__);
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
            ESP_LOGE(BLE_SECURITY_SYSTEM, "reg for notify failed, error status = %x", p_data->reg_for_notify.status);
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
                ESP_LOGE(BLE_SECURITY_SYSTEM, "esp_ble_gattc_get_attr_count error, %d", __LINE__);
            }
            if (count > 0){
                descr_elem_result = malloc(sizeof(esp_gattc_descr_elem_t) * count);
                if (!descr_elem_result){
                    ESP_LOGE(BLE_SECURITY_SYSTEM, "malloc error, gattc no mem");
                }else{
                    ret_status = esp_ble_gattc_get_all_descr(gattc_if,
                                                             gl_profile_tab[PROFILE_A_APP_ID].conn_id,
                                                             p_data->reg_for_notify.handle,
                                                             descr_elem_result,
                                                             &count,
                                                             offset);
                if (ret_status != ESP_GATT_OK){
                    ESP_LOGE(BLE_SECURITY_SYSTEM, "esp_ble_gattc_get_all_descr error, %d", __LINE__);
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
        ESP_LOGI(BLE_SECURITY_SYSTEM, "ESP_GATTC_NOTIFY_EVT, receive notify value:");
        esp_log_buffer_hex(BLE_SECURITY_SYSTEM, p_data->notify.value, p_data->notify.value_len);
        break;
    case ESP_GATTC_WRITE_DESCR_EVT:
        if (p_data->write.status != ESP_GATT_OK){
            ESP_LOGE(BLE_SECURITY_SYSTEM, "write descr failed, error status = %x", p_data->write.status);
            break;
        }
        ESP_LOGI(BLE_SECURITY_SYSTEM, "write descr success");
        break;
    case ESP_GATTC_SRVC_CHG_EVT: {
        esp_bd_addr_t bda;
        memcpy(bda, p_data->srvc_chg.remote_bda, sizeof(esp_bd_addr_t));
        ESP_LOGI(BLE_SECURITY_SYSTEM, "ESP_GATTC_SRVC_CHG_EVT, bd_addr:");
        esp_log_buffer_hex(BLE_SECURITY_SYSTEM, bda, sizeof(esp_bd_addr_t));
        break;
    }
    case ESP_GATTC_WRITE_CHAR_EVT:
        if (p_data->write.status != ESP_GATT_OK){
            ESP_LOGE(BLE_SECURITY_SYSTEM, "write char failed, error status = %x", p_data->write.status);
            break;
        }
        ESP_LOGI(BLE_SECURITY_SYSTEM, "Write char success ");
        break;
    case ESP_GATTC_DISCONNECT_EVT:
        ESP_LOGI(BLE_SECURITY_SYSTEM, "ESP_GATTC_DISCONNECT_EVT, reason = 0x%x", p_data->disconnect.reason);
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
            ESP_LOGE(BLE_SECURITY_SYSTEM, "Advertising start failed\n");
        }
        ESP_LOGI(BLE_SECURITY_SYSTEM, "Advertising start successfully\n");
        break;
    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(BLE_SECURITY_SYSTEM, "Advertising stop failed\n");
        } else {
            ESP_LOGI(BLE_SECURITY_SYSTEM, "Stop adv successfully\n");
        }
        break;
    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
         ESP_LOGI(BLE_SECURITY_SYSTEM, "update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d\n",
                  param->update_conn_params.status,
                  param->update_conn_params.min_int,
                  param->update_conn_params.max_int,
                  param->update_conn_params.conn_int,
                  param->update_conn_params.latency,
                  param->update_conn_params.timeout);
        break;
    case ESP_GAP_BLE_SET_LOCAL_PRIVACY_COMPLETE_EVT:
        if (param->local_privacy_cmpl.status != ESP_BT_STATUS_SUCCESS){
            ESP_LOGE(BLE_SECURITY_SYSTEM, "config local privacy failed, error code =%x", param->local_privacy_cmpl.status);
            break;
        }
        esp_err_t scan_ret = esp_ble_gap_set_scan_params(&ble_scan_params);
        if (scan_ret){
            ESP_LOGE(BLE_SECURITY_SYSTEM, "set scan params error, error code = %x", scan_ret);
        }
        break;
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
        break;
    }
    case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
        //scan start complete event to indicate scan start successfully or failed
        if (param->scan_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(BLE_SECURITY_SYSTEM, "scan start failed, error status = %x", param->scan_start_cmpl.status);
            break;
        }
        ESP_LOGI(BLE_SECURITY_SYSTEM, "Scan start success");
        break;
    case ESP_GAP_BLE_PASSKEY_REQ_EVT:                           /* passkey request event */
        /* Call the following function to input the passkey which is displayed on the remote device */
        //esp_ble_passkey_reply(gl_profile_tab[PROFILE_A_APP_ID].remote_bda, true, 0x00);
        ESP_LOGI(BLE_SECURITY_SYSTEM, "ESP_GAP_BLE_PASSKEY_REQ_EVT");
        break;
    case ESP_GAP_BLE_OOB_REQ_EVT: {
        ESP_LOGI(BLE_SECURITY_SYSTEM, "ESP_GAP_BLE_OOB_REQ_EVT");
        uint8_t tk[16] = {1}; //If you paired with OOB, both devices need to use the same tk
        esp_ble_oob_req_reply(param->ble_security.ble_req.bd_addr, tk, sizeof(tk));
        break;
    }
    case ESP_GAP_BLE_LOCAL_IR_EVT:                               /* BLE local IR event */
        ESP_LOGI(BLE_SECURITY_SYSTEM, "ESP_GAP_BLE_LOCAL_IR_EVT");
        break;
    case ESP_GAP_BLE_LOCAL_ER_EVT:                               /* BLE local ER event */
        ESP_LOGI(BLE_SECURITY_SYSTEM, "ESP_GAP_BLE_LOCAL_ER_EVT");
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
        ESP_LOGI(BLE_SECURITY_SYSTEM, "ESP_GAP_BLE_NC_REQ_EVT, the passkey Notify number:%d", param->ble_security.key_notif.passkey);
        break;
    case ESP_GAP_BLE_PASSKEY_NOTIF_EVT:  ///the app will receive this evt when the IO  has Output capability and the peer device IO has Input capability.
        ///show the passkey number to the user to input it in the peer device.
        ESP_LOGI(BLE_SECURITY_SYSTEM, "The passkey Notify number:%06d", param->ble_security.key_notif.passkey);
        break;
    case ESP_GAP_BLE_KEY_EVT:
        //shows the ble key info share with peer device to the user.
        ESP_LOGI(BLE_SECURITY_SYSTEM, "key type = %s", esp_key_type_to_str(param->ble_security.ble_key.key_type));
        break;
    case ESP_GAP_BLE_AUTH_CMPL_EVT: {
        esp_bd_addr_t bd_addr;
        memcpy(bd_addr, param->ble_security.auth_cmpl.bd_addr, sizeof(esp_bd_addr_t));
        ESP_LOGI(BLE_SECURITY_SYSTEM, "remote BD_ADDR: %08x%04x",\
                (bd_addr[0] << 24) + (bd_addr[1] << 16) + (bd_addr[2] << 8) + bd_addr[3],
                (bd_addr[4] << 8) + bd_addr[5]);
        ESP_LOGI(BLE_SECURITY_SYSTEM, "address type = %d", param->ble_security.auth_cmpl.addr_type);
        ESP_LOGI(BLE_SECURITY_SYSTEM, "pair status = %s",param->ble_security.auth_cmpl.success ? "success" : "fail");
        if (!param->ble_security.auth_cmpl.success) {
            ESP_LOGI(BLE_SECURITY_SYSTEM, "fail reason = 0x%x",param->ble_security.auth_cmpl.fail_reason);
        } else {
            ESP_LOGI(BLE_SECURITY_SYSTEM, "auth mode = %s",esp_auth_req_to_str(param->ble_security.auth_cmpl.auth_mode));    
        }
        break;
    }
    case ESP_GAP_BLE_SCAN_RESULT_EVT: {
        esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;
        cJSON *json_obj = cJSON_CreateObject();
        switch (scan_result->scan_rst.search_evt) {
        case ESP_GAP_SEARCH_INQ_RES_EVT:
            esp_log_buffer_hex(BLE_SECURITY_SYSTEM, scan_result->scan_rst.bda, 6);
            ESP_LOGI(BLE_SECURITY_SYSTEM, "Searched Adv Data Len %d, Scan Response Len %d", scan_result->scan_rst.adv_data_len, scan_result->scan_rst.scan_rsp_len);
            adv_name = esp_ble_resolve_adv_data(scan_result->scan_rst.ble_adv,
                                                ESP_BLE_AD_TYPE_NAME_CMPL, &adv_name_len);
            if (adv_name_len > 1){
                adv_name[adv_name_len] = '\0';
            }
            
            switch (*scan_type_ptr){
                case Just_scan:{
                    char address[20];
                    uint8_t *a = scan_result->scan_rst.bda;
                    sprintf(address, "%02x:%02x:%02x:%02x:%02x:%02x", a[0], a[1], a[2], a[3], a[4], a[5]);
                    cJSON_AddStringToObject(json_obj, "address", (const char*)address);
                    cJSON_AddStringToObject(json_obj, "name", (const char*)adv_name);
                    cJSON_AddItemToArray(json_resp, json_obj);
                    ESP_LOGI(BLE_SECURITY_SYSTEM, "Searched Device Name Len %d", adv_name_len);
                    esp_log_buffer_char(BLE_SECURITY_SYSTEM, adv_name, adv_name_len);
                    ESP_LOGI(BLE_SECURITY_SYSTEM, "\n");
                    }
                    break;
                case Add_new:
                    if (compare_uuids(new_address, scan_result->scan_rst.bda) == ESP_OK){
                        esp_ble_gap_stop_scanning();
                        if (connect == false) {
                            connect = true;
                            ESP_LOGI(BLE_SECURITY_SYSTEM, "connect to the remote device.");
                            esp_ble_gattc_open(gl_profile_tab[PROFILE_A_APP_ID].gattc_if, scan_result->scan_rst.bda, scan_result->scan_rst.ble_addr_type, true);
                        }
                    }
                    break;
                case Search_known:{
                    if (*security_state == Armed){
                        int bond_cnt = esp_ble_get_bond_device_num();
                        esp_ble_bond_dev_t list[bond_cnt];
                        ESP_LOGI(REST_TAG,"list length: %d", bond_cnt);
                        esp_ble_get_bond_device_list(&bond_cnt, list);

                        for(int i = 0; i < bond_cnt; i++){
                            if (compare_uuids(list[i].bd_addr, scan_result->scan_rst.bda) == ESP_OK){
                                esp_ble_gap_stop_scanning();
                                if (connect == false) {
                                    connect = true;
                                    ESP_LOGI(BLE_SECURITY_SYSTEM, "connect to the remote device.");
                                    esp_err_t ret = esp_ble_gattc_open(gl_profile_tab[PROFILE_A_APP_ID].gattc_if, scan_result->scan_rst.bda, scan_result->scan_rst.ble_addr_type, true);
                                    if (ret == ESP_OK){
                                        xTaskCreate(stop_alarm_task, "stop_alarm", 1024*2, NULL, configMAX_PRIORITIES-1, NULL);
                                        vTaskDelete(xHandle_search);
                                        *security_state = Disarmed;
                                    }
                                }
                            }
                        }
                    }else{
                        esp_ble_gap_stop_scanning();
                    }
                    }
                    break;
                default:
                    break;
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
            ESP_LOGE(BLE_SECURITY_SYSTEM, "Scan stop failed, error status = %x", param->scan_stop_cmpl.status);
            break;
        }
        ESP_LOGI(BLE_SECURITY_SYSTEM, "Stop scan successfully");
        break;

    default:
        break;
    }
}

static void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
    ESP_LOGI(BLE_SECURITY_SYSTEM, "EVT %d, gattc if %d", event, gattc_if);

    /* If event is register event, store the gattc_if for each profile */
    if (event == ESP_GATTC_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            gl_profile_tab[param->reg.app_id].gattc_if = gattc_if;
        } else {
            ESP_LOGI(BLE_SECURITY_SYSTEM, "Reg app failed, app_id %04x, status %d",
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

static void gatts_profile_IP_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    switch (event) {
    case ESP_GATTS_REG_EVT:
        ESP_LOGI(BLE_SECURITY_SYSTEM, "REGISTER_APP_EVT, status %d, app_id %d\n", param->reg.status, param->reg.app_id);
        gatts_profile_tab[GATTS_PROFILE_C_APP_ID].service_id.is_primary = true;
        gatts_profile_tab[GATTS_PROFILE_C_APP_ID].service_id.id.inst_id = 0x00;
        gatts_profile_tab[GATTS_PROFILE_C_APP_ID].service_id.id.uuid.len = ESP_UUID_LEN_128;
        memcpy(gatts_profile_tab[GATTS_PROFILE_C_APP_ID].service_id.id.uuid.uuid.uuid128, &(service_uuid128[32]), ESP_UUID_LEN_128);

        esp_ble_gatts_create_service(gatts_if, &gatts_profile_tab[GATTS_PROFILE_C_APP_ID].service_id, GATTS_NUM_HANDLE_TEST_A);
        break;
    case ESP_GATTS_READ_EVT: {
        ESP_LOGI(BLE_SECURITY_SYSTEM, "GATT_READ_EVT, conn_id %d, trans_id %d, handle %d\n", param->read.conn_id, param->read.trans_id, param->read.handle);
        esp_gatt_rsp_t rsp;
        memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
        rsp.attr_value.handle = param->read.handle;
        rsp.attr_value.len = 4;
        for (int i = 0; i < 4; i++){
            rsp.attr_value.value[i] = ((uint8_t*)(&(&(s_ip_addr))->addr))[i];
            ESP_LOGI("wifi", "%d", rsp.attr_value.value[i]);
        }
        esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id,
                                    ESP_GATT_OK, &rsp);
        break;
    }
    case ESP_GATTS_MTU_EVT:
        ESP_LOGI(BLE_SECURITY_SYSTEM, "ESP_GATTS_MTU_EVT, MTU %d\n", param->mtu.mtu);
        break;
    case ESP_GATTS_UNREG_EVT:
        break;
    case ESP_GATTS_CREATE_EVT:
        ESP_LOGI(BLE_SECURITY_SYSTEM, "CREATE_SERVICE_EVT, status %d,  service_handle %d\n", param->create.status, param->create.service_handle);
        gatts_profile_tab[GATTS_PROFILE_C_APP_ID].service_handle = param->create.service_handle;
        gatts_profile_tab[GATTS_PROFILE_C_APP_ID].char_uuid.len = ESP_UUID_LEN_128;
        memcpy(gatts_profile_tab[GATTS_PROFILE_C_APP_ID].char_uuid.uuid.uuid128, &(service_uuid128[32]), ESP_UUID_LEN_128);

        esp_ble_gatts_start_service(gatts_profile_tab[GATTS_PROFILE_C_APP_ID].service_handle);
        b_property = ESP_GATT_CHAR_PROP_BIT_READ;
        esp_err_t add_char_ret =esp_ble_gatts_add_char( gatts_profile_tab[GATTS_PROFILE_C_APP_ID].service_handle, &gatts_profile_tab[GATTS_PROFILE_C_APP_ID].char_uuid,
                                                        ESP_GATT_PERM_READ,
                                                        b_property,
                                                        NULL, NULL);
        if (add_char_ret) {
            ESP_LOGE(BLE_SECURITY_SYSTEM, "add char failed, error code =%x\n",add_char_ret);
        }
        break;
    case ESP_GATTS_ADD_INCL_SRVC_EVT:
        break;
    case ESP_GATTS_ADD_CHAR_EVT:
        ESP_LOGI(BLE_SECURITY_SYSTEM, "ADD_CHAR_EVT, status %d,  attr_handle %d, service_handle %d\n",
                 param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle);

        gatts_profile_tab[GATTS_PROFILE_C_APP_ID].char_handle = param->add_char.attr_handle;
        gatts_profile_tab[GATTS_PROFILE_C_APP_ID].descr_uuid.len = ESP_UUID_LEN_16;
        gatts_profile_tab[GATTS_PROFILE_C_APP_ID].descr_uuid.uuid.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
        esp_ble_gatts_add_char_descr(gatts_profile_tab[GATTS_PROFILE_C_APP_ID].service_handle, &gatts_profile_tab[GATTS_PROFILE_C_APP_ID].descr_uuid,
                                     ESP_GATT_PERM_READ,
                                     NULL, NULL);
        break;
    case ESP_GATTS_ADD_CHAR_DESCR_EVT:
        gatts_profile_tab[GATTS_PROFILE_C_APP_ID].descr_handle = param->add_char_descr.attr_handle;
        ESP_LOGI(BLE_SECURITY_SYSTEM, "ADD_DESCR_EVT, status %d, attr_handle %d, service_handle %d\n",
                 param->add_char_descr.status, param->add_char_descr.attr_handle, param->add_char_descr.service_handle);
        break;
    case ESP_GATTS_DELETE_EVT:
        break;
    case ESP_GATTS_START_EVT:
        ESP_LOGI(BLE_SECURITY_SYSTEM, "SERVICE_START_EVT, status %d, service_handle %d\n",
                 param->start.status, param->start.service_handle);
        break;
    case ESP_GATTS_STOP_EVT:
        break;
    case ESP_GATTS_CONNECT_EVT:
        ESP_LOGI(BLE_SECURITY_SYSTEM, "CONNECT_EVT, conn_id %d, remote %02x:%02x:%02x:%02x:%02x:%02x\n",
                 param->connect.conn_id,
                 param->connect.remote_bda[0], param->connect.remote_bda[1], param->connect.remote_bda[2],
                 param->connect.remote_bda[3], param->connect.remote_bda[4], param->connect.remote_bda[5]);
                 gatts_profile_tab[GATTS_PROFILE_C_APP_ID].conn_id = param->connect.conn_id;
        esp_ble_set_encryption(param->connect.remote_bda, ESP_BLE_SEC_ENCRYPT_MITM);
        esp_ble_gap_start_advertising(&adv_params);
        break;
    case ESP_GATTS_CONF_EVT:
        ESP_LOGI(BLE_SECURITY_SYSTEM, "ESP_GATTS_CONF_EVT status %d attr_handle %d\n", param->conf.status, param->conf.handle);
        if (param->conf.status != ESP_GATT_OK) {
            esp_log_buffer_hex(BLE_SECURITY_SYSTEM, param->conf.value, param->conf.len);
        }
    break;
    case ESP_GATTS_DISCONNECT_EVT:
    case ESP_GATTS_OPEN_EVT:
    default:
        break;
    }
}

static void gatts_profile_sensor_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    switch (event) {
    case ESP_GATTS_REG_EVT:
        ESP_LOGI(BLE_SECURITY_SYSTEM, "REGISTER_APP_EVT, status %d, app_id %d\n", param->reg.status, param->reg.app_id);
        gatts_profile_tab[GATTS_PROFILE_B_APP_ID].service_id.is_primary = true;
        gatts_profile_tab[GATTS_PROFILE_B_APP_ID].service_id.id.inst_id = 0x00;
        gatts_profile_tab[GATTS_PROFILE_B_APP_ID].service_id.id.uuid.len = ESP_UUID_LEN_128;
        memcpy(gatts_profile_tab[GATTS_PROFILE_B_APP_ID].service_id.id.uuid.uuid.uuid128, &(service_uuid128[16]), ESP_UUID_LEN_128);

        esp_ble_gatts_create_service(gatts_if, &gatts_profile_tab[GATTS_PROFILE_B_APP_ID].service_id, GATTS_NUM_HANDLE_TEST_A);
        break;
    case ESP_GATTS_READ_EVT: {
        ESP_LOGI(BLE_SECURITY_SYSTEM, "GATT_READ_EVT, conn_id %d, trans_id %d, handle %d\n", param->read.conn_id, param->read.trans_id, param->read.handle);
        esp_gatt_rsp_t rsp;
        memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
        rsp.attr_value.handle = param->read.handle;
        rsp.attr_value.len = 1;
        rsp.attr_value.value[0] = 0x00;
        esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id,
                                    ESP_GATT_OK, &rsp);
        break;
    }
    case ESP_GATTS_WRITE_EVT: {
        ESP_LOGI(BLE_SECURITY_SYSTEM, "GATT_WRITE_EVT, conn_id %d, trans_id %d, handle %d\n", param->write.conn_id, param->write.trans_id, param->write.handle);
        if (!param->write.is_prep) {
            ESP_LOGI(BLE_SECURITY_SYSTEM, "GATT_WRITE_EVT, value len %d, value :", param->write.len);
            if (*security_state == Armed && *(param->write.value) > 0){
                *security_state = Alarm;
                vTaskDelete(xHandle_search);
                xTaskCreate(alarm_task, "alarm_task", 1024*2, NULL, configMAX_PRIORITIES-1, &xHandle_alarm);
                ESP_LOGI(BLE_SECURITY_SYSTEM, "Alarm triggered by sensor");
            }
            esp_log_buffer_hex(BLE_SECURITY_SYSTEM, param->write.value, param->write.len);
            if (gatts_profile_tab[GATTS_PROFILE_B_APP_ID].descr_handle == param->write.handle && param->write.len == 2) {
                uint16_t descr_value= param->write.value[1]<<8 | param->write.value[0];
                if (descr_value == NOTIFY_ENABLE) {
                    if (b_property & ESP_GATT_CHAR_PROP_BIT_NOTIFY) {
                        ESP_LOGI(BLE_SECURITY_SYSTEM, "notify enable\n");
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
                        ESP_LOGI(BLE_SECURITY_SYSTEM, "indicate enable\n");
                        uint8_t indicate_data[15];
                        for (int i = 0; i < sizeof(indicate_data); ++ i) {
                            indicate_data[i] = i%0xff;
                        }
                        //the size of indicate_data[] need less than MTU size
                        esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, gatts_profile_tab[GATTS_PROFILE_B_APP_ID].char_handle,
                                                    sizeof(indicate_data), indicate_data, true);
                    }
                } else if (descr_value == NOTIFY_INDICATE_DISABLE) {
                    ESP_LOGI(BLE_SECURITY_SYSTEM, "notify/indicate disable \n");
                } else {
                    ESP_LOGE(BLE_SECURITY_SYSTEM, "unknown value\n");
                }

            }
        }
        example_write_event_env(gatts_if, &b_prepare_write_env, param);
        break;
    }
    case ESP_GATTS_EXEC_WRITE_EVT:
        ESP_LOGI(BLE_SECURITY_SYSTEM,"ESP_GATTS_EXEC_WRITE_EVT\n");
        esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
        example_exec_write_event_env(&b_prepare_write_env, param);
        break;
    case ESP_GATTS_MTU_EVT:
        ESP_LOGI(BLE_SECURITY_SYSTEM, "ESP_GATTS_MTU_EVT, MTU %d\n", param->mtu.mtu);
        break;
    case ESP_GATTS_UNREG_EVT:
        break;
    case ESP_GATTS_CREATE_EVT:
        ESP_LOGI(BLE_SECURITY_SYSTEM, "CREATE_SERVICE_EVT, status %d,  service_handle %d\n", param->create.status, param->create.service_handle);
        gatts_profile_tab[GATTS_PROFILE_B_APP_ID].service_handle = param->create.service_handle;
        gatts_profile_tab[GATTS_PROFILE_B_APP_ID].char_uuid.len = ESP_UUID_LEN_128;
        memcpy(gatts_profile_tab[GATTS_PROFILE_B_APP_ID].char_uuid.uuid.uuid128, &(service_uuid128[16]), ESP_UUID_LEN_128);

        esp_ble_gatts_start_service(gatts_profile_tab[GATTS_PROFILE_B_APP_ID].service_handle);
        b_property = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
        esp_err_t add_char_ret =esp_ble_gatts_add_char( gatts_profile_tab[GATTS_PROFILE_B_APP_ID].service_handle, &gatts_profile_tab[GATTS_PROFILE_B_APP_ID].char_uuid,
                                                        ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                                        b_property,
                                                        NULL, NULL);
        if (add_char_ret) {
            ESP_LOGE(BLE_SECURITY_SYSTEM, "add char failed, error code =%x\n",add_char_ret);
        }
        break;
    case ESP_GATTS_ADD_INCL_SRVC_EVT:
        break;
    case ESP_GATTS_ADD_CHAR_EVT:
        ESP_LOGI(BLE_SECURITY_SYSTEM, "ADD_CHAR_EVT, status %d,  attr_handle %d, service_handle %d\n",
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
        ESP_LOGI(BLE_SECURITY_SYSTEM, "ADD_DESCR_EVT, status %d, attr_handle %d, service_handle %d\n",
                 param->add_char_descr.status, param->add_char_descr.attr_handle, param->add_char_descr.service_handle);
        break;
    case ESP_GATTS_DELETE_EVT:
        break;
    case ESP_GATTS_START_EVT:
        ESP_LOGI(BLE_SECURITY_SYSTEM, "SERVICE_START_EVT, status %d, service_handle %d\n",
                 param->start.status, param->start.service_handle);
        break;
    case ESP_GATTS_STOP_EVT:
        break;
    case ESP_GATTS_CONNECT_EVT:
        ESP_LOGI(BLE_SECURITY_SYSTEM, "CONNECT_EVT, conn_id %d, remote %02x:%02x:%02x:%02x:%02x:%02x\n",
                 param->connect.conn_id,
                 param->connect.remote_bda[0], param->connect.remote_bda[1], param->connect.remote_bda[2],
                 param->connect.remote_bda[3], param->connect.remote_bda[4], param->connect.remote_bda[5]);
                 gatts_profile_tab[GATTS_PROFILE_B_APP_ID].conn_id = param->connect.conn_id;
        esp_ble_set_encryption(param->connect.remote_bda, ESP_BLE_SEC_ENCRYPT_MITM);
        esp_ble_gap_start_advertising(&adv_params);
        break;
    case ESP_GATTS_CONF_EVT:
        ESP_LOGI(BLE_SECURITY_SYSTEM, "ESP_GATTS_CONF_EVT status %d attr_handle %d\n", param->conf.status, param->conf.handle);
        if (param->conf.status != ESP_GATT_OK) {
            esp_log_buffer_hex(BLE_SECURITY_SYSTEM, param->conf.value, param->conf.len);
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
                    ESP_LOGE(BLE_SECURITY_SYSTEM, "Gatt_server prep no mem\n");
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
               ESP_LOGE(BLE_SECURITY_SYSTEM, "Send response error\n");
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
        esp_log_buffer_hex(BLE_SECURITY_SYSTEM, prepare_write_env->prepare_buf, prepare_write_env->prepare_len);
    } else {
        ESP_LOGI(BLE_SECURITY_SYSTEM,"ESP_GATT_PREP_WRITE_CANCEL\n");
    }
    if (prepare_write_env->prepare_buf) {
        free(prepare_write_env->prepare_buf);
        prepare_write_env->prepare_buf = NULL;
    }
    prepare_write_env->prepare_len = 0;
}

static void gatts_profile_status_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    switch (event) {
    case ESP_GATTS_REG_EVT:
        ESP_LOGI(BLE_SECURITY_SYSTEM, "REGISTER_APP_EVT, status %d, app_id %d\n", param->reg.status, param->reg.app_id);
        gatts_profile_tab[GATTS_PROFILE_A_APP_ID].service_id.is_primary = true;
        gatts_profile_tab[GATTS_PROFILE_A_APP_ID].service_id.id.inst_id = 0x00;
        gatts_profile_tab[GATTS_PROFILE_A_APP_ID].service_id.id.uuid.len = ESP_UUID_LEN_128;
        memcpy(gatts_profile_tab[GATTS_PROFILE_A_APP_ID].service_id.id.uuid.uuid.uuid128, service_uuid128, ESP_UUID_LEN_128);

        esp_err_t set_dev_name_ret = esp_ble_gap_set_device_name(GATTS_ADV_NAME);
        if (set_dev_name_ret) {
            ESP_LOGE(BLE_SECURITY_SYSTEM, "set device name failed, error code = %x\n", set_dev_name_ret);
        }
        //config adv data
        esp_err_t ret = esp_ble_gap_config_adv_data(&adv_data);
        if (ret) {
            ESP_LOGE(BLE_SECURITY_SYSTEM, "config adv data failed, error code = %x\n", ret);
        }
        adv_config_done |= adv_config_flag;
        //config scan response data
        ret = esp_ble_gap_config_adv_data(&scan_rsp_data);
        if (ret) {
            ESP_LOGE(BLE_SECURITY_SYSTEM, "config scan response data failed, error code = %x\n", ret);
        }
        adv_config_done |= scan_rsp_config_flag;

        esp_ble_gatts_create_service(gatts_if, &gatts_profile_tab[GATTS_PROFILE_A_APP_ID].service_id, GATTS_NUM_HANDLE_TEST_A);
        break;
    case ESP_GATTS_READ_EVT: {
        ESP_LOGI(BLE_SECURITY_SYSTEM, "GATT_READ_EVT, conn_id %d, trans_id %d, handle %d\n", param->read.conn_id, param->read.trans_id, param->read.handle);
        esp_gatt_rsp_t rsp;
        memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
        rsp.attr_value.handle = param->read.handle;
        rsp.attr_value.len = 1;
        // check for known mac addresses in sensors and the decrement value for last connection
        if (record_sensor(param->read.bda) == 0){
            rsp.attr_value.value[0] = *security_state;
        }else{
            rsp.attr_value.value[0] = 5;
        }
        esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ESP_GATT_OK, &rsp);
        break;
    }
    case ESP_GATTS_MTU_EVT:
        ESP_LOGI(BLE_SECURITY_SYSTEM, "ESP_GATTS_MTU_EVT, MTU %d\n", param->mtu.mtu);
        break;
    case ESP_GATTS_CREATE_EVT:
        ESP_LOGI(BLE_SECURITY_SYSTEM, "CREATE_SERVICE_EVT, status %d,  service_handle %d\n", param->create.status, param->create.service_handle);
        gatts_profile_tab[GATTS_PROFILE_A_APP_ID].service_handle = param->create.service_handle;
        gatts_profile_tab[GATTS_PROFILE_A_APP_ID].char_uuid.len = ESP_UUID_LEN_128;
        memcpy(gatts_profile_tab[GATTS_PROFILE_A_APP_ID].char_uuid.uuid.uuid128, service_uuid128, ESP_UUID_LEN_128);

        esp_ble_gatts_start_service(gatts_profile_tab[GATTS_PROFILE_A_APP_ID].service_handle);
        a_property = ESP_GATT_CHAR_PROP_BIT_READ;
        esp_err_t add_char_ret = esp_ble_gatts_add_char(gatts_profile_tab[GATTS_PROFILE_A_APP_ID].service_handle, &gatts_profile_tab[GATTS_PROFILE_A_APP_ID].char_uuid,
                                                        ESP_GATT_PERM_READ,
                                                        a_property,
                                                        &gatts_demo_char1_val, NULL);
        if (add_char_ret) {
            ESP_LOGE(BLE_SECURITY_SYSTEM, "add char failed, error code =%x\n",add_char_ret);
        }
        break;
    case ESP_GATTS_ADD_CHAR_EVT: {
        ESP_LOGI(BLE_SECURITY_SYSTEM, "ADD_CHAR_EVT, status %d,  attr_handle %d, service_handle %d\n",
                param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle);
        gatts_profile_tab[GATTS_PROFILE_A_APP_ID].char_handle = param->add_char.attr_handle;
        gatts_profile_tab[GATTS_PROFILE_A_APP_ID].descr_uuid.len = ESP_UUID_LEN_16;
        gatts_profile_tab[GATTS_PROFILE_A_APP_ID].descr_uuid.uuid.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;

        esp_err_t add_descr_ret = esp_ble_gatts_add_char_descr(gatts_profile_tab[GATTS_PROFILE_A_APP_ID].service_handle, &gatts_profile_tab[GATTS_PROFILE_A_APP_ID].descr_uuid,
                                                                ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, NULL, NULL);
        if (add_descr_ret) {
            ESP_LOGE(BLE_SECURITY_SYSTEM, "add char descr failed, error code =%x\n", add_descr_ret);
        }
        break;
    }
    case ESP_GATTS_ADD_CHAR_DESCR_EVT:
        gatts_profile_tab[GATTS_PROFILE_A_APP_ID].descr_handle = param->add_char_descr.attr_handle;
        ESP_LOGI(BLE_SECURITY_SYSTEM, "ADD_DESCR_EVT, status %d, attr_handle %d, service_handle %d\n",
                 param->add_char_descr.status, param->add_char_descr.attr_handle, param->add_char_descr.service_handle);
        break;
    case ESP_GATTS_START_EVT:
        ESP_LOGI(BLE_SECURITY_SYSTEM, "SERVICE_START_EVT, status %d, service_handle %d\n",
                 param->start.status, param->start.service_handle);
        break;
    case ESP_GATTS_CONNECT_EVT: {
        
        ESP_LOGI(BLE_SECURITY_SYSTEM, "ESP_GATTS_CONNECT_EVT, conn_id %d, remote %02x:%02x:%02x:%02x:%02x:%02x\n",
                 param->connect.conn_id,
                 param->connect.remote_bda[0], param->connect.remote_bda[1], param->connect.remote_bda[2],
                 param->connect.remote_bda[3], param->connect.remote_bda[4], param->connect.remote_bda[5]);
        gatts_profile_tab[GATTS_PROFILE_A_APP_ID].conn_id = param->connect.conn_id;
        esp_ble_set_encryption(param->connect.remote_bda, ESP_BLE_SEC_ENCRYPT_MITM);
        esp_ble_gap_start_advertising(&adv_params);
        break;
    }
    case ESP_GATTS_DISCONNECT_EVT:
        ESP_LOGI(BLE_SECURITY_SYSTEM, "ESP_GATTS_DISCONNECT_EVT, disconnect reason 0x%x\n", param->disconnect.reason);
        esp_ble_gap_start_advertising(&adv_params);
        break;
    case ESP_GATTS_CONF_EVT:
        ESP_LOGI(BLE_SECURITY_SYSTEM, "ESP_GATTS_CONF_EVT, status %d attr_handle %d\n", param->conf.status, param->conf.handle);
        if (param->conf.status != ESP_GATT_OK) {
            esp_log_buffer_hex(BLE_SECURITY_SYSTEM, param->conf.value, param->conf.len);
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
            ESP_LOGI(BLE_SECURITY_SYSTEM, "Reg app failed, app_id %04x, status %d\n",
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

void app_main(){
    esp_err_t ret;
    nvs_handle_t nvs_handle;

    *security_state = Disarmed;

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    netbiosns_init();
    netbiosns_set_name(CONFIG_MDNS_HOST_NAME);

    // load alarm code, wifi crendentials, sensor addresses from non-volatile memory
    ret = nvs_open("storage", NVS_READONLY, &nvs_handle);
    if (ret != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(ret));
    } else {
        int64_t code_int = 0;
        printf("Reading code from NVS ... ");
        ret = nvs_get_i64(nvs_handle, "code", &code_int);
        switch (ret) {
            case ESP_OK:
                printf("Done\n");
                itoa(code_int, expected_code, 10);
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                printf("Code is not initialized yet!\n");
                break;
            default :
                printf("Error (%s) reading!\n", esp_err_to_name(ret));
        }
        printf("Reading wifi credentials from NVS ... ");
        unsigned int str_len = 32;
        ret = nvs_get_str(nvs_handle, "ssid", wifi_ssid, &str_len);
        switch (ret) {
            case ESP_OK:
                printf("SSID Done...");
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                printf("SSID is not initialized yet!\n");
                break;
            default :
                printf("Error (%s) reading!\n", esp_err_to_name(ret));
        }
        str_len *= 2;
        ret = nvs_get_str(nvs_handle, "pass", wifi_pass, &str_len);
        switch (ret) {
            case ESP_OK:
                printf("Pass Done\n");
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                printf("Pass is not initialized yet!\n");
                break;
            default :
                printf("Error (%s) reading!\n", esp_err_to_name(ret));
        }
        printf("Reading sensors addresses from NVS ... ");
        ret = nvs_get_u8(nvs_handle, "sensors_cnt", &number_of_sensors);
        switch (ret) {
            case ESP_OK:
                printf("CNT Done\n");
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                number_of_sensors = 0;
                printf("CNT is not initialized yet!\n");
                break;
            default :
                printf("Error (%s) reading!\n", esp_err_to_name(ret));
        }
        printf("number of sensors: %d\n",number_of_sensors);
        uint64_t number = 0;
        for (uint8_t i = 0; i < number_of_sensors; i++){
            ret = nvs_get_u64(nvs_handle, sensors_nvs_key[i], &number);
            switch (ret) {
                case ESP_OK:
                    for (int8_t j = 5; j >= 0; j--){
                        number = number >> 8;
                        sensors[i].address[j] = number & 255;
                    }
                    printf("%s Done\n", sensors_nvs_key[i]);
                    break;
                case ESP_ERR_NVS_NOT_FOUND:
                    printf("%s is not initialized yet!\n", sensors_nvs_key[i]);
                    break;
                default :
                    printf("Error (%s) reading!\n", esp_err_to_name(ret));
            }
        }
        nvs_close(nvs_handle);
    }

    nvs_stats_t nvs_stats;
    nvs_get_stats(NULL, &nvs_stats);
    printf("Count: UsedEntries = (%d), FreeEntries = (%d), AllEntries = (%d)\n",
            nvs_stats.used_entries, nvs_stats.free_entries, nvs_stats.total_entries);

    // init rest server
    ESP_ERROR_CHECK(wifi_connect());
    ESP_ERROR_CHECK(start_rest_server());

    // init BLE
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(BLE_SECURITY_SYSTEM, "%s initialize controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(BLE_SECURITY_SYSTEM, "%s enable controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(BLE_SECURITY_SYSTEM, "%s init bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(BLE_SECURITY_SYSTEM, "%s enable bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    // register the  callback function to the gap module
    ret = esp_ble_gap_register_callback(esp_gap_cb);
    if (ret){
        ESP_LOGE(BLE_SECURITY_SYSTEM, "%s gap register error, error code = %x\n", __func__, ret);
        return;
    }

    // gattc register
    // register the callback function to the gattc module
    ret = esp_ble_gattc_register_callback(esp_gattc_cb);
    if(ret){
        ESP_LOGE(BLE_SECURITY_SYSTEM, "%s gattc register error, error code = %x\n", __func__, ret);
        return;
    }

    ret = esp_ble_gattc_app_register(PROFILE_A_APP_ID);
    if (ret){
        ESP_LOGE(BLE_SECURITY_SYSTEM, "%s gattc app register error, error code = %x\n", __func__, ret);
    }

    ret = esp_ble_gatt_set_local_mtu(200);
    if (ret){
        ESP_LOGE(BLE_SECURITY_SYSTEM, "set local  MTU failed, error code = %x", ret);
    }

    /* set the security iocap & auth_req & key size & init key response key parameters to the stack*/
    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_MITM_BOND;     //bonding with peer device after authentication
    esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;           //set the IO capability to No output No input
    uint8_t key_size = 16;      //the key size should be 7~16 bytes
    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t oob_support = ESP_BLE_OOB_DISABLE;
    uint32_t passkey = 123456;
    uint8_t auth_option = ESP_BLE_ONLY_ACCEPT_SPECIFIED_AUTH_DISABLE;
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_STATIC_PASSKEY, &passkey, sizeof(uint32_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_ONLY_ACCEPT_SPECIFIED_SEC_AUTH, &auth_option, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_OOB_SUPPORT, &oob_support, sizeof(uint8_t));
    /* If your BLE device act as a Slave, the init_key means you hope which types of key of the master should distribute to you,
    and the response key means which key you can distribute to the Master;
    If your BLE device act as a master, the response key means you hope which types of key of the slave should distribute to you,
    and the init key means which key you can distribute to the slave. */
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));

    // gatts register
    ret = esp_ble_gatts_register_callback(gatts_event_handler);
    if (ret) {
        ESP_LOGE(BLE_SECURITY_SYSTEM, "gatts register error, error code = %x", ret);
        return;
    }

    ret = esp_ble_gatts_app_register(GATTS_PROFILE_A_APP_ID);
    if (ret) {
        ESP_LOGE(BLE_SECURITY_SYSTEM, "gatts app register error, error code = %x", ret);
        return;
    }

    ret = esp_ble_gatts_app_register(GATTS_PROFILE_B_APP_ID);
    if (ret) {
        ESP_LOGE(BLE_SECURITY_SYSTEM, "gatts app register error, error code = %x", ret);
        return;
    }

    ret = esp_ble_gatts_app_register(GATTS_PROFILE_C_APP_ID);
    if (ret) {
        ESP_LOGE(BLE_SECURITY_SYSTEM, "gatts app register error, error code = %x", ret);
        return;
    }

    // set IO for keypad
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = 1;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    xTaskCreate(hadle_keypad_task, "hadle_keypad_task", 1024*2, NULL, configMAX_PRIORITIES-1, NULL);

    // set output for alarm indications - LED, buzzer
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_13_BIT, // resolution of PWM duty
        .freq_hz = 3000,                      // frequency of PWM signal
        .speed_mode = LEDC_HS_MODE,           // timer mode
        .timer_num = LEDC_HS_TIMER,            // timer index
        .clk_cfg = LEDC_AUTO_CLK,              // Auto select the source clock
    };
    ledc_timer_config(&ledc_timer);
    ledc_timer.speed_mode = LEDC_HS_MODE;
    ledc_timer.timer_num = LEDC_HS_TIMER;
    ledc_timer_config(&ledc_timer);

    for (int ch = 0; ch < LEDC_CH_NUM; ch++) {
        ledc_channel_config(&ledc_channel[ch]);
    }
    ledc_fade_func_install(0);

    xTaskCreate(increment_sensor_beeps_task, "increment_beeps", 1024*2, NULL, configMAX_PRIORITIES-1, &xHandle_increment_beeps);

    ESP_LOGI("app-main", "inicialization end");
}
