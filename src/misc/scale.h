#pragma once

#include <array>
#include <cmath>

#include "debug.h"
#include "misc/window.h"

template<uint16_t LogCnt = 256, uint16_t MaxValue = 4096>
class LogScale {
    Window<WindowMode::MAX> _window_max;
    Window<WindowMode::MIN> _window_min;

public:
    const uint16_t LOG_CNT = LogCnt;
    const uint16_t MAX_VALUE = MaxValue;

    explicit LogScale(size_t window) : _window_max(window), _window_min(window) {}

    void scale(uint16_t *data, size_t size);

private:
    constexpr std::array<int16_t, LogCnt> _init_log_table();

    const std::array<int16_t, LogCnt> _log_table = _init_log_table();
};

template<uint16_t LogCnt, uint16_t MaxValue>
void LogScale<LogCnt, MaxValue>::scale(uint16_t *data, size_t size) {
    const auto start_t = micros();

    uint16_t v_max = std::numeric_limits<uint16_t>::min(), v_min = std::numeric_limits<uint16_t>::max();
    for (size_t i = 0; i < size; ++i) {
        const auto value = data[i];

        if (value < v_min) v_min = value;
        if (value > v_max) v_max = value;
    }

    VERBOSE(D_PRINTF("LogScale Input: %u..%u\n", v_min, v_max));

    _window_min.add(v_min);
    _window_max.add(v_max);

    const auto min = _window_min.get(), max = _window_max.get();

    VERBOSE(D_PRINTF("LogScale Window: %u..%u\n", min, max));

    const auto width = max - min;
    if (width == 0) return;

    for (size_t i = 0; i < size; ++i) {
        const uint16_t rel_value = (data[i] - min) * LOG_CNT / width;
        data[i] = _log_table[rel_value];
    }

    D_PRINTF("LogScale: %lu us\n", micros() - start_t);
}


template<uint16_t LogCnt, uint16_t MaxValue>
constexpr std::array<int16_t, LogCnt> LogScale<LogCnt, MaxValue>::_init_log_table() {
    std::array<int16_t, LogCnt> a{};
    for (size_t i = 0; i < a.size(); ++i) {
        a[i] = log10(1 + 9.f * i / (LogCnt - 1)) * MaxValue;
    }

    return a;
}
