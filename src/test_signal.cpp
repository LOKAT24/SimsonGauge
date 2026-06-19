// On-board test signal generator: feeds a known square wave to the gauge so the
// frequency-measurement path can be exercised without an external generator.
//
// Wire a jumper from TEST_OUT_GPIO to the MCPWM capture input (MCPWM_GPIO in
// main.cpp). With the default multiplier (1.0) the needle settles at the angle
// matching TEST_SIGNAL_HZ. Set ENABLE_TEST_SIGNAL to 0 to disable.

#include "test_signal.h"
#include <Arduino.h>

#define ENABLE_TEST_SIGNAL 1
#define TEST_OUT_GPIO      2
#define TEST_SIGNAL_HZ     1000

void test_signal_start(void)
{
#if ENABLE_TEST_SIGNAL
    ledcAttach(TEST_OUT_GPIO, TEST_SIGNAL_HZ, 10);  // 10-bit resolution
    ledcWrite(TEST_OUT_GPIO, 512);                  // 512/1023 ~= 50% duty
#endif
}
