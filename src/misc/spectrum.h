#pragma once

#include <array>
#include <cctype>
#include <cmath>

template<uint16_t SampleSize>
class SpectrumAnalyzer {
    static_assert(SampleSize > 0 && SampleSize % 2 == 0, "SampleSize must be power of 2");

public:
    static constexpr uint16_t SAMPLE_SIZE = SampleSize;
    static constexpr uint16_t SPECTRUM_SIZE = SampleSize / 2;

    static constexpr uint16_t COS_AMOUNT = 4096;

    explicit SpectrumAnalyzer(uint16_t gain = 1);

    void dft(const uint16_t *data, uint16_t *result);

private:
    constexpr std::array<int16_t, SampleSize> _init_cos_table();

    const std::array<int16_t, SampleSize> _cos_table = _init_cos_table();

    uint16_t _gain;
};

template<uint16_t SampleSize>
SpectrumAnalyzer<SampleSize>::SpectrumAnalyzer(uint16_t gain): _gain(gain) {}

template<uint16_t SampleSize>
void SpectrumAnalyzer<SampleSize>::dft(const uint16_t *data, uint16_t *result) {
    auto t_begin = micros();

    for (uint16_t freq = 0; freq < SPECTRUM_SIZE; ++freq) {
        int32_t freq_amp = 0;

        for (uint16_t i = 0; i < SAMPLE_SIZE; ++i) {
            freq_amp += (int32_t) data[i] * _cos_table[(freq * i) % SAMPLE_SIZE];
        }

        result[freq] = abs(freq_amp) * _gain / COS_AMOUNT / SPECTRUM_SIZE;
    }

    // TODO: 0-nth freq. always too big
    result[0] = result[1];

    D_PRINTF("DFT: %lu us\n", micros() - t_begin);
}

template<uint16_t SampleSize>
constexpr std::array<int16_t, SampleSize> SpectrumAnalyzer<SampleSize>::_init_cos_table() {
    const float const_part = 2 * M_PI / SampleSize;

    std::array<int16_t, SampleSize> a{};
    for (size_t i = 0; i < a.size(); ++i) {
        a[i] = cosf(const_part * (float) i) * COS_AMOUNT;
    }
    return a;
}
