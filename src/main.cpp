#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <ESP8266WiFi.h>
#include <Max72xxPanel.h>

#include "debug.h"

#include "misc/resample.h"
#include "misc/spectrum.h"
#include "misc/analog.h"

constexpr int MATRIX_PIN_CS = 5;
constexpr int MATRIX_WIDTH = 1;
constexpr int MATRIX_HEIGHT = 4;

constexpr int MATRIX_ROTATION = 1;
constexpr int MATRIX_INTENSITY = 10;

Max72xxPanel matrix = Max72xxPanel(MATRIX_PIN_CS, MATRIX_WIDTH, MATRIX_HEIGHT);

const uint16_t FFT_SAMPLE_RATE = 9400;
const uint16_t FFT_SAMPLE_INTERVAL_US = (uint32_t) 1000000 / FFT_SAMPLE_RATE;

const uint16_t FFT_SIZE = 128;
const uint16_t FFT_GAIN = 10;
const int16_t FFT_GATE = 0;

SpectrumAnalyzer<FFT_SIZE> spectrum_analyzer(FFT_GAIN, FFT_GATE);
const uint16_t FFT_OUT_SIZE = spectrum_analyzer.SPECTRUM_SIZE;

Resample<MATRIX_HEIGHT * 8, FFT_OUT_SIZE> resample(FFT_SAMPLE_RATE / FFT_OUT_SIZE, FFT_SAMPLE_RATE);


const int fft_update_period = 1000 / 15;
const int render_interval = 1000 / 30;

unsigned long last_fft_update = 0;

void setup() {
#if defined(DEBUG)
    Serial.begin(115200);
    while (!Serial);
#endif

    WiFi.mode(WIFI_OFF);

    matrix.setIntensity(MATRIX_INTENSITY);
    matrix.setRotation(MATRIX_ROTATION);

    D_PRINT("Initialized");
}

void populate_data(uint16_t *result);
void render();

static uint16_t fft_1[FFT_OUT_SIZE];
static uint16_t fft_2[FFT_OUT_SIZE];

uint16_t *current_fft = fft_1;
uint16_t *prev_fft = fft_2;
uint8_t k = 0;

void loop() {
    const auto start = millis();

    render();

    if (start - last_fft_update >= fft_update_period) {
        uint16_t *tmp = current_fft;
        current_fft = prev_fft;
        prev_fft = tmp;

        populate_data(current_fft);
        last_fft_update = start;
    }

    k = (start - last_fft_update) * 255 / fft_update_period;

    const auto elapsed = millis() - start;

    if (elapsed < render_interval) {
        delay(render_interval - elapsed);
    } else {
        D_PRINTF("OMG! Too long: %lu ms\n", elapsed);
    }
}

static uint16_t g_max = 0, g_min = 1024;

void populate_data(uint16_t *result) {
    static uint16_t data[FFT_SIZE];

    read_analog_data(data, FFT_SIZE, FFT_SAMPLE_RATE);

    spectrum_analyzer.dft(data, result);

    auto t_begin = micros();

    int32_t v_min = 1024, v_max = 0;
    for (int i = 0; i < FFT_OUT_SIZE; ++i) {
        const auto value = result[i];

        if (value < v_min) v_min = value;
        if (value > v_max) v_max = value;
    }

    if (v_min < g_min) g_min = v_min;
    if (v_max > g_max) g_max = v_max;

    VERBOSE(D_PRINTF("SPECTRAL: %u..%u\n", v_min, v_max));

    spectrum_analyzer.scale(result, g_min, g_max);

    g_min += 1;
    if (g_max > 0) g_max -= 1;

    D_PRINTF("Spectral processing: %lu us\n", micros() - t_begin);
}

void render() {
    matrix.fillScreen(LOW);

    int prev_index = 0;
    for (int16_t i = 0; i < matrix.width(); ++i) {
        const int index = resample.index_table[i];

        int32_t accumulated = 0;
        int32_t significant_cnt = 0;
        for (int j = prev_index; j <= index; ++j) {
            const auto value = prev_fft[j] - ((int32_t) prev_fft[j] - current_fft[j]) * k / 255;
            accumulated += value;

            if (value >= FFT_GATE) ++significant_cnt;
        }

        if (significant_cnt > 0) accumulated /= significant_cnt;
        else accumulated = 0;

        const auto height = (int16_t) (accumulated * matrix.height() / spectrum_analyzer.MAX_VALUE);
        matrix.drawLine(i, matrix.height(), i, matrix.height() - height, HIGH);

        prev_index = index;
    }

    matrix.write();
}