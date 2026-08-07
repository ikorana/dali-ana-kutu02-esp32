#pragma once

#include <string>
#include <vector>
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

typedef void (*uartCallback_t)(const uint8_t*, size_t);

class UartDma {
public:
    // Sınıf oluşturulduğunda port numarasını belirler
    explicit UartDma(uart_port_t port = UART_NUM_1) : _port(port), _event_queue(nullptr) {}

    // Destructor: Kaynakları otomatik temizler (RAII)
    ~UartDma() {
        stop();
    }

    // UART Başlatma (DMA benzeri Queue mekanizmasıyla)
    void set_Callback( uartCallback_t cb) {
      callback = cb;
    }
    esp_err_t begin(int tx_pin, int rx_pin, int baud = 115200) {
        
        uart_config_t uart_config = {};
            uart_config.baud_rate = baud;
            uart_config.data_bits = UART_DATA_8_BITS;
            uart_config.parity = UART_PARITY_DISABLE;
            uart_config.stop_bits = UART_STOP_BITS_1;
            uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
            uart_config.source_clk = UART_SCLK_DEFAULT;

        ESP_ERROR_CHECK(uart_param_config(_port, &uart_config));
        ESP_ERROR_CHECK(uart_set_pin(_port, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

        // RX ve TX için 2048 byte'lık donanım buffer'ı ayırır. 
        // _event_queue parametresi asenkron olay yönetimi sağlar.
        return uart_driver_install(_port, BUF_SIZE, BUF_SIZE, 20, &_event_queue, 0);
    }

    // Veri Gönderimi (Donanım FIFO'suna asenkron yazar)
    int send(const std::string& data) {
        std::string terminated_data = data + "#";
        int donen = uart_write_bytes(_port, terminated_data.c_str(), terminated_data.length());
        return donen;
    }

    int now_send(const std::string& data) {
       return uart_tx_chars(_port, data.c_str(), data.length());
    }

    // Arka planda sürekli dinleme yapan Task fonksiyonu
    void startListening() {
        xTaskCreatePinnedToCore([](void* arg) {
            static_cast<UartDma*>(arg)->eventLoop();
        }, "uart_event_task", 4096, this, 12, nullptr, 1);
    }

    void stop() {
        if (uart_is_driver_installed(_port)) {
            uart_driver_delete(_port);
        }
    }

private:
    uart_port_t _port;
    QueueHandle_t _event_queue;
    static constexpr int BUF_SIZE = 2048;
    static constexpr const char* TAG = "UART_CPP";
    uartCallback_t callback = NULL;

    // Kesme tabanlı olay döngüsü (DMA mantığı burada simüle edilir)
    void eventLoop() {
    uart_event_t event;
    std::vector<uint8_t> rx_buffer(BUF_SIZE);

    for (;;) {
        if (xQueueReceive(_event_queue, (void*)&event, portMAX_DELAY)) {
            switch (event.type) {
                case UART_DATA: { // <--- Süslü parantez ekledik
                    int len = uart_read_bytes(_port, rx_buffer.data(), event.size, portMAX_DELAY);
                    if (len > 0) {
                        onDataReceived(rx_buffer.data(), len);
                    }
                } break; // <--- Parantezi kapattık

                case UART_FIFO_OVF: {
                    ESP_LOGW(TAG, "Hardware FIFO Overflow!");
                    uart_flush_input(_port);
                    xQueueReset(_event_queue);
                } break;

                case UART_BUFFER_FULL: {
                    ESP_LOGW(TAG, "Ring Buffer Full!");
                    uart_flush_input(_port);
                    xQueueReset(_event_queue);
                } break;

                case UART_BREAK:
                case UART_DATA_BREAK:
                case UART_PARITY_ERR:
                case UART_FRAME_ERR:
                case UART_PATTERN_DET: {
                    ESP_LOGW(TAG, "UART Event Error or Pattern: %d", event.type);
                } break;

                case UART_EVENT_MAX:
                    break;

                default: {
                    ESP_LOGD(TAG, "Unhandled UART event: %d", event.type);
                } break;
            }
        }
    }
}

    // Gelen veriyi işleme (Buraya bir callback veya bffuer mekanizması eklenebilir)
    void onDataReceived(const uint8_t* data, size_t len) {
        // Örnek: Gelen veriyi logla
        //ESP_LOGI(TAG, "Gelen (%d byte): %.*s", (int)len, (int)len, data);
        callback(data, len);

    }
};