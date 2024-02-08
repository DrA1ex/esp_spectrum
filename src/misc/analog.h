#pragma once

#include <Arduino.h>

#include "debug.h"

void read_analog_data(uint16_t *data, size_t size, uint16_t sample_rate) {
    const auto sample_interval = (uint32_t) 1000000 / sample_rate;

    // TOO FAST
    //system_adc_read_fast(data, size, 24);

    auto t_begin = micros();
    for (size_t i = 0; i < size; ++i) {
        auto t = micros();
        data[i] = analogRead(0);

        auto delta = micros() - t;
        if (delta < sample_interval) {
            delayMicroseconds(sample_interval - delta);
        }
    }

    VERBOSE(([&] {
        int32_t d_min = 1024, d_max = 0;
        for (size_t i = 0; i < size; ++i) {
            const auto value = data[i];

            if (value < d_min) d_min = value;
            if (value > d_max) d_max = value;
        }

        D_PRINTF("SIGNAL: %u..%u\n", d_min, d_max);
    })());

    VERBOSE(([&] {
        auto t_delta = micros() - t_begin;
        D_PRINTF("Data populating: %lu us\n", t_delta);

        const auto sample_rate = 1000000 * size / (micros() - t_begin);
        D_PRINTF("Sample rate: %lu Hz\n", sample_rate);
        D_PRINTF("Spectrum sample rate: %lu Hz, Band size: %lu Hz\n", sample_rate / 2, sample_rate / 2 / (size / 2));
    })());
}