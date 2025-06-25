#ifndef UART_INTERFACE_H
#define UART_INTERFACE_H

#include <zephyr/types.h>
#include <zephyr/kernel.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>

// Buffer sizes
#define UART_TX_BUF_SIZE        64
#define UART_RX_BUF_SIZE        32

#define MIDI_BAUD_RATE 31250

// UART MIDI context structure
typedef struct {
    const struct device *uart_dev;
    struct k_sem tx_done_sem;
    struct k_mutex tx_mutex;
    bool tx_busy;
    bool async_api_supported;
    uint8_t tx_buf[UART_TX_BUF_SIZE];
    uint8_t rx_buf[UART_RX_BUF_SIZE];
} uart_midi_ctx_t;


/**
 * @brief Initialize and check if UART is ready
 * 
 * @param 
 * @return 
 */
int app_uart_init(void);

/**
 * @brief Send raw MIDI bytes via UART using asynchronous API
 */
int uart_send_midi_data(const uint8_t *data, size_t length);

#endif /* UART_INTERFACE_H */
