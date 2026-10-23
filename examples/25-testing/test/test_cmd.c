#include <string.h>

#include "cmd.h"
#include "unity.h"

/* ---- fake hardware: records what the code under test asked for ---------- */
static int fake_led_calls;
static int fake_last_led;
static bool fake_last_on;
static uint32_t fake_now_ms;

static void fake_led_set(int led, bool on)
{
    fake_led_calls++;
    fake_last_led = led;
    fake_last_on = on;
}

static uint32_t fake_millis(void)
{
    return fake_now_ms;
}

static const cmd_hw_t hw = {fake_led_set, fake_millis};
static char out[128];

void setUp(void)
{
    fake_led_calls = 0;
    fake_last_led = -1;
    fake_now_ms = 0;
    memset(out, 0, sizeof out);
}

void tearDown(void) {}

static void test_led_red_on(void)
{
    TEST_ASSERT_EQUAL_INT(0, cmd_execute("led red on", out, sizeof out, &hw));
    TEST_ASSERT_EQUAL_STRING("ok", out);
    TEST_ASSERT_EQUAL_INT(1, fake_led_calls);
    TEST_ASSERT_EQUAL_INT(CMD_LED_RED, fake_last_led);
    TEST_ASSERT_TRUE(fake_last_on);
}

static void test_led_blue_off_with_extra_spaces(void)
{
    TEST_ASSERT_EQUAL_INT(0, cmd_execute("  led   blue\toff ", out, sizeof out, &hw));
    TEST_ASSERT_EQUAL_INT(CMD_LED_BLUE, fake_last_led);
    TEST_ASSERT_FALSE(fake_last_on);
}

static void test_bad_led_arguments_touch_nothing(void)
{
    TEST_ASSERT_EQUAL_INT(-1, cmd_execute("led purple on", out, sizeof out, &hw));
    TEST_ASSERT_EQUAL_INT(-1, cmd_execute("led red maybe", out, sizeof out, &hw));
    TEST_ASSERT_EQUAL_INT(-1, cmd_execute("led red", out, sizeof out, &hw));
    TEST_ASSERT_EQUAL_INT(0, fake_led_calls);
    TEST_ASSERT_EQUAL_STRING("error: usage: led <green|orange|red|blue> <on|off>", out);
}

static void test_baud_reports_brr_and_error(void)
{
    TEST_ASSERT_EQUAL_INT(0, cmd_execute("baud 16000000 115200", out, sizeof out, &hw));
    TEST_ASSERT_EQUAL_STRING("BRR=139 (0x008b), actual 115107 baud, error -807 ppm", out);
}

static void test_baud_rejects_nonsense(void)
{
    TEST_ASSERT_EQUAL_INT(-1, cmd_execute("baud 16000000 0", out, sizeof out, &hw));
    TEST_ASSERT_EQUAL_INT(-1, cmd_execute("baud -5 9600", out, sizeof out, &hw));
    TEST_ASSERT_EQUAL_INT(-1, cmd_execute("baud 16MHz 9600", out, sizeof out, &hw));
    TEST_ASSERT_EQUAL_INT(-1, cmd_execute("baud 100000 115200", out, sizeof out, &hw));
}

static void test_uptime_uses_the_clock(void)
{
    fake_now_ms = 4321;
    cmd_execute("uptime", out, sizeof out, &hw);
    TEST_ASSERT_EQUAL_STRING("uptime 4321 ms", out);
}

static void test_unknown_command(void)
{
    TEST_ASSERT_EQUAL_INT(-1, cmd_execute("reboot now", out, sizeof out, &hw));
    TEST_ASSERT_EQUAL_STRING("error: unknown command 'reboot'", out);
}

static void test_empty_and_overlong_lines(void)
{
    char longline[100];
    memset(longline, 'x', sizeof longline - 1);
    longline[sizeof longline - 1] = '\0';

    TEST_ASSERT_EQUAL_INT(0, cmd_execute("   ", out, sizeof out, &hw));
    TEST_ASSERT_EQUAL_STRING("", out);
    TEST_ASSERT_EQUAL_INT(-1, cmd_execute(longline, out, sizeof out, &hw));
}

static void test_reply_is_truncated_not_overflowed(void)
{
    char tiny[8];
    memset(tiny, '#', sizeof tiny);
    cmd_execute("help", tiny, sizeof tiny, &hw);
    TEST_ASSERT_EQUAL_STRING("command", tiny);   /* 7 chars + NUL */
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_led_red_on);
    RUN_TEST(test_led_blue_off_with_extra_spaces);
    RUN_TEST(test_bad_led_arguments_touch_nothing);
    RUN_TEST(test_baud_reports_brr_and_error);
    RUN_TEST(test_baud_rejects_nonsense);
    RUN_TEST(test_uptime_uses_the_clock);
    RUN_TEST(test_unknown_command);
    RUN_TEST(test_empty_and_overlong_lines);
    RUN_TEST(test_reply_is_truncated_not_overflowed);
    return UNITY_END();
}
