#pragma once

#include <array>
#include <cctype>
#include <cmath>

template<uint16_t SampleSize, uint16_t CosAmount = 4096>
class SpectrumAnalyzer {
    static_assert((SampleSize & (SampleSize - 1)) == 0, "SampleSize must be power of 2");

    constexpr std::array<int16_t, SampleSize> _init_cos_table();
    const std::array<int16_t, SampleSize> _cos_table = _init_cos_table();

    uint16_t _gain;

public:
    static constexpr uint16_t SAMPLE_SIZE = SampleSize;
    static constexpr uint16_t SPECTRUM_SIZE = SampleSize / 2;

    static constexpr uint16_t COS_AMOUNT = CosAmount;

    explicit SpectrumAnalyzer(uint16_t gain = 1);

    void dft(const uint16_t *data, uint16_t *result);

private:
    void _dft(const uint16_t *data, int32_t *result, uint16_t length, uint16_t step);
};

template<uint16_t SampleSize, uint16_t CosAmount>
SpectrumAnalyzer<SampleSize, CosAmount>::SpectrumAnalyzer(uint16_t gain): _gain(gain) {}

template<uint16_t SampleSize, uint16_t CosAmount>
void SpectrumAnalyzer<SampleSize, CosAmount>::dft(const uint16_t *data, uint16_t *result) {
    auto t_begin = micros();

    int32_t temp[SAMPLE_SIZE];
    _dft(data, temp, SAMPLE_SIZE, 1);

    // TODO: 0-nth freq. always too high
    temp[0] = temp[1];

    for (int i = 0; i < SPECTRUM_SIZE; ++i) {
        result[i] = abs(temp[i]) * _gain / SPECTRUM_SIZE;
    }

    D_PRINTF("DFT: %lu us\n", micros() - t_begin);
}

template<uint16_t SampleSize, uint16_t CosAmount>
void SpectrumAnalyzer<SampleSize, CosAmount>::_dft(const uint16_t *in, int32_t *out, uint16_t length, uint16_t step) {
    if (length == 1) {
        *out = *in;
        return;
    }

    length /= 2;

    _dft(in, out, length, 2 * step);
    _dft(in + step, out + length, length, 2 * step);

    size_t period = SAMPLE_SIZE / length;
    for (size_t i = 0, j = 0; i < length; i++, j += period) {
        int32_t temp = out[i + length] * _cos_table[j % SAMPLE_SIZE] / COS_AMOUNT;

        out[i + length] = out[i] - temp;
        out[i] += temp;
    }
}

template<uint16_t SampleSize, uint16_t CosAmount>
constexpr std::array<int16_t, SampleSize> SpectrumAnalyzer<SampleSize, CosAmount>::_init_cos_table() {
    const float const_part = 2 * M_PI / SampleSize;

    std::array<int16_t, SampleSize> a{};
    for (size_t i = 0; i < SampleSize; ++i) {
        a[i] = cosf(const_part * (float) i) * CosAmount;
    }

    return a;
}
