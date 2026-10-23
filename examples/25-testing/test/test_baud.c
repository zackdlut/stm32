#include "baud.h"
#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

static void test_brr_for_115200_at_16mhz_matches_chapter_8(void)
{
    TEST_ASSERT_EQUAL_HEX32(0x8B, uart_brr(16000000, 115200));
    TEST_ASSERT_EQUAL_UINT32(115107, uart_baud_from_brr(16000000, 0x8B));
}

static void test_brr_rounds_to_nearest(void)
{
    /* 16 MHz / 9600 = 1666.67 -> 1667, not 1666 */
    TEST_ASSERT_EQUAL_UINT32(1667, uart_brr(16000000, 9600));
}

static void test_error_is_small_for_common_rates(void)
{
    static const uint32_t rates[] = {9600, 19200, 38400, 57600, 115200};
    for (unsigned i = 0; i < sizeof rates / sizeof rates[0]; i++) {
        int32_t ppm = uart_baud_error_ppm(16000000, rates[i]);
        TEST_ASSERT_INT32_WITHIN_MESSAGE(20000, 0, ppm, "more than 2% off");
    }
}

static void test_error_of_115200_at_16mhz(void)
{
    TEST_ASSERT_EQUAL_INT32(-807, uart_baud_error_ppm(16000000, 115200));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_brr_for_115200_at_16mhz_matches_chapter_8);
    RUN_TEST(test_brr_rounds_to_nearest);
    RUN_TEST(test_error_is_small_for_common_rates);
    RUN_TEST(test_error_of_115200_at_16mhz);
    return UNITY_END();
}
