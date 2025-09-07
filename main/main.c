// do am kk, temp kk, do am dat, mua.
// bom, led

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_log.h"
#include "tbc_mqtt_helper.h"
#include "protocol_examples_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"



static const char *TAG = "SYSTEM ";

uint8_t te_temp;
uint8_t te_hum;
uint8_t te_soil;
uint8_t te_rain;
uint8_t te_pump;
uint8_t te_led;
uint8_t at_pump;
uint8_t at_led;
uint8_t req_pump;
uint8_t req_led;

bool update_stm = false;
//********************************   UART DEFINE  **********************************//
#define ECHO_TEST_TXD (CONFIG_EXAMPLE_UART_TXD)
#define ECHO_TEST_RXD (CONFIG_EXAMPLE_UART_RXD)
#define ECHO_TEST_RTS (UART_PIN_NO_CHANGE)
#define ECHO_TEST_CTS (UART_PIN_NO_CHANGE)
#define ECHO_UART_PORT_NUM      (CONFIG_EXAMPLE_UART_PORT_NUM)
#define ECHO_UART_BAUD_RATE     (CONFIG_EXAMPLE_UART_BAUD_RATE)
#define ECHO_TASK_STACK_SIZE    (CONFIG_EXAMPLE_TASK_STACK_SIZE)
#define BUF_SIZE (1024)
//********************************   TELEMETRY DEFINE  **********************************//

#define TELEMETRY_TEMPERATURE         	"temperature"
#define TELEMETRY_HUMIDITY          	"humidity"
#define TELEMETRY_SOIL                  "soil moisture"
#define TELEMETRY_RAIN                  "rain"
#define TELEMETRY_PUMP                  "pump"
#define TELEMETRY_LED                   "led"


//********************************   ATT_REQUEST_&_UPDATE DEFINE  **********************************//
#define CLIENTATTRIBUTE_PUMP    	    "pump"
#define SHAREDATTRIBUTE_SNTP_SERVER     "sntp_server"
#define CLIENTATTRIBUTE_LED       	    "led"

#define SERVER_RPC_PUMP                 "pump_request"
#define SERVER_RPC_LED                  "led_request"


tbcmh_value_t* te_get_temp(void)
{
    ESP_LOGI(TAG, "Get temperature");
    cJSON* temp = cJSON_CreateNumber(te_temp);
    return temp;
}

tbcmh_value_t* te_get_hum(void)
{
    ESP_LOGI(TAG, "Get humidity");
    cJSON* humi = cJSON_CreateNumber(te_hum);
    return humi;
}

tbcmh_value_t* te_get_rain(void)
{
    ESP_LOGI(TAG, "Get rain sensor");
    cJSON* rainn = cJSON_CreateNumber(te_rain);
    return rainn;
}

tbcmh_value_t* te_get_soil(void)
{
    ESP_LOGI(TAG, "Get soil moisture");
    cJSON* soill = cJSON_CreateNumber(te_soil);
    return soill;
}
/*
tbcmh_value_t* te_get_pump(void)
{
    ESP_LOGI(TAG, "Get pump status");
    cJSON* pumpp = cJSON_CreateNumber(te_pump);
    return pumpp;
}

tbcmh_value_t* te_get_led(void)
{
    ESP_LOGI(TAG, "Get led status");
    cJSON* ledd = cJSON_CreateNumber(te_led);
    return ledd;
}
*/
void tb_telemetry_send(tbcmh_handle_t client)
{
    ESP_LOGI(TAG, "Send telemetry: %s, %s, %s, %s, %s, %s", TELEMETRY_TEMPERATURE, TELEMETRY_HUMIDITY, TELEMETRY_SOIL, TELEMETRY_RAIN, TELEMETRY_LED, TELEMETRY_PUMP);

    cJSON *object = cJSON_CreateObject();
    cJSON_AddItemToObject(object, TELEMETRY_TEMPERATURE, te_get_temp());
    cJSON_AddItemToObject(object, TELEMETRY_HUMIDITY, te_get_hum());
    cJSON_AddItemToObject(object, TELEMETRY_RAIN, te_get_rain());
    cJSON_AddItemToObject(object, TELEMETRY_SOIL, te_get_soil());
    //cJSON_AddItemToObject(object, TELEMETRY_LED, te_get_led());
    //cJSON_AddItemToObject(object, TELEMETRY_PUMP, te_get_pump());
    tbcmh_telemetry_upload_ex(client, object, 1/*qos*/, 0/*retain*/);
    cJSON_Delete(object);
}


//****************************************** **************************************************************//
/*
void tb_attributesrequest_on_response(tbcmh_handle_t client,
                             void *context,
                             const cJSON *client_attributes,
                             const cJSON *shared_attributes)
{
    ESP_LOGI(TAG, "Receiving response of the attributes request!"); 

    if (client_attributes) {
        char *pack = cJSON_PrintUnformatted(client_attributes); 
        ESP_LOGI(TAG, "client_attributes: %s", pack);    

        cJSON *root = cJSON_Parse(pack);
        if((root != NULL)) {
            cJSON *pumpValue = cJSON_GetObjectItem(root, "pump");
            if ((pumpValue != NULL)&&(pumpValue->valuedouble != at_pump)) {
                at_pump = pumpValue->valuedouble;
                if(at_pump != at_pump_pre)  update_stm = true;
                ESP_LOGI(TAG, "pump attribute: %d", at_pump);
            } 

            cJSON *ledValue = cJSON_GetObjectItem(root, "led");
            if ((ledValue != NULL)&&(ledValue->valuedouble != at_led)) {
                at_led = ledValue->valuedouble;
                if(at_led != at_led_pre)  update_stm = true;
                ESP_LOGI(TAG, "led attribute: %d", at_led);
            } 

        }
        cJSON_Delete(root);
        cJSON_free(pack);
    }

    if (shared_attributes) {
        char *pack = cJSON_PrintUnformatted(shared_attributes); 
        ESP_LOGI(TAG, "shared_attributes: %s", pack);        
        cJSON_free(pack); 
    }

    at_pump_pre = at_pump;
    at_led_pre = at_led;
}

void tb_attributesrequest_on_timeout(tbcmh_handle_t client, void *context) 
{
    ESP_LOGI(TAG, "Timeout of the attributes request!"); 
}

void tb_attributesrequest_send(tbcmh_handle_t client)
{
    ESP_LOGI(TAG, "Request attributes, client attributes: %s; client attributes: %s",
        CLIENTATTRIBUTE_PUMP, CLIENTATTRIBUTE_LED);

    tbcmh_attributes_request(client, NULL,
                             tb_attributesrequest_on_response,
                             tb_attributesrequest_on_timeout,
                             CLIENTATTRIBUTE_PUMP, SHAREDATTRIBUTE_SNTP_SERVER);
    tbcmh_attributes_request(client, NULL,
                             tb_attributesrequest_on_response,
                             tb_attributesrequest_on_timeout,
                             CLIENTATTRIBUTE_LED, SHAREDATTRIBUTE_SNTP_SERVER);

}


*/

//************************************************* *****************************************************//

tbcmh_rpc_results_t *tb_serverrpc_on_request_led(tbcmh_handle_t client,
                            void *context, uint32_t request_id, 
                            const char *method, const tbcmh_rpc_params_t *params)
{
    if (!client || !method || !params) {
        ESP_LOGW(TAG, "client, method or params is NULL in %s()!", __FUNCTION__);
        return NULL;
    }

    cJSON *value_led = cJSON_GetObjectItem(params, "LED");
    if (!value_led) {
        return NULL;
    }
    

    if (cJSON_IsNumber(value_led)) {
        req_led = cJSON_GetNumberValue(value_led);
        update_stm = true;
    } else {
        ESP_LOGW(TAG, "Receive setpoint is NOT a number!");
    }

    return NULL;
}


tbcmh_rpc_results_t *tb_serverrpc_on_request_pump(tbcmh_handle_t client,
                            void *context, uint32_t request_id, 
                            const char *method, const tbcmh_rpc_params_t *params)
{
    if (!client || !method || !params) {
        ESP_LOGW(TAG, "client, method or params is NULL in %s()!", __FUNCTION__);
        return NULL;
    }

    cJSON *value_pump = cJSON_GetObjectItem(params, "PUMP");
    if (!value_pump) {
        return NULL;
    }

    if (cJSON_IsNumber(value_pump)) {
        req_pump = cJSON_GetNumberValue(value_pump);
        update_stm = true;
    } else {
        ESP_LOGW(TAG, "Receive setpoint is NOT a number!");
    }

    return NULL;
}
//************************************************* *****************************************************//

tbcmh_value_t* tb_clientattribute_on_get_pump(void *context)
{
    ESP_LOGI(TAG, "Get pump status");
    
    return cJSON_CreateNumber(1 - at_pump);
}

tbcmh_value_t* tb_clientattribute_on_get_led(void *context)
{
    ESP_LOGI(TAG, "Get led status");
    
    return cJSON_CreateNumber(1 - at_led);
}

void tb_clientattribute_send(tbcmh_handle_t client)
{
    ESP_LOGI(TAG, "Update attributes: %s, %s",CLIENTATTRIBUTE_PUMP, CLIENTATTRIBUTE_LED);

    cJSON *object = cJSON_CreateObject(); // create json object
    cJSON_AddItemToObject(object, CLIENTATTRIBUTE_PUMP, tb_clientattribute_on_get_pump(NULL));
    cJSON_AddItemToObject(object, CLIENTATTRIBUTE_LED, tb_clientattribute_on_get_led(NULL));
    tbcmh_attributes_update_ex(client, object, 1/*qos*/, 0/*retain*/);
    cJSON_Delete(object); // delete json object
}

void tb_on_connected(tbcmh_handle_t client, void *context)
{
    ESP_LOGI(TAG, "Connected to thingsboard server!");
}

void tb_on_disconnected(tbcmh_handle_t client, void *context)
{
    ESP_LOGI(TAG, "Disconnected from thingsboard server!");
}



static void mqtt_main(void)
{
	tbc_err_t err;
    const char *access_token = CONFIG_ACCESS_TOKEN;
    const char *uri = CONFIG_BROKER_URL;


    ESP_LOGI(TAG, "Init tbcmh ...");
    tbcmh_handle_t client = tbcmh_init();
    if (!client) {
        ESP_LOGE(TAG, "Failure to init tbcmh!");
        return;
    }
    err = tbcmh_serverrpc_subscribe(client, SERVER_RPC_PUMP, NULL,
                                 tb_serverrpc_on_request_pump);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "failure to append server RPC: %s!", SERVER_RPC_PUMP);
        goto exit_destroy;
    }

    err = tbcmh_serverrpc_subscribe(client, SERVER_RPC_LED, NULL,
                                 tb_serverrpc_on_request_led);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "failure to append server RPC: %s!", SERVER_RPC_LED);
        goto exit_destroy;
    }

    ESP_LOGI(TAG, "Connect tbcmh ...");
    tbc_transport_config_esay_t config = {
        .uri = uri,                     /*!< Complete ThingsBoard MQTT broker URI */
        .access_token = access_token,   /*!< ThingsBoard Access Token */
        .log_rxtx_package = true                /*!< print Rx/Tx MQTT package */
     };
    bool result = tbcmh_connect_using_url(client, &config, 
                        NULL, tb_on_connected, tb_on_disconnected);
    if (!result) {
        ESP_LOGE(TAG, "failure to connect to tbcmh!");
        goto exit_destroy;
    }


    ESP_LOGI(TAG, "connect tbcmh ...");
    int i = 0;
    while(1) {
        if (tbcmh_has_events(client)) {
            tbcmh_run(client);
        }
        
        i++;
        if (tbcmh_is_connected(client)) {
            tb_telemetry_send(client);
            
            if(i%5 == 0)   
            {
                i = 0;
                tb_clientattribute_send(client);
                //tb_attributesrequest_send(client);
            }
            
            
            
            ESP_LOGI(TAG, "data:      temp %d; hum %d; soil %d; rain %d; pump %d; led %d", te_temp, te_hum, te_soil, te_rain, req_pump, req_led);
        } 
        else  ESP_LOGI(TAG, "Still NOT connected to server!");

        
        sleep(1);
    }


    ESP_LOGI(TAG, "Disconnect tbcmh ...");
    tbcmh_disconnect(client);

exit_destroy:
    ESP_LOGI(TAG, "Destroy tbcmh ...");
    tbcmh_destroy(client);
}




//********************************   UART FUNCTION  **********************************//
static void echo_task(void *arg)
{
    uart_config_t uart_config = {
        .baud_rate = ECHO_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    int intr_alloc_flags = 0;

    ESP_ERROR_CHECK(uart_driver_install(ECHO_UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(ECHO_UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(ECHO_UART_PORT_NUM, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS));

    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);
    while (1) {
        int len = uart_read_bytes(ECHO_UART_PORT_NUM, data, (BUF_SIZE - 1), 20 / portTICK_RATE_MS);
        //vTaskDelay(pdMS_TO_TICKS(1000));
        if(len)
        {
            data[len] = '\0';
            ESP_LOGI(TAG, "Recv str: %s", (char *) data);
            for(uint8_t startIndex = 0; startIndex < len; startIndex++)
            {
                if(data[startIndex] == 0x02)
                {
                    te_hum = data[startIndex + 1];
                    te_temp = data[startIndex + 2];
                    te_soil = ((data[startIndex + 3])&0x02)>>1;
                    te_rain = (data[startIndex + 3])&0x01;


                    at_led = ((data[startIndex + 3])&0x04)>>2;
                    at_pump = ((data[startIndex + 3])&0x08)>>3;

                    ESP_LOGI(TAG, "Recv str:   temp %d; hum %d; soil %d; rain %d; pump %d; led %d", te_temp, te_hum, te_soil, te_rain, at_pump, at_led);
                    break;
                }
            }
 
        }
        //vTaskDelay(pdMS_TO_TICKS(100));
        if(update_stm)
        {
            uint8_t buf_tran = ((req_pump << 1)|(req_led))&0x03;
            uart_write_bytes(ECHO_UART_PORT_NUM, &buf_tran, 1);
            update_stm = false;
        }
        
    }
}






void app_main(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO); 
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_BASE", ESP_LOG_VERBOSE);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);

    esp_log_level_set(TAG, ESP_LOG_VERBOSE);
    esp_log_level_set("tb_mqtt_client", ESP_LOG_VERBOSE);
    esp_log_level_set("tb_mqtt_client_helper", ESP_LOG_VERBOSE);
    esp_log_level_set("attributes_reques", ESP_LOG_VERBOSE);
    esp_log_level_set("clientattribute", ESP_LOG_VERBOSE);
    esp_log_level_set("clientrpc", ESP_LOG_VERBOSE);
    esp_log_level_set("otaupdate", ESP_LOG_VERBOSE);
    esp_log_level_set("serverrpc", ESP_LOG_VERBOSE);
    esp_log_level_set("sharedattribute", ESP_LOG_VERBOSE);
    esp_log_level_set("telemetry_upload", ESP_LOG_VERBOSE);

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_ERROR_CHECK(example_connect());

    xTaskCreate(echo_task, "uart_echo_task", ECHO_TASK_STACK_SIZE, NULL, 10, NULL);
    
    mqtt_main();
}

