#include <stdint.h>
#include <zephyr/ztest.h>
#include <zephyr/sys/ring_buffer.h>
#include <string.h>
#include <rtio_vbus/data_frame.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(frame_encode_test, LOG_LEVEL_DBG);

ZTEST_SUITE(vbus_frame_tests, NULL, NULL, NULL, NULL, NULL);

ZTEST(vbus_frame_tests, test_encode_invalid_params)
{
    struct vbus_frame frame1 = {.channel_idx = 1, .size = 0, .data = NULL};
    struct vbus_frame *frames[] = {&frame1};
    uint8_t buffer[100];
    uint8_t *buf_ptr = buffer;
    uint32_t buf_size = sizeof(buffer);
    
    // Test NULL frames
    zassert_equal(vbus_frame_encode(NULL, 1, &buf_ptr, &buf_size), -EINVAL);
    
    // Test NULL buffer
    zassert_equal(vbus_frame_encode((const struct vbus_frame **)frames, 1, NULL, &buf_size), -EINVAL);
    
    // Test frame_count = 0
    zassert_equal(vbus_frame_encode((const struct vbus_frame **)frames, 0, &buf_ptr, &buf_size), -EINVAL);
}

ZTEST(vbus_frame_tests, test_encode_single_frame)
{
    // Create frame with data
    uint8_t test_data[] = {'A', 'B', 'C'};
    struct vbus_frame frame1 = {
        .channel_idx = 5,
        .size = 3,
        .data = test_data
    };
    struct vbus_frame *frames[] = {&frame1};
    
    uint8_t *buf_ptr = NULL;
    uint32_t buf_size = 0;
    
    int ret = vbus_frame_encode((const struct vbus_frame **) frames, 1, &buf_ptr, &buf_size);
    
    zassert_equal(ret, 0);
    zassert_equal(buf_size, 6); // 3 header + 3 data
    zassert_not_null(buf_ptr);
    
    // Check encoded frame
    zassert_equal(buf_ptr[0], 5);    // channel_idx
    zassert_equal(buf_ptr[1], 0);    // size high byte
    zassert_equal(buf_ptr[2], 3);    // size low byte
    zassert_equal(buf_ptr[3], 'A');  // data
    zassert_equal(buf_ptr[4], 'B');
    zassert_equal(buf_ptr[5], 'C');
    
    // Cleanup
    k_free(buf_ptr);
}

/*
* ZTest have bug when running unittest
* Context: test case is wrong in logic but correct in syntax
* Expected: Run time error
* Actual: Build error without any error message
*/
ZTEST(vbus_frame_tests, test_encode_multiple_frames) {

    const uint32_t FRAME_COUNT = 10;
    const struct vbus_frame **frames = k_malloc(FRAME_COUNT * sizeof(struct vbus_frame *));
    uint8_t *encoded_buf;
    uint32_t buf_size;
    uint32_t expected_buf_size = 0;
    for (int i = 0; i < FRAME_COUNT; i++) {
        uint8_t *data = k_malloc(i * sizeof(uint8_t));
        for (int j = 0; j < i; j++) {
            data[j] = j;
        }
        struct vbus_frame *frame = k_malloc(sizeof(struct vbus_frame));
        frame->channel_idx = i;
        frame->size = i;
        frame->data = data;
        frames[i] = frame;
        expected_buf_size += 3 + i;
    }

    
    int ret = vbus_frame_encode(frames, FRAME_COUNT, &encoded_buf, &buf_size);

    zassert_equal(ret, 0);

    zassert_equal(buf_size, expected_buf_size);

    uint32_t offset = 0;
    for (int i = 0; i < FRAME_COUNT; i++) {
        const struct vbus_frame *frame = frames[i];
        zassert_equal(encoded_buf[offset], frame->channel_idx);
        zassert_equal(encoded_buf[offset + 1], (frame->size >> 8) & 0xFF);
        zassert_equal(encoded_buf[offset + 2], frame->size & 0xFF);
        for (int j = 0; j < i; j++) {
            zassert_equal(encoded_buf[offset + 3 + j], frame->data[j]);
        }
        offset += frame->size + 3;
    }
}
