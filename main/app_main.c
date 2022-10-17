#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "../include/protocol_examples_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "../include/led_strip.h"
#include "sdkconfig.h"

#define LED_GPIO 2

static const char *TAG = "MQTT_WITH_RABBITMQ_EP32";

static uint8_t led_state = 0;

static void blink_led(void) {
    led_state = !led_state;
    gpio_set_level(LED_GPIO, led_state);
    vTaskDelay(10);
}

static void configure_led(void) {
    ESP_LOGI(TAG, "Confiurando LED...");
    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
        // O cliente estabeleceu com sucesso uma conexão com o broker. O cliente agora está pronto para enviar e receber dados.
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
            ESP_LOGI(TAG, "ESP32 se inscreve nda fila /topic/qos0, MSD_ID=%d", msg_id);
            msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
            ESP_LOGI(TAG, "ESP32 se inscreve nda fila /topic/qos1, MSD_ID=%d", msg_id);
            
            msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
            ESP_LOGI(TAG, "ESP32 envia uma mensagem para a fila /topic/qos0, MSD_ID=%d", msg_id);
            msg_id = esp_mqtt_client_publish(client, "/topic/qos1", "data", 0, 1, 0);
            ESP_LOGI(TAG, "ESP32 envia uma mensagem para a fila /topic/qos1, MSD_ID=%d", msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            blink_led();
            printf("TOPIC=%.*s\r\n", event -> topic_len, event -> topic);
            printf("DATA=%.*s\r\n", event -> data_len, event -> data);
            blink_led();
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event -> event_id);
            break;
        }
}

static void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_config = {
        .host = "192.168.0.16",
        .port = 1883,
        .username = "guest",
        .password = "guest"
    };
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_config);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

void app_main(void)
{
    ESP_LOGI(TAG, "[APP] Startup...");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("MQTT_EXAMPLE", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_BASE", ESP_LOG_VERBOSE);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    configure_led();

    ESP_ERROR_CHECK(example_connect());
    mqtt_app_start();
}