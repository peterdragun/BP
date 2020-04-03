#include "rest_server.h"

extern char expected_code[19];

/* Send HTTP response with the contents of the requested file */
static esp_err_t rest_common_get_handler(httpd_req_t *req){
    if (*security_state != Disarmed && *security_state != Setup){
        char * err_msg = "Security system needs to be in setup or disarmed mode for this operation.";
        ESP_LOGW(SECURITY_SYSTEM, "Security system needs to be in setup or disarmed mode for this operation.");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, err_msg);
        return ESP_FAIL;
    }
    return ESP_OK;
}

/* Simple handler for getting system handler */
static esp_err_t system_info_get_handler(httpd_req_t *req){
    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateObject();
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    cJSON_AddStringToObject(root, "version", IDF_VER);
    cJSON_AddNumberToObject(root, "cores", chip_info.cores);
    const char *sys_info = cJSON_Print(root);
    httpd_resp_sendstr(req, sys_info);
    free((void *)sys_info);
    cJSON_Delete(root);
    return ESP_OK;
}

/* Simple handler for getting BLE scan results*/
static esp_err_t ble_scan_get_handler(httpd_req_t *req){
    httpd_resp_set_type(req, "application/json");
    json_resp = cJSON_CreateArray();
    // start scanning for near devices
    ESP_LOGI("rest-ble-scan", "Start scan");
    uint32_t duration = 5; // in seconds
    esp_ble_gap_start_scanning(duration);
    sleep(5);
    const char *scan = cJSON_Print(json_resp);
    httpd_resp_sendstr(req, scan);
    ESP_LOGI(REST_TAG,"resp: %s", scan);
    free((void *)scan);
    ESP_LOGI(REST_TAG,"ble/scan");
    cJSON_Delete(json_resp);
    return ESP_OK;
}

static esp_err_t list_device_get_handler(httpd_req_t *req){
    int bond_cnt = esp_ble_get_bond_device_num();
    esp_ble_bond_dev_t list[bond_cnt];
    ESP_LOGI(REST_TAG,"list length: %d", bond_cnt);
    esp_ble_get_bond_device_list(&bond_cnt, list);
    char address[20];
    uint8_t *a;
    cJSON *json_list = cJSON_CreateArray();
    cJSON *json_obj;
    for (int i = 0; i < bond_cnt; i++){
        a = list[i].bd_addr;
        sprintf(address, "%02x %02x %02x %02x %02x %02x", a[0], a[1], a[2], a[3], a[4], a[5]);
        json_obj = cJSON_CreateObject();
        cJSON_AddStringToObject(json_obj, "address", (const char*)address);
        cJSON_AddItemToArray(json_list, json_obj);
    }
    const char *scan = cJSON_Print(json_list);
    httpd_resp_sendstr(req, scan);
    ESP_LOGI(REST_TAG,"resp: %s", scan);
    free((void *)scan);
    cJSON_Delete(json_list);
    return ESP_OK;
}

static esp_err_t remove_device_post_handler(httpd_req_t *req){
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len) {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0) {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';
    cJSON *root = cJSON_Parse(buf);
    char* address_str = cJSON_GetObjectItem(root, "address")->valuestring;
    esp_bd_addr_t address_hex;
    char str[3] = "\0\0\0";
    ESP_LOGI(REST_TAG, "address to be removed: %s", address_str);
    for (int i = 0; i < ESP_BD_ADDR_LEN; i++){
        str[0] = address_str[0];
        str[1] = address_str[1];
        address_hex[i] = (uint8_t)strtol(str, NULL, 16);
        address_str = address_str + 2;
        ESP_LOGI(REST_TAG, "address to be removed: %s hex: %x", str, address_hex[i]);
    }
    esp_ble_remove_bond_device(address_hex);
    return ESP_OK;
}

static esp_err_t add_device_post_handler(httpd_req_t *req){
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len) {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0) {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    cJSON *root = cJSON_Parse(buf);

    // TODO hadle not presented object names
    char* name = cJSON_GetObjectItem(root, "name")->valuestring;
    char* characteristic = cJSON_GetObjectItem(root, "char")->valuestring;
    char* gatt = cJSON_GetObjectItem(root, "gatt")->valuestring;
    ESP_LOGI(REST_TAG, "Connecting to: name = %s, characteristic = %s, gatt = %s", name, characteristic, gatt);

    // TODO connect to device
    // start new scan?
    // maybe esp_ble_gattc_open() ? 
    // esp_ble_gap_update_whitelist(true, "address", BLE_WL_ADDR_TYPE_RANDOM);
    cJSON_Delete(root);
    httpd_resp_sendstr(req, "Device was successfully added to whitelist");
    return ESP_OK;
}

static esp_err_t change_code_post_handler(httpd_req_t *req){
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len) {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0) {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    cJSON *root = cJSON_Parse(buf);
    char* code = cJSON_GetObjectItem(root, "code")->valuestring;
    char* new_code = cJSON_GetObjectItem(root, "new_code")->valuestring;

    if (strcmp(code, expected_code)){
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Code does not match.");
        return ESP_FAIL;
    }

    if (strlen(code) > 18){
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "New code is too long. Max 20 numbers.");
        return ESP_FAIL;
    }
    
    char * ptr;
    int64_t code_int = strtol(new_code, &ptr, 10);
    if(*ptr != '\0'){
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "New code have to contain only numbers.");
        return ESP_FAIL;
    }

    // store code do non-volatile memory
    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open("storage", NVS_READWRITE, &nvs_handle);
    if (ret != ESP_OK) {
        cJSON_Delete(root);
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(ret));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Error while opening nvs.");
        return ESP_FAIL;
    } else {
        ESP_LOGI(REST_TAG, "Writing code to NVS memory... ");
        ret = nvs_set_i64(nvs_handle, "code", code_int);
        if(ret == ESP_OK){
            ret = nvs_commit(nvs_handle);
        }
        nvs_close(nvs_handle);
        ESP_LOGI(REST_TAG, "Done");
    }

    if(ret != ESP_OK){
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Error while writing to nvs.");
    }else{
        // store code also to variable
        strncpy(expected_code, new_code, 19);
        httpd_resp_sendstr(req, "Code was successfully changed");
        ESP_LOGI(REST_TAG, "Code was successfully changed to %s", expected_code);
    }
    cJSON_Delete(root);
    return ret;
}

static esp_err_t system_arm_get_handler(httpd_req_t *req){
    esp_err_t ret = arm_system();
    if (ret == ESP_FAIL){
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "System failed to activate.");
    }else{
        httpd_resp_sendstr(req, "System will be activated in 30 seconds");
    }
    return ret;
}

esp_err_t start_rest_server(const char *base_path){
    REST_CHECK(base_path, "Wrong base path", err);
    rest_server_context_t *rest_context = calloc(1, sizeof(rest_server_context_t));
    REST_CHECK(rest_context, "No memory for rest context", err);
    strlcpy(rest_context->base_path, base_path, sizeof(rest_context->base_path));

    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;

    ESP_LOGI(REST_TAG, "Starting HTTP Server");
    REST_CHECK(httpd_start(&server, &config) == ESP_OK, "Start server failed", err_start);

    /* URI handler for fetching system info */
    httpd_uri_t system_info_get_uri = {
        .uri = "/api/v1/system/info",
        .method = HTTP_GET,
        .handler = system_info_get_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &system_info_get_uri);

    /* URI handler for getting web server files */
    httpd_uri_t scan_get_uri = {
        .uri = "/ble/scan",
        .method = HTTP_GET,
        .handler = ble_scan_get_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &scan_get_uri);

    httpd_uri_t ble_add_device_post_uri = {
        .uri = "/ble/device/add",
        .method = HTTP_POST,
        .handler = add_device_post_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &ble_add_device_post_uri);

    httpd_uri_t ble_remove_device_post_uri = {
        .uri = "/ble/device/remove",
        .method = HTTP_POST,
        .handler = remove_device_post_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &ble_remove_device_post_uri);
    
    httpd_uri_t ble_list_device_get_uri = {
        .uri = "/ble/device/list",
        .method = HTTP_GET,
        .handler = list_device_get_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &ble_list_device_get_uri);

    httpd_uri_t change_code_post_uri = {
        .uri = "/code/change",
        .method = HTTP_POST,
        .handler = change_code_post_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &change_code_post_uri);

    httpd_uri_t system_arm_get_uri = {
        .uri = "/system/arm",
        .method = HTTP_GET,
        .handler = system_arm_get_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &system_arm_get_uri);

    /* URI handler for getting web server files */
    httpd_uri_t common_get_uri = {
        .uri = "/*",
        .method = HTTP_GET,
        .handler = rest_common_get_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &common_get_uri);

    return ESP_OK;
err_start:
    free(rest_context);
err:
    return ESP_FAIL;
}
