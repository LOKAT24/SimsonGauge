// On-board test signal generator: feeds a known waveform to the gauge so the
// frequency-measurement path can be exercised without an external generator.
//
// Wire a jumper from TEST_OUT_GPIO to the MCPWM capture input (MCPWM_GPIO in
// main.cpp). Set ENABLE_TEST_SIGNAL to 0 to disable.
//
// Two modes (TEST_SIGNAL_MODE):
//   1 = fixed square wave at TEST_SIGNAL_HZ (simple, via LEDC).
//   2 = replay of a real Simson waveform captured on the oscilloscope. The
//       analog capture was thresholded to a digital edge train (see
//       tools/gen_simson_waveform.py -> simson_waveform.h) and is replayed with
//       the RMT peripheral in a hardware loop, so the gauge sees the genuine
//       ~120 Hz pulse pattern with its real cycle-to-cycle variation and glitches.

#include "test_signal.h"
#include <Arduino.h>

#define ENABLE_TEST_SIGNAL 1
#define TEST_SIGNAL_MODE   2        // 1 = square wave, 2 = Simson replay
#define TEST_OUT_GPIO      2

// --- Mode 1: simple square wave ---
#define TEST_SIGNAL_HZ     3500

#if ENABLE_TEST_SIGNAL && (TEST_SIGNAL_MODE == 2)
extern "C" {
#include "driver/rmt_tx.h"
#include "esp_err.h"
}
#include "simson_waveform.h"

static rmt_channel_handle_t s_rmt_chan = NULL;
static rmt_encoder_handle_t s_rmt_encoder = NULL;

static void simson_replay_start(void)
{
    rmt_tx_channel_config_t tx_cfg = {};
    tx_cfg.gpio_num          = (gpio_num_t)TEST_OUT_GPIO;
    tx_cfg.clk_src           = RMT_CLK_SRC_DEFAULT;
    tx_cfg.resolution_hz     = SIMSON_WAVEFORM_RESOLUTION_HZ;   // 1 us/tick
    tx_cfg.mem_block_symbols = 64;     // must hold the whole sequence for looping (we have ~30)
    tx_cfg.trans_queue_depth = 4;
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_cfg, &s_rmt_chan));

    rmt_copy_encoder_config_t enc_cfg = {};
    ESP_ERROR_CHECK(rmt_new_copy_encoder(&enc_cfg, &s_rmt_encoder));

    ESP_ERROR_CHECK(rmt_enable(s_rmt_chan));

    rmt_transmit_config_t tx = {};
    tx.loop_count = -1;                // infinite hardware loop, zero CPU cost
    ESP_ERROR_CHECK(rmt_transmit(s_rmt_chan, s_rmt_encoder,
                                 SIMSON_WAVEFORM, sizeof(SIMSON_WAVEFORM), &tx));
}
#endif

void test_signal_start(void)
{
#if ENABLE_TEST_SIGNAL
#if TEST_SIGNAL_MODE == 2
    simson_replay_start();
#else
    // W najnowszym rdzeniu Arduino (v3) preferowana jest funkcja tone(),
    // ew. stare ledcAttach/ledcWrite. Wracamy do prostej stalej wartosci.
    ledcAttach(TEST_OUT_GPIO, TEST_SIGNAL_HZ, 10);
    ledcWrite(TEST_OUT_GPIO, 512);
#endif
#endif
}
