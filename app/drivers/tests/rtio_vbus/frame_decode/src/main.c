#include <stdint.h>
#include <zephyr/ztest.h>
#include <zephyr/sys/ring_buffer.h>
#include <string.h>
#include <rtio_vbus/data_frame.h>
#include <zephyr/logging/log.h>

#define TEST_BUFFER_SIZE 256

LOG_MODULE_REGISTER(frame_decode_test, LOG_LEVEL_DBG);

static uint8_t test_ring_buffer[TEST_BUFFER_SIZE];
static struct ring_buf test_buf;

static void setup_test_buffer(void)
{
    ring_buf_init(&test_buf, TEST_BUFFER_SIZE, test_ring_buffer);
}

ZTEST_SUITE(vbus_frame_tests, NULL, NULL, NULL, NULL, NULL);

ZTEST(vbus_frame_tests, test_decode_invalid_params)
{
    setup_test_buffer();
    
    struct vbus_frame **frames = NULL;
    uint32_t frame_count = 0;
    
    // Test NULL buffer
    zassert_equal(vbus_frame_decode(NULL, 10, &frames, &frame_count), -EINVAL);
    
    // Test NULL frames pointer
    zassert_equal(vbus_frame_decode(&test_buf, 10, NULL, &frame_count), -EINVAL);
}

ZTEST(vbus_frame_tests, test_decode_single_frame_with_data)
{
    setup_test_buffer();
    
    // Create frame: channel=2, size=4, data="TEST"
    uint8_t test_data[] = {0x02, 0x00, 0x04, 'T', 'E', 'S', 'T'};
    ring_buf_put(&test_buf, test_data, sizeof(test_data));
    
    struct vbus_frame **frames = NULL;
    uint32_t frame_count = 0;
    
    int ret = vbus_frame_decode(&test_buf, sizeof(test_data), &frames, &frame_count);
    
    zassert_equal(ret, 0);
    zassert_equal(frame_count, 1);
    zassert_not_null(frames);
    zassert_equal(frames[0]->channel_idx, 2);
    zassert_equal(frames[0]->size, 4);
    zassert_not_null(frames[0]->data);
    zassert_mem_equal(frames[0]->data, "TEST", 4);
    
    // Cleanup
    if (frames) {
        for (uint32_t i = 0; i < frame_count; i++) {
            if (frames[i]->data) {
                k_free(frames[i]->data);
            }
        }
        k_free(frames);
    }
}

ZTEST(vbus_frame_tests, test_decode_incomplete_frame) {

    setup_test_buffer();

    const uint8_t FRAME_SIZE = 0x0C;

    // create incomplete frame
    uint8_t test_data[] = {0x01, 0x00, 0x0C, 'H', 'E', 'L', 'L', 'O', '_'};

    ring_buf_put(&test_buf, test_data, sizeof(test_data));

    struct vbus_frame **frames = NULL;
    uint32_t frame_count = 0;

    int ret = vbus_frame_decode(&test_buf, ring_buf_size_get(&test_buf), &frames, &frame_count);

    zassert_equal(ret, 0);
    zassert_equal(frame_count, 0);
    zassert_is_null(frames);

    uint8_t test_data2[] = {'W', 'O', 'R', 'L', 'D', '\0'};
    ring_buf_put(&test_buf, test_data2, sizeof(test_data2));

    ret = vbus_frame_decode(&test_buf, ring_buf_size_get(&test_buf), &frames, &frame_count);

    zassert_equal(ret, 0);
    zassert_equal(frame_count, 1);
    zassert_not_null(frames);
    zassert_equal(frames[0]->channel_idx, 1);
    zassert_equal(frames[0]->size, FRAME_SIZE);
    zassert_not_null(frames[0]->data);
    zassert_mem_equal(frames[0]->data, "HELLO_WORLD\0", FRAME_SIZE);

    LOG_INF("Received data: %s", frames[0]->data);
}