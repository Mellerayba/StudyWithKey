#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_bt_defs.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_gatt_defs.h"
#include "esp_hidd_prf_api.h"
#include "hid_dev.h"
#include "nvs_flash.h"
#include "esp_sleep.h"
#include "driver/rtc_io.h"

#define KEY_H_GPIO      GPIO_NUM_42   // SW2
#define KEY_LEFT_GPIO   GPIO_NUM_1    // SW3
#define KEY_SPACE_GPIO  GPIO_NUM_40   // SW4
#define KEY_RIGHT_GPIO  GPIO_NUM_38   // SW5
#define KEY_RECONNECT_GPIO GPIO_NUM_0 // SW1

static const char *TAG = "swk";

static uint16_t hid_conn_id = 0;
static bool sec_conn = false;

#define HIDD_DEVICE_NAME "StudyWithKey"
static uint8_t hidd_service_uuid128[] = {
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x12, 0x18, 0x00, 0x00,
};

static esp_ble_adv_data_t hidd_adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = true,
    .min_interval = ESP_BLE_GAP_CONN_ITVL_MS(7.5),
    .max_interval = ESP_BLE_GAP_CONN_ITVL_MS(20),
    .appearance = 0x03c0,
    .manufacturer_len = 0,
    .p_manufacturer_data = NULL,
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = sizeof(hidd_service_uuid128),
    .p_service_uuid = hidd_service_uuid128,
    .flag = 0x6,
};

static esp_ble_adv_params_t hidd_adv_params = {
    .adv_int_min = ESP_BLE_GAP_ADV_ITVL_MS(20),
    .adv_int_max = ESP_BLE_GAP_ADV_ITVL_MS(30),
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

static void hidd_event_callback(esp_hidd_cb_event_t event, esp_hidd_cb_param_t *param)
{
    switch (event) {
        case ESP_HIDD_EVENT_REG_FINISH:
            if (param->init_finish.state == ESP_HIDD_INIT_OK) {
                esp_ble_gap_set_device_name(HIDD_DEVICE_NAME);
                esp_ble_gap_config_adv_data(&hidd_adv_data);
            }
            break;
        case ESP_HIDD_EVENT_BLE_CONNECT:
            ESP_LOGI(TAG, "BLE connect");
            hid_conn_id = param->connect.conn_id;
            break;
        case ESP_HIDD_EVENT_BLE_DISCONNECT:
            sec_conn = false;
            ESP_LOGI(TAG, "BLE disconnect");
            esp_ble_gap_start_advertising(&hidd_adv_params);
            break;
        default:
            break;
    }
}

static void clear_all_bonds(void)
{
    int dev_num = esp_ble_get_bond_device_num();
    if (dev_num == 0) return;

    esp_ble_bond_dev_t *dev_list = malloc(sizeof(esp_ble_bond_dev_t) * dev_num);
    esp_ble_get_bond_device_list(&dev_num, dev_list);
    for (int i = 0; i < dev_num; i++) {
        esp_ble_remove_bond_device(dev_list[i].bd_addr);
    }
    free(dev_list);
    ESP_LOGI(TAG, "Cleared %d bonded device(s)", dev_num);
}

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
            esp_ble_gap_start_advertising(&hidd_adv_params);
            break;
        case ESP_GAP_BLE_SEC_REQ_EVT:
            esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
            break;
        case ESP_GAP_BLE_AUTH_CMPL_EVT:
            ESP_LOGI(TAG, "pair status = %s",
                     param->ble_security.auth_cmpl.success ? "success" : "fail");
            if (param->ble_security.auth_cmpl.success) {
                sec_conn = true;
            }
            break;
        default:
            break;
    }
}



void app_main(void)
{
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));

    esp_bluedroid_config_t bd_cfg = BT_BLUEDROID_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bluedroid_init_with_cfg(&bd_cfg));
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    ESP_ERROR_CHECK(esp_hidd_profile_init());

    esp_ble_gap_register_callback(gap_event_handler);
    esp_hidd_register_callbacks(hidd_event_callback);

    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_BOND;
    esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;
    uint8_t key_size = 16;
    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << KEY_LEFT_GPIO) | (1ULL << KEY_RIGHT_GPIO) | (1ULL << KEY_SPACE_GPIO) | (1ULL << KEY_H_GPIO) | (1ULL << KEY_RECONNECT_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);


    int64_t timeoutLast = 0;
    
    int64_t reconnectLast = 0;

    bool reconnectCheck = false;

    int leftOutput = 1;
    int64_t leftLast = 0;

    int rightOutput = 1;
    int64_t rightLast = 0;
    //Individual trackers for the 4 buttons
    int spaceOutput = 1;
    int64_t spaceLast = 0;

    int hOutput = 1;
    int64_t hLast = 0;
    ESP_LOGI(TAG, "Stage 3: watching all, with debouncing");

    while (1) {
        //Reconnect key check
        int reconnectLevel =  gpio_get_level(KEY_RECONNECT_GPIO);
        if(reconnectCheck){
            if(reconnectLevel == 0){
                if(esp_timer_get_time() > reconnectLast + 3000000){
                    clear_all_bonds();
                    reconnectCheck = false;
                }
            }
            else{
                reconnectCheck = false;
            }
        }
        if (reconnectLevel == 0 && reconnectCheck == false){
                reconnectCheck = true;
                reconnectLast = esp_timer_get_time();
            }
        

        vTaskDelay(pdMS_TO_TICKS(10)); //Prevent running at clock speed 
        if(esp_timer_get_time() > timeoutLast + 60000000){
            rtc_gpio_pullup_en(GPIO_NUM_1);
            rtc_gpio_pulldown_dis(GPIO_NUM_1);
            esp_sleep_enable_ext0_wakeup(GPIO_NUM_1, 0);
            esp_deep_sleep_start();
        }
        //Left key check
        int Leftlevel = gpio_get_level(KEY_LEFT_GPIO);
        if (!(Leftlevel == leftOutput || esp_timer_get_time()-leftLast < 30000)) {
            leftOutput = Leftlevel;
            if(leftOutput == 0){
                ESP_LOGI(TAG, "Left button pressed");
                if (sec_conn) {
                uint8_t key_code = HID_KEY_LEFT_ARROW;
                esp_hidd_send_keyboard_value(hid_conn_id, 0, &key_code, 1);
                vTaskDelay(100 / portTICK_PERIOD_MS);
                esp_hidd_send_keyboard_value(hid_conn_id, 0, NULL, 0); // Release key
                }
            }
            timeoutLast = esp_timer_get_time();
            leftLast = esp_timer_get_time();
        }


        //Right key check
        int Rightlevel = gpio_get_level(KEY_RIGHT_GPIO);
        if (!(Rightlevel == rightOutput || (esp_timer_get_time()-rightLast < 30000))) {
            rightOutput = Rightlevel;
            if(rightOutput == 0){
                ESP_LOGI(TAG, "Right button pressed");
                if (sec_conn) {
                uint8_t key_code = HID_KEY_RIGHT_ARROW;
                esp_hidd_send_keyboard_value(hid_conn_id, 0, &key_code, 1);
                vTaskDelay(100 / portTICK_PERIOD_MS);
                esp_hidd_send_keyboard_value(hid_conn_id, 0, NULL, 0); // Release key
                }
            }
            rightLast = esp_timer_get_time();
            timeoutLast = esp_timer_get_time();
        }



        //Space key check
        int Spacelevel = gpio_get_level(KEY_SPACE_GPIO);
        if (!(Spacelevel == spaceOutput || (esp_timer_get_time()-spaceLast < 30000))) {
            spaceOutput = Spacelevel;
            if(spaceOutput == 0){
                ESP_LOGI(TAG, "Space button pressed");
                if (sec_conn) {
                uint8_t key_code = HID_KEY_SPACEBAR;
                esp_hidd_send_keyboard_value(hid_conn_id, 0, &key_code, 1);
                vTaskDelay(100 / portTICK_PERIOD_MS);
                esp_hidd_send_keyboard_value(hid_conn_id, 0, NULL, 0); // Release key
                }
            }
            spaceLast = esp_timer_get_time();
            timeoutLast = esp_timer_get_time();
        }

        //H key check
        int Hlevel = gpio_get_level(KEY_H_GPIO);
        if (!(Hlevel == hOutput || (esp_timer_get_time()-hLast < 30000))){
            hOutput = Hlevel;
            if(hOutput == 0){
                ESP_LOGI(TAG, "H button pressed");
                if (sec_conn) {
                uint8_t key_code = HID_KEY_H;
                esp_hidd_send_keyboard_value(hid_conn_id, 0, &key_code, 1);
                vTaskDelay(100 / portTICK_PERIOD_MS);
                esp_hidd_send_keyboard_value(hid_conn_id, 0, NULL, 0); // Release key
                }
            }
            hLast = esp_timer_get_time();
            timeoutLast = esp_timer_get_time();
        }

    }
}