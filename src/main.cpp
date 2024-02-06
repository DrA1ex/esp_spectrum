#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <Adafruit_GFX.h>
#include <Max72xxPanel.h>

#include "debug.h"

void populate_data(uint16_t *result);
void dft(const uint16_t *data, uint16_t *result);

const int pinCS = 5;
const int numberOfHorizontalDisplays = 1;
const int numberOfVerticalDisplays = 4;

Max72xxPanel matrix = Max72xxPanel(pinCS, numberOfHorizontalDisplays, numberOfVerticalDisplays);

const uint16_t FFT_SIZE = 128;
const uint16_t FFT_GAIN = 10;
const uint16_t FFT_GATE = 0;

const int fft_update_period = 1000 / 15;
const int render_interval = 1000 / 30;

unsigned long last_fft_update = 0;

const uint16_t COS_AMOUNT = 4096;
static int16_t cosF[FFT_SIZE];

const uint16_t LOG_AMOUNT = 4096;
const uint16_t LOG_CNT = 255;
static int16_t log10F[LOG_CNT];

static int indexes[numberOfVerticalDisplays * 8];

void setup() {
#if defined(DEBUG)
    Serial.begin(115200);
    while (!Serial);
#endif

    WiFi.mode(WIFI_OFF);

    matrix.setIntensity(10);
    matrix.setRotation(1);

    matrix.fillScreen(LOW);

    const float const_part = 2 * PI / FFT_SIZE;
    for (uint16_t i = 0; i < FFT_SIZE; ++i) {
        cosF[i] = cosf(const_part * i) * COS_AMOUNT;
    }

    for (int i = 0; i < LOG_CNT; ++i) {
        log10F[i] = log10(1 + 9.f * i / LOG_CNT) * LOG_AMOUNT;
    }

    for (int i = 0; i < matrix.width(); ++i) {
        float value = (float) i / (matrix.width() - 1);
        indexes[i] = floor((1 - log10(10 - value * 9)) * (FFT_SIZE / 2 - 1));
    }

    D_PRINT("Initialized");
}

static uint16_t fft_1[FFT_SIZE / 2];
static uint16_t fft_2[FFT_SIZE / 2];

uint16_t *current_fft = fft_1;
uint16_t *prev_fft = fft_2;

void loop() {
    const auto start = millis();

    if (start - last_fft_update > fft_update_period) {
        uint16_t *tmp = current_fft;
        current_fft = prev_fft;
        prev_fft = tmp;

        populate_data(current_fft);
        last_fft_update = start;
    }


    const uint8_t k = (start - last_fft_update) * 255 / fft_update_period;

    const auto elapsed = millis() - start;

    matrix.fillScreen(LOW);

    int prev_index = 0;
    for (int i = 0; i < matrix.width(); ++i) {
        const int index = indexes[i];

        int32_t accumulated = 0;
        int32_t significant_cnt = 0;
        for (int j = prev_index; j <= index; ++j) {
            const auto value = prev_fft[j] - ((int32_t) prev_fft[j] - current_fft[j]) * k / 255;
            accumulated += value;

            if (value < LOG_AMOUNT - FFT_GATE) ++significant_cnt;
        }

        if (significant_cnt > 0) accumulated /= significant_cnt;
        else accumulated = LOG_AMOUNT;

        const int height = accumulated * matrix.height() / LOG_AMOUNT;
        matrix.drawLine(i, matrix.height(), i, height, HIGH);

        prev_index = index;
    }

    matrix.write();

    if (elapsed < render_interval) {
        delay(render_interval - elapsed);
    } else {
        D_PRINTF("OMG! Too long: %lu\n", elapsed);
    }
}

static uint16_t g_max = 0, g_min = 1024;

void populate_data(uint16_t *result) {
    static uint16_t data[FFT_SIZE], out[FFT_SIZE / 2];

    auto t_begin = micros();
    for (auto &value: data) {
        value = analogRead(0);
    }

    // TOO FAST
    //system_adc_read_fast(data, FFT_SIZE, 24);

    {
        int32_t d_min = 1024, d_max = 0;
        for (auto value: data) {
            if (value < d_min) d_min = value;
            if (value > d_max) d_max = value;
        }

        VERBOSE(D_PRINTF("SIGNAL: %u..%u\n", d_min, d_max));
    }

    auto t_delta = micros() - t_begin;
    VERBOSE(D_PRINTF("Data populating: %lu us\n", t_delta));
    const auto sample_rate = 1000000 * FFT_SIZE / (micros() - t_begin);
    VERBOSE(D_PRINTF("Sample rate: %lu Hz\n", sample_rate));
    VERBOSE(D_PRINTF("Spectrum sample rate: %lu Hz, Band size: %lu Hz\n", sample_rate / 2, sample_rate / 2 / (FFT_SIZE / 2)));

    t_begin = micros();

    dft(data, out);
    out[0] = out[1];

    D_PRINTF("DFT: %lu us\n", micros() - t_begin);

    t_begin = micros();

    int32_t v_min = 1024, v_max = 0;
    for (auto value: out) {
        if (value < v_min) v_min = value;
        if (value > v_max) v_max = value;
    }

    if (v_min < g_min) g_min = v_min;
    if (v_max > g_max) g_max = v_max;

    D_PRINTF("SPECTRAL: %u..%u\n", v_min, v_max);

    auto g_peakToPeak = g_max - g_min;

    for (int i = 0; i < FFT_SIZE / 2; ++i) {
        const auto signal = out[i] - g_min;
        const uint16_t rel_value = g_peakToPeak > 0 ? signal * LOG_CNT / g_peakToPeak : 0;
        result[i] = LOG_AMOUNT - log10F[rel_value];
    }

    g_min += 1;
    if (g_max > 0) g_max -= 1;

    D_PRINTF("Spectral processing: %lu us\n", micros() - t_begin);
}
void dft(const uint16_t *data, uint16_t *result) {
    for (uint16_t freq = 0; freq < FFT_SIZE / 2; ++freq) {
        int32_t freq_amp = 0;

        for (uint16_t i = 0; i < FFT_SIZE; ++i) {
            freq_amp += (int32_t) data[i] * cosF[(freq * i) % FFT_SIZE];
        }

        result[freq] = abs(freq_amp) * FFT_GAIN / COS_AMOUNT / (FFT_SIZE / 2);
    }
}