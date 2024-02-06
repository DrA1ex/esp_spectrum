#pragma once

#include <cctype>
#include <cmath>

template<uint16_t SampleSize>
class SpectrumAnalyzer {
public:
    static constexpr uint16_t SAMPLE_SIZE = SampleSize;
    static constexpr uint16_t SPECTRUM_SIZE = SampleSize / 2;

    static const uint16_t COS_AMOUNT = 4096;
    static const uint16_t LOG_CNT = 256;

    static const uint16_t MAX_VALUE = 4096;

    explicit SpectrumAnalyzer(uint16_t gain = 1, uint16_t gate = 0);

    void dft(const uint16_t *data, uint16_t *result);
    void scale(uint16_t *spectrum, uint16_t min, uint16_t max);

private:
    constexpr std::array<int16_t, SPECTRUM_SIZE> _init_cos_table();
    constexpr std::array<int16_t, LOG_CNT> _init_log_table();

    const std::array<int16_t, LOG_CNT> _log_table = _init_log_table();
    const std::array<int16_t, SPECTRUM_SIZE> _cos_table = _init_cos_table();

    uint16_t _gain;
    uint16_t _gate;
};

template<uint16_t SampleSize>
SpectrumAnalyzer<SampleSize>::SpectrumAnalyzer(uint16_t gain, uint16_t gate): _gain(gain), _gate(gate) {}

template<uint16_t SampleSize>
void SpectrumAnalyzer<SampleSize>::dft(const uint16_t *data, uint16_t *result) {
    for (uint16_t freq = 0; freq < SPECTRUM_SIZE; ++freq) {
        int32_t freq_amp = 0;

        for (uint16_t i = 0; i < SAMPLE_SIZE; ++i) {
            freq_amp += (int32_t) data[i] * _cos_table[(freq * i) % SAMPLE_SIZE];
        }

        result[freq] = abs(freq_amp) * _gain / COS_AMOUNT / SPECTRUM_SIZE;
    }

    // TODO: 0-nth freq. always too big
    result[0] = result[1];
}

template<uint16_t SampleSize>
void SpectrumAnalyzer<SampleSize>::scale(uint16_t *spectrum, uint16_t min, uint16_t max) {
    const auto width = max - min;
    if (width == 0) return;

    for (int i = 0; i < SPECTRUM_SIZE; ++i) {
        const uint16_t rel_value = (spectrum[i] - min) * LOG_CNT / width;
        spectrum[i] = _log_table[rel_value];
    }
}

template<uint16_t SampleSize>
constexpr std::array<int16_t, SpectrumAnalyzer<SampleSize>::SPECTRUM_SIZE> SpectrumAnalyzer<SampleSize>::_init_cos_table() {
    const float const_part = 2 * M_PI / SAMPLE_SIZE;

    std::array<int16_t, SPECTRUM_SIZE> a{};
    for (size_t i = 0; i < a.size(); ++i) {
        a[i] = cosf(const_part * i) * COS_AMOUNT;
    }
    return a;
}

template<uint16_t SampleSize>
constexpr std::array<int16_t, SpectrumAnalyzer<SampleSize>::LOG_CNT> SpectrumAnalyzer<SampleSize>::_init_log_table() {
    std::array<int16_t, LOG_CNT> a{};
    for (size_t i = 0; i < a.size(); ++i) {
        a[i] = log10(1 + 9.f * i / (LOG_CNT - 1)) * MAX_VALUE;
    }

    return a;
}
