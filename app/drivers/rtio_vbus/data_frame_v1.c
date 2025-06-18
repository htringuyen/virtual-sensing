#include <rtio_vbus/data_frame.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(vbus_frame, LOG_LEVEL_DBG);

#define HEADER_SIZE 3
#define CHANNEL_IDX_OFFSET 0
#define FRAME_SIZE_FIRST_BYTE_IDX 1
#define FRAME_SIZE_SECOND_BYTE_IDX 2
#define FRAME_DATA_OFFSET 3


static inline void split_two_bytes(uint32_t value, uint8_t *b1, uint8_t *b2) {
    *b1 = (value >> 8) & 0xFF;
    *b2 = value & 0xFF;
}

static inline uint32_t concat_two_bytes(uint8_t b1, uint8_t b2) {
    return (uint32_t)((b1 << 8) | b2);
}

static inline struct vbus_frame *create_frame(uint8_t *buffer, uint32_t size, uint8_t channel_idx) {
    struct vbus_frame *frame = k_malloc(sizeof(struct vbus_frame));
    if (!frame) {
        LOG_ERR("Failed to allocate memory for frame");
        return NULL;
    }
    
    frame->channel_idx = channel_idx;
    frame->size = size;
    
    if (size == 0) {
        frame->data = NULL;
    } else {
        uint8_t *frame_data = k_malloc(size);  // Fixed: allocate 'size' bytes, not 1 byte
        if (!frame_data) {
            LOG_ERR("Failed to allocate memory for frame data");
            k_free(frame);
            return NULL;
        }
        memcpy(frame_data, buffer, size);
        frame->data = frame_data;
    }
    
    return frame;
}

int vbus_frame_decode(struct ring_buf *buffer, uint32_t buf_size,
                      struct vbus_frame ***frames, uint32_t *frame_count) {
    if (!buffer || !frames || !frame_count) {
        LOG_ERR("Invalid parameters");
        return -EINVAL;
    }
    
    if (buf_size > ring_buf_size_get(buffer)) {
        LOG_ERR("Requested decode bytes cannot be greater than buffer size");
        return -ENOTSUP;
    }

    uint8_t *claimed_data;
    uint32_t data_size;
    uint32_t frame_size;
    uint8_t channel_idx;
    uint32_t remaining_size = buf_size;
    *frame_count = 0;

    // Use dynamic array with reasonable initial capacity
    uint32_t frames_capacity = 8;
    *frames = k_malloc(frames_capacity * sizeof(struct vbus_frame *));
    if (!*frames) {
        LOG_ERR("Failed to allocate memory for frames array");
        return -ENOMEM;
    }
    
    while (remaining_size >= HEADER_SIZE) {
        uint32_t claimed_size = ring_buf_get_claim(buffer, &claimed_data, remaining_size);
        if (claimed_size < HEADER_SIZE) {
            break;
        }

        data_size = concat_two_bytes(claimed_data[FRAME_SIZE_FIRST_BYTE_IDX],
                                   claimed_data[FRAME_SIZE_SECOND_BYTE_IDX]);
        frame_size = data_size + HEADER_SIZE;
        
        if (frame_size > claimed_size || frame_size > remaining_size) {
            LOG_DBG("Insufficient data, discard decoding (frame_size=%d, bytes_available=%d)", data_size, claimed_size);
            ring_buf_get_finish(buffer, 0); // Release claim without consuming
            break;
        }

        // Expand array if needed
        if (*frame_count >= frames_capacity) {
            frames_capacity *= 2;
            struct vbus_frame **new_frames = k_realloc(*frames, 
                                                      frames_capacity * sizeof(struct vbus_frame *));
            if (!new_frames) {
                LOG_ERR("Failed to expand frames array");
                // Cleanup existing frames
                for (uint32_t i = 0; i < *frame_count; i++) {
                    if ((*frames)[i]) {
                        if ((*frames)[i]->data) {
                            k_free((*frames)[i]->data);
                        }
                        k_free((*frames)[i]);
                    }
                }
                k_free(*frames);
                *frames = NULL;
                *frame_count = 0;
                return -ENOMEM;
            }
            *frames = new_frames;
        }

        channel_idx = claimed_data[CHANNEL_IDX_OFFSET];
        (*frames)[*frame_count] = create_frame(claimed_data + FRAME_DATA_OFFSET, 
                                             data_size, channel_idx);
        
        if (!(*frames)[*frame_count]) {
            // Cleanup on failure
            for (uint32_t i = 0; i < *frame_count; i++) {
                if ((*frames)[i]) {
                    if ((*frames)[i]->data) {
                        k_free((*frames)[i]->data);
                    }
                    k_free((*frames)[i]);
                }
            }
            k_free(*frames);
            *frames = NULL;
            *frame_count = 0;
            return -ENOMEM;
        }
        
        (*frame_count)++;
        ring_buf_get_finish(buffer, frame_size);
        remaining_size -= frame_size;
    }

    // If no frames decoded, free the array
    if (*frame_count == 0) {
        k_free(*frames);
        *frames = NULL;
    }

    return 0;
}

int vbus_frame_encode(const struct vbus_frame **frames, uint32_t frame_count,
                      uint8_t **buffer, uint32_t *buf_size) {
    if (!frames || !buffer || !buf_size || frame_count == 0) {
        LOG_ERR("Invalid parameters");
        return -EINVAL;
    }
    
    uint32_t total_size = 0;
    uint32_t offset = 0;
    
    // Calculate total required buffer size
    for (uint32_t i = 0; i < frame_count; i++) {
        total_size += HEADER_SIZE + frames[i]->size;
    }

    *buffer = k_malloc(total_size);
    
    // Encode each frame
    for (uint32_t i = 0; i < frame_count; i++) {
        const struct vbus_frame *frame = frames[i];
        
        // Check for frame size overflow (max 16-bit value)
        if (frame->size > 0xFFFF) {
            LOG_ERR("Frame size too large: %u", frame->size);
            return -EINVAL;
        }
        
        // Encode header
        (*buffer)[offset + CHANNEL_IDX_OFFSET] = frame->channel_idx;
        split_two_bytes(frame->size, 
                       &(*buffer)[offset + FRAME_SIZE_FIRST_BYTE_IDX],
                       &(*buffer)[offset + FRAME_SIZE_SECOND_BYTE_IDX]);
        
        // Encode data
        if (frame->size > 0 && frame->data) {
            memcpy(&(*buffer)[offset + FRAME_DATA_OFFSET], frame->data, frame->size);
        }
        
        offset += HEADER_SIZE + frame->size;
    }
    
    *buf_size = total_size;
    return 0;
}