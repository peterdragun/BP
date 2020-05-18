/**
* @file  sensors.c
*
* @brief Operations with sensors
* @author Peter Dragun (xdragu01)
*/

#include "sensors.h"

uint8_t unknown_sensor[6];
uint8_t unknown_sensor_type;
sensor_t sensors[MAX_NUMBER_OF_SENSORS];
uint8_t number_of_sensors = 0;
TaskHandle_t xHandle_increment;
const char sensors_nvs_key[5][3] = {"s1\0", "s2\0", "s3\0", "s4\0", "s5\0" };

esp_err_t compare_uint8_array(uint8_t *uuid1, uint8_t *uuid2, uint8_t size){
    for (int i = 0; i < size; i++){
        if(uuid1[i] != uuid2[i]){
            return ESP_FAIL;
        }
    }
    return ESP_OK;
}

int find_idx(uint8_t *address){
    for (int i = 0; i < number_of_sensors; i++){
        if(compare_uint8_array(sensors[i].address, address, 6) == ESP_OK){
            ESP_LOGI(SENSORS_TAG, "Index: %d\n", i);
            return i;
        }
    }
    return -1;
}

esp_err_t record_sensor(uint8_t *address){
    int idx = find_idx(address);
    if (idx < 0){
        for (int i = 0; i < 6; i++){
            unknown_sensor[i] = address[i];
        }
        return ESP_FAIL;
    }
    if(sensors[idx].missed_beeps >= 1 && not_responding() <= 1){
        for (int ch = 1; ch < LEDC_CH_NUM; ch++) {
            ledc_set_duty(ledc_channel[ch].speed_mode, ledc_channel[ch].channel, 0);
            ledc_update_duty(ledc_channel[ch].speed_mode, ledc_channel[ch].channel);
        }
    }
    sensors[idx].missed_beeps = 0;
    time(&sensors[idx].last_connection);
    return ESP_OK;
}

esp_err_t add_new_sensor(uint8_t *address){
    if (number_of_sensors == MAX_NUMBER_OF_SENSORS){
        return ESP_FAIL;
    }
    if (find_idx(address) != -1){
        return ESP_FAIL;
    }
    nvs_handle_t nvs_handle;
    uint64_t number = 0;
    esp_err_t ret = nvs_open("storage", NVS_READWRITE, &nvs_handle);
    if (ret == ESP_OK) {
        ESP_LOGI(SENSORS_TAG, "Writing sensor address to NVS memory... ");
        number |= unknown_sensor_type;
        number = number << 8;
        sensors[number_of_sensors].type = unknown_sensor_type;
        for (uint8_t i = 0; i < 6; i++){
            printf("%x\n", address[i]);
            number |= address[i];
            number = number << 8;
            sensors[number_of_sensors].address[i] = address[i];
            unknown_sensor[i] = 0;
        }
        ret = nvs_set_u64(nvs_handle, sensors_nvs_key[number_of_sensors], number);
        if(ret == ESP_OK){
            nvs_set_u8(nvs_handle, "sensors_cnt", number_of_sensors+1);
            nvs_commit(nvs_handle);
        }
        nvs_close(nvs_handle);
        ESP_LOGI(SENSORS_TAG, "Done");
    }
    sensors[number_of_sensors].last_connection = 0;
    sensors[number_of_sensors].missed_beeps = 0;
    sensors[number_of_sensors].last_alarm = 0;
    number_of_sensors++;
    return ESP_OK;
}

esp_err_t remove_sensor(uint8_t *address){
    int idx = find_idx(address);
    if(idx < 0){
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

esp_err_t record_alarm(uint8_t *address){
    int idx = find_idx(address);
    if(idx < 0){
        return ESP_FAIL;
    }
    time(&last_alarm);
    sensors[idx].last_alarm = last_alarm;
    return ESP_OK;
}

int not_responding(){
    int cnt = 0;
    for(int i = 0; i < number_of_sensors; i++){
        if (sensors[i].missed_beeps > 0){
            cnt++;
        }
    }
    return cnt;
}

void increment_sensor_task(){
    uint8_t wait_time;
    time_t now;
    while (1) {
        time(&now);
        wait_time = DISARMED_WAIT;
        if(*security_state > Activating){
            wait_time = ARMED_WAIT;
        }
        ESP_LOGI("incrementing task", "start");
        for(uint8_t i = 0; i < number_of_sensors; i++){
            if(now - sensors[i].last_connection > wait_time + 2*REPEAT_WAIT){
                sensors[i].missed_beeps = 1;
                ESP_LOGE("incrementing task", "senosor is not responding!!!");
                if(*security_state < Activating){
                    ledc_set_duty(ledc_channel[2].speed_mode, ledc_channel[2].channel, LEDC_DUTY);
                    ledc_update_duty(ledc_channel[2].speed_mode, ledc_channel[2].channel);
                }else if (*security_state == Armed){
                    ESP_LOGI(SECURITY_SYSTEM, "Alarm");
                    *security_state = Alarm;
                    vTaskDelete(xHandle_search);
                    xTaskCreate(alarm_task, "alarm_task", 1024*2, NULL, configMAX_PRIORITIES-1, &xHandle_alarm);
                }
            }
        }
        vTaskDelay(wait_time*1000 / portTICK_PERIOD_MS);
    }
}
