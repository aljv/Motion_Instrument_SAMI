#include <zephyr/drivers/uart.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <zephyr/sys/ring_buffer.h>

#include "uart_interface.h"

LOG_MODULE_REGISTER(uart_interface, LOG_LEVEL_DBG);

// UART context - global state
static uart_midi_ctx_t uart_ctx;

// Configuration for MIDI (31.25 kbps, 8N1)
static const struct uart_config uart_cfg = {
    .baudrate = MIDI_BAUD_RATE,
    .parity = UART_CFG_PARITY_NONE,
    .stop_bits = UART_CFG_STOP_BITS_1,
    .data_bits = UART_CFG_DATA_BITS_8,
    .flow_ctrl = UART_CFG_FLOW_CTRL_NONE
};


// Ring buffer for RX data (if needed for MIDI input)
#define RX_RING_BUF_SIZE 256
static uint8_t rx_ring_buf_data[RX_RING_BUF_SIZE];
static struct ring_buf rx_ring_buf;

// Asynchronous UART callback (if supported)
static void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data)
{
    uart_midi_ctx_t *ctx = (uart_midi_ctx_t *)user_data;

    switch (evt->type) {
    case UART_TX_DONE:
        //LOG_DBG("UART TX completed: %d bytes", evt->data.tx.len);
        ctx->tx_busy = false;
        k_sem_give(&ctx->tx_done_sem);
        break;

    case UART_TX_ABORTED:
        LOG_WRN("UART TX aborted");
        ctx->tx_busy = false;
        k_sem_give(&ctx->tx_done_sem);
        break;

    case UART_RX_RDY:
        /*LOG_DBG("UART RX ready: %d bytes at offset %d", 
                evt->data.rx.len, evt->data.rx.offset);*/
        
        // Copy received data to ring buffer
        if (evt->data.rx.len > 0) {
            uint32_t written = ring_buf_put(&rx_ring_buf, 
                                          &evt->data.rx.buf[evt->data.rx.offset], 
                                          evt->data.rx.len);
            if (written != evt->data.rx.len) {
                LOG_WRN("RX ring buffer overflow, lost %d bytes", 
                        evt->data.rx.len - written);
            }
        }
        break;

    case UART_RX_BUF_REQUEST:
        LOG_DBG("UART RX buffer request");
        // Provide new buffer for continuous reception
        uart_rx_enable(dev, ctx->rx_buf, sizeof(ctx->rx_buf), SYS_FOREVER_US);
        break;

    case UART_RX_BUF_RELEASED:
        LOG_DBG("UART RX buffer released");
        break;

    case UART_RX_DISABLED:
        LOG_DBG("UART RX disabled");
        break;

    default:
        LOG_WRN("Unknown UART event: %d", evt->type);
        break;
    }
}

// Interrupt-driven RX handler (fallback mode)
static void uart_isr_rx_handler(const struct device *dev, void *user_data)
{
    uint8_t byte;
    
    while (uart_poll_in(dev, &byte) == 0) {
        // Put received byte into ring buffer
        if (ring_buf_put(&rx_ring_buf, &byte, 1) == 0) {
            LOG_WRN("RX ring buffer full, dropping byte");
        }
    }
}

int app_uart_init(void)
{
    int err;
    
    // Initialize synchronization primitives
    k_sem_init(&uart_ctx.tx_done_sem, 0, 1);
    k_mutex_init(&uart_ctx.tx_mutex);
    uart_ctx.tx_busy = false;
    uart_ctx.async_api_supported = false;
    
    // Initialize ring buffer for RX
    ring_buf_init(&rx_ring_buf, sizeof(rx_ring_buf_data), rx_ring_buf_data);

    // Get UART device
    uart_ctx.uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart1));
    if (!device_is_ready(uart_ctx.uart_dev)) {
        LOG_ERR("UART device not ready");
        return -ENODEV;
    }

    // Try to set up asynchronous API first
    err = uart_callback_set(uart_ctx.uart_dev, uart_cb, &uart_ctx);
    if (err == 0) {
        LOG_INF("Using asynchronous UART API");
        uart_ctx.async_api_supported = true;
        
        // Enable RX for continuous reception
        err = uart_rx_enable(uart_ctx.uart_dev, uart_ctx.rx_buf, 
                            sizeof(uart_ctx.rx_buf), SYS_FOREVER_US);
        if (err) {
            LOG_WRN("Failed to enable UART RX: %d", err);
            // Continue without RX - we mainly need TX for MIDI output
        }
    } else {
        LOG_WRN("Async UART API not supported (error %d), falling back to interrupt-driven", err);
        
        // Fall back to interrupt-driven API
        uart_irq_callback_user_data_set(uart_ctx.uart_dev, uart_isr_rx_handler, &uart_ctx);
        uart_irq_rx_enable(uart_ctx.uart_dev);
        
        uart_ctx.async_api_supported = false;
    }

    LOG_INF("UART MIDI interface initialized successfully");
    LOG_INF("Mode: %s", uart_ctx.async_api_supported ? "Asynchronous" : "Interrupt-driven");

    return 0;
}

int uart_send_midi_data(const uint8_t *data, size_t len)
{
    if (!data || len == 0) {
        return -EINVAL;
    }

    // Acquire mutex to ensure atomic transmission
    k_mutex_lock(&uart_ctx.tx_mutex, K_FOREVER);

    int err = 0;
    
    if (uart_ctx.async_api_supported) {
        // Use asynchronous API
        uart_ctx.tx_busy = true;
        
        err = uart_tx(uart_ctx.uart_dev, data, len, SYS_FOREVER_US);
        if (err) {
            LOG_ERR("Failed to start UART TX: %d", err);
            uart_ctx.tx_busy = false;
        } else {
            // Wait for transmission to complete
            err = k_sem_take(&uart_ctx.tx_done_sem, K_MSEC(1000));
            if (err) {
                LOG_ERR("UART TX timeout");
                uart_tx_abort(uart_ctx.uart_dev);
                uart_ctx.tx_busy = false;
                err = -ETIMEDOUT;
            }
        }
    } else {
        // Use polling API as fallback
        for (size_t i = 0; i < len; i++) {
            uart_poll_out(uart_ctx.uart_dev, data[i]);
        }
        //LOG_DBG("Sent %d MIDI bytes via polling", len);
    }

    k_mutex_unlock(&uart_ctx.tx_mutex);
    return err;
}



