#include "ringbuf.h"
#include "unity.h"

static uint8_t storage[8];
static ringbuf_t rb;

void setUp(void)
{
    TEST_ASSERT_TRUE(rb_init(&rb, storage, sizeof storage));
}

void tearDown(void) {}

static void test_new_buffer_is_empty(void)
{
    uint8_t b;
    TEST_ASSERT_EQUAL_UINT32(0, rb_count(&rb));
    TEST_ASSERT_FALSE(rb_get(&rb, &b));
}

static void test_bytes_come_out_in_order(void)
{
    uint8_t b;
    rb_put(&rb, 'a');
    rb_put(&rb, 'b');
    TEST_ASSERT_TRUE(rb_get(&rb, &b));
    TEST_ASSERT_EQUAL_CHAR('a', b);
    TEST_ASSERT_TRUE(rb_get(&rb, &b));
    TEST_ASSERT_EQUAL_CHAR('b', b);
    TEST_ASSERT_FALSE(rb_get(&rb, &b));
}

static void test_holds_exactly_size_bytes(void)
{
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT_TRUE(rb_put(&rb, (uint8_t)i));
    }
    TEST_ASSERT_FALSE(rb_put(&rb, 99));         /* full: rejected, nothing overwritten */
    TEST_ASSERT_EQUAL_UINT32(8, rb_count(&rb));
    uint8_t b;
    rb_get(&rb, &b);
    TEST_ASSERT_EQUAL_UINT8(0, b);
}

static void test_wraps_around_the_array(void)
{
    uint8_t b;
    for (int round = 0; round < 5; round++) {   /* 5 x 3 bytes through an 8-byte array */
        for (int i = 0; i < 3; i++) {
            TEST_ASSERT_TRUE(rb_put(&rb, (uint8_t)(round * 10 + i)));
        }
        for (int i = 0; i < 3; i++) {
            TEST_ASSERT_TRUE(rb_get(&rb, &b));
            TEST_ASSERT_EQUAL_UINT8(round * 10 + i, b);
        }
    }
}

static void test_counters_wrap_at_2_pow_32(void)
{
    /* White-box: after ~4 billion bytes the free-running counters overflow. */
    rb.head = rb.tail = UINT32_MAX - 1u;
    for (int i = 0; i < 5; i++) {
        TEST_ASSERT_TRUE(rb_put(&rb, (uint8_t)i));
    }
    TEST_ASSERT_EQUAL_UINT32(5, rb_count(&rb));
    uint8_t b;
    for (int i = 0; i < 5; i++) {
        TEST_ASSERT_TRUE(rb_get(&rb, &b));
        TEST_ASSERT_EQUAL_UINT8(i, b);
    }
}

static void test_rejects_sizes_that_are_not_powers_of_two(void)
{
    ringbuf_t other;
    TEST_ASSERT_FALSE(rb_init(&other, storage, 0));
    TEST_ASSERT_FALSE(rb_init(&other, storage, 6));
    TEST_ASSERT_TRUE(rb_init(&other, storage, 4));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_new_buffer_is_empty);
    RUN_TEST(test_bytes_come_out_in_order);
    RUN_TEST(test_holds_exactly_size_bytes);
    RUN_TEST(test_wraps_around_the_array);
    RUN_TEST(test_counters_wrap_at_2_pow_32);
    RUN_TEST(test_rejects_sizes_that_are_not_powers_of_two);
    return UNITY_END();
}
