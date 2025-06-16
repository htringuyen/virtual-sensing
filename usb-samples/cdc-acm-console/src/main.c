#include "zephyr/device.h"
#include "zephyr/kernel/thread.h"
#include "zephyr/kernel/thread_stack.h"
#include "zephyr/toolchain.h"
#include "zephyr/usb/usbd.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/_intsup.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <usb_samples/common/sample_usbd.h>
#include <zephyr/drivers/uart.h>

// register log module
LOG_MODULE_REGISTER(cdc_acm_echo, LOG_LEVEL_DBG);

// check if zephyr console uses cdc-acm-uart backend
BUILD_ASSERT(DT_NODE_HAS_COMPAT(DT_NODELABEL(cdc_acm_uart0), zephyr_cdc_acm_uart),
        "CDC-ACM UART device 0 not found");

BUILD_ASSERT(DT_NODE_HAS_COMPAT(DT_NODELABEL(cdc_acm_uart1), zephyr_cdc_acm_uart),
        "CDC-ACM UART device 1 not found");

struct usbd_context *sample_usbd;

#define STACK_SIZE 512
#define THREAD_PRIO 9

k_tid_t sdev1_tid;
k_tid_t sdev2_tid;
struct k_thread sdev1_thread;
struct k_thread sdev2_thread;
K_THREAD_STACK_DEFINE(sdev1_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(sdev2_stack, STACK_SIZE);



// static void print_usb_info_callback(const struct usbd_context *ctx, const struct usbd_msg *msg) {
//     LOG_INF("============== USB CDC ACM devices info ==============");
//     LOG_INF("USB name: %s\n", ctx->name);
//     LOG_INF("USB status: %s\n", ctx->status);
// }


static int enable_usb_device(void) {
    int err;
    sample_usbd = sample_usbd_init_device(NULL);

    if (sample_usbd == NULL) {
        LOG_ERR("Failed to initialize USB device");
        return -ENODEV;
    }

    if (!usbd_can_detect_vbus(sample_usbd)) {
        err = usbd_enable(sample_usbd);
        if (err) {
            LOG_ERR("Failed to enable device support");
            return err;
        }
    }

    LOG_INF("USB device support enabled");
    return 0;
}

static void uart_print(const char *str, struct device *uart_dev) {
    while (*str) {
        uart_poll_out(uart_dev, *str++);
    }
}

static void uart_print_formatted(struct device *uart_dev, const char *name, int counter) {
    // Print device name
    const char *p = name;
    while (*p) {
        uart_poll_out(uart_dev, *p++);
    }
    
    // Print colon
    uart_poll_out(uart_dev, ':');
    
    // Print 5-digit zero-padded counter
    char digits[6];
    int temp = counter;
    digits[5] = '\0';
    for (int i = 4; i >= 0; i--) {
        digits[i] = '0' + (temp % 10);
        temp /= 10;
    }
    uart_print(digits, uart_dev);
    
    // Print constant suffix
    uart_print(": Hello, world!\n", uart_dev);
}

static void serial_device_thread_run(void *arg1, void *arg2, void *arg3) {
    struct device *dev = (struct device *) arg1;
    
    uint32_t dtr = 0;
    while (!dtr) {
        uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
        k_sleep(K_MSEC(200));
    }
    
    int i = 0;
    while (1) {
        uart_print_formatted(dev, dev->name, i++);
        k_sleep(K_MSEC(1000));
    }
}

int main(void) {

    int err = 0;
    err = enable_usb_device();

    if (err) {
        LOG_ERR("Faield to initialize USB device");
        return -ENODEV;
    }

    // get zephyr uart devices
    const struct device *uart_dev1 = DEVICE_DT_GET(DT_NODELABEL(cdc_acm_uart0));
    const struct device *uart_dev2 = DEVICE_DT_GET(DT_NODELABEL(cdc_acm_uart1));


    LOG_INF("Serial devices ready. Starting serial device threads.");

    sdev1_tid = k_thread_create(&sdev1_thread, sdev1_stack, STACK_SIZE, 
        serial_device_thread_run, uart_dev1, NULL, NULL, THREAD_PRIO, 0, K_NO_WAIT);

    sdev2_tid = k_thread_create(&sdev2_thread, sdev2_stack, STACK_SIZE, 
        serial_device_thread_run, uart_dev2, NULL, NULL, THREAD_PRIO, 0, K_NO_WAIT);
}