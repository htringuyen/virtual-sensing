

#ifndef ZEPHYR_DRIVER_VRTIO_BUS_FRAME_H
#define ZEPHYR_DRIVER_VRTIO_BUS_FRAME_H

#include <stdint.h>
#include <zephyr/sys/ring_buffer.h>

 struct vbus_frame {
    uint8_t channel_idx;
    uint8_t *data;
    uint32_t size;
 };

int vbus_frame_decode(struct ring_buf *buffer, uint32_t buf_size,
                      struct vbus_frame ***frames, uint32_t *frame_count);

int vbus_frame_encode(const struct vbus_frame **frames, uint32_t frame_count, 
    uint8_t **buffer, uint32_t *buf_size);

#endif