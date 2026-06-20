// AUTOGENEROWANE przez tools/gen_simson_waveform.py - nie edytowac recznie.
// Zrodlo: processed_Simson_wolne_ok1250_FULL.csv (histereza 1.0/2.0 V)
// 30 symboli, czas petli 240.00 ms, rozdzielczosc 1.0 us/tick.
#ifndef SIMSON_WAVEFORM_H
#define SIMSON_WAVEFORM_H

#include "driver/rmt_tx.h"

#define SIMSON_WAVEFORM_RESOLUTION_HZ 1000000u  // 1.0 us/tick

static const rmt_symbol_word_t SIMSON_WAVEFORM[] = {
    { .duration0 = 3100, .level0 = 0, .duration1 = 4280, .level1 = 1 },
    { .duration0 = 3683, .level0 = 0, .duration1 = 4329, .level1 = 1 },
    { .duration0 = 3867, .level0 = 0, .duration1 = 4645, .level1 = 1 },
    { .duration0 = 3826, .level0 = 0, .duration1 = 4421, .level1 = 1 },
    { .duration0 = 3775, .level0 = 0, .duration1 = 4482, .level1 = 1 },
    { .duration0 = 3969, .level0 = 0, .duration1 = 4834, .level1 = 1 },
    { .duration0 = 3926, .level0 = 0, .duration1 = 4567, .level1 = 1 },
    { .duration0 = 3874, .level0 = 0, .duration1 = 4641, .level1 = 1 },
    { .duration0 = 3301, .level0 = 0, .duration1 = 1, .level1 = 1 },
    { .duration0 = 781, .level0 = 0, .duration1 = 5016, .level1 = 1 },
    { .duration0 = 4035, .level0 = 0, .duration1 = 4724, .level1 = 1 },
    { .duration0 = 3979, .level0 = 0, .duration1 = 4809, .level1 = 1 },
    { .duration0 = 4200, .level0 = 0, .duration1 = 5067, .level1 = 1 },
    { .duration0 = 3638, .level0 = 0, .duration1 = 4028, .level1 = 1 },
    { .duration0 = 3472, .level0 = 0, .duration1 = 4013, .level1 = 1 },
    { .duration0 = 3633, .level0 = 0, .duration1 = 4227, .level1 = 1 },
    { .duration0 = 3578, .level0 = 0, .duration1 = 4131, .level1 = 1 },
    { .duration0 = 3550, .level0 = 0, .duration1 = 4149, .level1 = 1 },
    { .duration0 = 3736, .level0 = 0, .duration1 = 4424, .level1 = 1 },
    { .duration0 = 3701, .level0 = 0, .duration1 = 4231, .level1 = 1 },
    { .duration0 = 3651, .level0 = 0, .duration1 = 4282, .level1 = 1 },
    { .duration0 = 3833, .level0 = 0, .duration1 = 4585, .level1 = 1 },
    { .duration0 = 3791, .level0 = 0, .duration1 = 4367, .level1 = 1 },
    { .duration0 = 3742, .level0 = 0, .duration1 = 4428, .level1 = 1 },
    { .duration0 = 3931, .level0 = 0, .duration1 = 4767, .level1 = 1 },
    { .duration0 = 3887, .level0 = 0, .duration1 = 4510, .level1 = 1 },
    { .duration0 = 3841, .level0 = 0, .duration1 = 4585, .level1 = 1 },
    { .duration0 = 4042, .level0 = 0, .duration1 = 4942, .level1 = 1 },
    { .duration0 = 3973, .level0 = 0, .duration1 = 4632, .level1 = 1 },
    { .duration0 = 3919, .level0 = 0, .duration1 = 3649, .level1 = 1 },
};

#define SIMSON_WAVEFORM_LEN (sizeof(SIMSON_WAVEFORM) / sizeof(SIMSON_WAVEFORM[0]))

#endif // SIMSON_WAVEFORM_H
