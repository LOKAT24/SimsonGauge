#ifndef TEST_SIGNAL_H
#define TEST_SIGNAL_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Start an on-board test square wave (no external generator needed).
 *
 * Generates a fixed frequency on a spare GPIO via LEDC. Wire a jumper from that
 * pin to the MCPWM capture input to feed the gauge. Does nothing when the test
 * signal is disabled (see ENABLE_TEST_SIGNAL in test_signal.cpp).
 */
void test_signal_start(void);

#ifdef __cplusplus
}
#endif

#endif // TEST_SIGNAL_H
