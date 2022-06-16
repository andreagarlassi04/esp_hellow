/* ESP HTTP Client Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "esp_tls.h"
#if CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
#include "esp_crt_bundle.h"
#endif

#include "esp_http_client.h"

#define MAX_HTTP_RECV_BUFFER 512
#define MAX_HTTP_OUTPUT_BUFFER 2048
static const char *TAG = "HTTP_CLIENT";

/* Root cert for howsmyssl.com, taken from howsmyssl_com_root_cert.pem

   The PEM file was extracted from the output of this command:
   openssl s_client -showcerts -connect www.howsmyssl.com:443 </dev/null

   The CA root cert is the last cert given in the chain of certs.

   To embed it in the app binary, the PEM file is named
   in the component.mk COMPONENT_EMBED_TXTFILES variable.
*/
extern const char howsmyssl_com_root_cert_pem_start[] asm("_binary_howsmyssl_com_root_cert_pem_start");
extern const char howsmyssl_com_root_cert_pem_end[]   asm("_binary_howsmyssl_com_root_cert_pem_end");

extern const char postman_root_cert_pem_start[] asm("_binary_postman_root_cert_pem_start");
extern const char postman_root_cert_pem_end[]   asm("_binary_postman_root_cert_pem_end");


/* Hello World Example
   This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/


#define BLINK_GPIO4 CONFIG_BLINK_GPIO

static uint8_t s_ledB_state = 0;

#ifdef CONFIG_BLINK_LED_GPIO

static void blink_ledB(void)
{
    /* Set the GPIO level according to the state (LOW or HIGH)*/
    gpio_set_level(4, s_ledB_state);
}

static void configure_ledB(void)
{
    ESP_LOGI(TAG, "Example configured to blink GPIO LED!");
    gpio_reset_pin(4);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(4, GPIO_MODE_OUTPUT);
}

#endif

#define BLINK_GPIO0 CONFIG_BLINK_GPIO

static uint8_t s_ledR_state = 0;

#ifdef CONFIG_BLINK_LED_GPIO

static void blink_ledR(void)
{
    /* Set the GPIO level according to the state (LOW or HIGH)*/
    gpio_set_level(0, s_ledR_state);
}

static void configure_ledR(void)
{
    ESP_LOGI(TAG, "Example configured to blink GPIO LED!");
    gpio_reset_pin(0);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(0, GPIO_MODE_OUTPUT);
}

#endif

#define BLINK_GPIO2 CONFIG_BLINK_GPIO

static uint8_t s_ledG_state = 0;

#ifdef CONFIG_BLINK_LED_GPIO

static void blink_ledG(void)
{
    /* Set the GPIO level according to the state (LOW or HIGH)*/
    gpio_set_level(2, s_ledG_state);
}

static void configure_ledG(void)
{
    ESP_LOGI(TAG, "Example configured to blink GPIO LED!");
    gpio_reset_pin(2);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(2, GPIO_MODE_OUTPUT);
}

#endif

#define BLINK_GPIO26 CONFIG_BLINK_GPIO

static void configure_button(void)
{
    gpio_reset_pin(26);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(26, GPIO_MODE_INPUT);
}

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    static char *output_buffer;  // Buffer to store response of http request from event handler
    static int output_len;       // Stores number of bytes read
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            /*
             *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
             *  However, event handler can also be used in case chunked encoding is used.
             */
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // If user_data buffer is configured, copy the response into the buffer
                if (evt->user_data) {
                    memcpy(evt->user_data + output_len, evt->data, evt->data_len);
                } else {
                    if (output_buffer == NULL) {
                        output_buffer = (char *) malloc(esp_http_client_get_content_length(evt->client));
                        output_len = 0;
                        if (output_buffer == NULL) {
                            ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
                            return ESP_FAIL;
                        }
                    }
                    memcpy(output_buffer + output_len, evt->data, evt->data_len);
                }
                output_len += evt->data_len;
            }

            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            if (output_buffer != NULL) {
                // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
                // ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            int mbedtls_err = 0;
            esp_err_t err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t)evt->data, &mbedtls_err, NULL);
            if (err != 0) {
                ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
                ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
            }
            if (output_buffer != NULL) {
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
            esp_http_client_set_header(evt->client, "From", "user@example.com");
            esp_http_client_set_header(evt->client, "Accept", "text/html");
            break;
    }
    return ESP_OK;
}

static void http_led_state(void)
{
    char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};
    esp_http_client_config_t config = {
        .host = "44.193.166.37",
        .path = "/pwUmyjIHEIdvqIRMVkMr",
        .query = "esp",
        .event_handler = _http_event_handler,
        .user_data = local_response_buffer,       
        .disable_auto_redirect = true,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err;
    const char* ledState="{\"red\":0,\"green\":0,\"blue\":0}";

   /* s_ledR_state=!s_ledR_state;
    blink_ledR();
    configure_ledR();
    s_ledG_state=!s_ledG_state;
    blink_ledG();
    configure_ledG();
    s_ledB_state=!s_ledB_state;
    blink_ledB();
    configure_ledB();*/

    if(!s_ledR_state && !s_ledG_state && !s_ledB_state){
        ledState="{\"red\":0,\"green\":0,\"blue\":0}";
    }
    else if(s_ledR_state && !s_ledG_state && !s_ledB_state){
        ledState="{\"red\":1,\"green\":0,\"blue\":0}";
    }
    else if(!s_ledR_state && s_ledG_state && !s_ledB_state){
        ledState="{\"red\":0,\"green\":1,\"blue\":0}";
    }
    else if(!s_ledR_state && !s_ledG_state && s_ledB_state){
        ledState="{\"red\":0,\"green\":0,\"blue\":1}";
    }
    else if(s_ledR_state && s_ledG_state && s_ledB_state){
        ledState="{\"red\":1,\"green\":1,\"blue\":1}";
    }
    else if(s_ledR_state && s_ledG_state && !s_ledB_state){
        ledState="{\"red\":1,\"green\":1,\"blue\":0}";
    }
    else if(s_ledR_state && !s_ledG_state && s_ledB_state){
        ledState="{\"red\":1,\"green\":0,\"blue\":1}";
    }
    else if(!s_ledR_state && s_ledG_state && s_ledB_state){
        ledState="{\"red\":0,\"green\":1,\"blue\":1}";
    }
    esp_http_client_set_url(client, "http://44.193.166.37/api/v1/pwUmyjIHEIdvqIRMVkMr/telemetry");
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, ledState, strlen(ledState));
    ESP_LOGI(TAG, "STATO LED: %s", ledState);
    err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %lld",
                esp_http_client_get_status_code(client),
                (long long int) esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }  
    esp_http_client_cleanup(client);
}

#define ENOUGH 100

static void http_button_state(void)
{
    char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};
    esp_http_client_config_t config = {
        .host = "44.193.166.37",
        .path = "/scm8TnPXBKUoHjznY8ft",
        .query = "esp",
        .event_handler = _http_event_handler,
        .user_data = local_response_buffer,       
        .disable_auto_redirect = true,
    };

    configure_button();
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err;
    char str[ENOUGH];
    sprintf(str, "{\"pressed\": %d}", gpio_get_level(26));    
    const char* buttonState=str;
    
    esp_http_client_set_url(client, "http://44.193.166.37/api/v1/scm8TnPXBKUoHjznY8ft/telemetry");
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, buttonState, strlen(buttonState));
    ESP_LOGI(TAG, "STATO BUTTON : %s", buttonState);
    err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %lld",
                esp_http_client_get_status_code(client),
                (long long int) esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }  
    esp_http_client_cleanup(client);
}


static void http_parameter(void)
{
    char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};
    esp_http_client_config_t config = {
        .host = "44.193.166.37",
        .path = "/RDFyHVYCXtdPJZEiXtEq",
        .query = "esp",
        .event_handler = _http_event_handler,
        .user_data = local_response_buffer,       
        .disable_auto_redirect = true,
    };
    time_t t;
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err;
    srand((unsigned) time(&t));
    int par;
    par=rand()%100;
    char str[ENOUGH];
    sprintf(str, "{\"parameter\": %d}", par);    
    const char* parameter=str;
    esp_http_client_set_url(client, "http://44.193.166.37/api/v1/RDFyHVYCXtdPJZEiXtEq/telemetry");
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, parameter, strlen(parameter));
    ESP_LOGI(TAG, "PARAMETER: %s", parameter);
    err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %lld",
                esp_http_client_get_status_code(client),
                (long long int) esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }  
    esp_http_client_cleanup(client);
}

int i=0;

static void http_button_change_led_state(void)
{
    char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};
    esp_http_client_config_t config = {
        .host = "44.193.166.37",
        .path = "/pwUmyjIHEIdvqIRMVkMr",
        .query = "esp",
        .event_handler = _http_event_handler,
        .user_data = local_response_buffer,       
        .disable_auto_redirect = true,
    };

    configure_button();
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err;
    
    bool bState=gpio_get_level(26);
    char str[ENOUGH];

    do{ 
        bState=gpio_get_level(26);
        http_button_state();
        http_parameter();
        http_led_state();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }while(bState==1);

    bState=gpio_get_level(26);  

    switch(i){
        case 0:
            s_ledG_state=!s_ledG_state;
            blink_ledG();
            configure_ledG();
            i++;
            break;
        case 1:
            s_ledR_state=!s_ledR_state;
            blink_ledR();
            configure_ledR();
            i++;
            break;
        case 2:
            s_ledB_state=!s_ledB_state;
            blink_ledB();
            configure_ledB();
            i=0;
            break;
    };

    sprintf(str, "{\"red\":%d,\"green\":%d,\"blue\":%d}", s_ledR_state, s_ledG_state, s_ledB_state);  
    const char* ledState=str;

    esp_http_client_set_url(client, "http://44.193.166.37/api/v1/pwUmyjIHEIdvqIRMVkMr/telemetry");
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, ledState, strlen(ledState));
    ESP_LOGI(TAG, "STATO LED : %s", ledState);
    err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %lld",
                esp_http_client_get_status_code(client),
                (long long int) esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }  
     esp_http_client_cleanup(client);
    
}

static void main_task()
{
	// wait for connection
	char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};
    esp_http_client_config_t config = {
        .host = "maker.ifttt.com",
        .path = "/XeMgZPP_2BuuVw4GQV1Pf",
        .query = "esp",
        .event_handler = _http_event_handler,
        .user_data = local_response_buffer,       
        .disable_auto_redirect = true,
    };

    configure_button();
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err;
    
    bool bState=gpio_get_level(26);

    do{ 
        bState=gpio_get_level(26);
    }while(bState==1);

    bState=gpio_get_level(26);  
    const char* content="{\"Value1\": \"10\"}";

    esp_http_client_set_url(client, "http://maker.ifttt.com/trigger/hello/with/key/XeMgZPP_2BuuVw4GQV1Pf");
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");

    ESP_LOGI(TAG, "Content : %s", content);
    err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %lld",
                esp_http_client_get_status_code(client),
                (long long int) esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }  
     esp_http_client_cleanup(client);
}

static void http_test_task(void *pvParameters)
{
    while(1){
        //http_button_change_led_state();
        //http_button_state();
        //http_parameter();
        //http_led_state();
        main_task();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    ESP_LOGI(TAG, "Finish http example");
    vTaskDelete(NULL);
}

void app_main(void)
{
    s_ledR_state=false;
    blink_ledR();
    configure_ledR();
    s_ledG_state=false;
    blink_ledG();
    configure_ledG();
    s_ledB_state=false;
    blink_ledB();
    configure_ledB();

    configure_button();
   
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());
    ESP_LOGI(TAG, "Connected to AP, begin http example");

    xTaskCreate(&http_test_task, "http_test_task", 8192, NULL, 5, NULL);
}