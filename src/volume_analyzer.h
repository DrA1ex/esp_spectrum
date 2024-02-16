#pragma once

#include <Arduino.h>

#include "./misc/analog.h"
#include "./misc/fourier.h"
#include "./misc/resample.h"
#include "./misc/scale.h"

template<uint16_t SampleSize, uint16_t BucketCount, uint8_t AnalogPin>
class VolumeAnalyzer {
    AnalogReader _reader;
    LogScale<> _log_scale;

    const uint16_t _update_interval;
    const uint16_t _read_interval;

    uint16_t _gain;
    uint16_t _gate;

    size_t _current_index = 0;
    uint16_t _history[BucketCount] = {};

    unsigned long _last_update_time = 0;

public:
    static constexpr uint16_t MAX_VALUE = decltype(_log_scale)::MAX_VALUE;

    VolumeAnalyzer(uint16_t sample_rate, uint16_t update_interval, uint16_t window_duration, uint16_t _gain = 1, uint16_t gate = 0) :
            _reader(SampleSize, AnalogPin, sample_rate),
            _log_scale(window_duration / update_interval),
            _update_interval(update_interval),
            _read_interval(max(0, update_interval - ((int32_t) 1000 * SampleSize / sample_rate) * 2)),
            _gain(_gain),
            _gate(gate) {}

    void tick();

    [[nodiscard]] inline uint16_t get(uint16_t index, uint8_t frac) const;
    [[nodiscard]] inline uint16_t delta() const { return millis() - _last_update_time; }
};

template<uint16_t SampleSize, uint16_t BucketCount, uint8_t AnalogPin>
void VolumeAnalyzer<SampleSize, BucketCount, AnalogPin>::tick() {
    if (millis() - _last_update_time >= _read_interval) {
        _reader.read_if_needed();
    }

    if (millis() - _last_update_time >= _update_interval) {
        _history[_current_index] = _log_scale.amplitude(_reader.data(), SampleSize);
        if (++_current_index == BucketCount) _current_index = 0;

        _last_update_time = millis();
    }
}

template<uint16_t SampleSize, uint16_t BucketCount, uint8_t AnalogPin>
uint16_t VolumeAnalyzer<SampleSize, BucketCount, AnalogPin>::get(uint16_t index, uint8_t) const {
    return _history[(BucketCount + _current_index - index - 1) % BucketCount];
}