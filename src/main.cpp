#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <ESP8266WiFi.h>
#include <Max72xxPanel.h>

#include "debug.h"

#include "misc/analog.h"
#include "misc/resample.h"
#include "misc/scale.h"
#include "misc/spectrum.h"

constexpr int MATRIX_PIN_CS = 5;
constexpr int MATRIX_WIDTH = 1;
constexpr int MATRIX_HEIGHT = 4;

constexpr int MATRIX_ROTATION = 1;
constexpr int MATRIX_INTENSITY = 0;

const uint16_t FFT_SAMPLE_RATE = 9400;

const uint16_t FFT_SIZE = 256;
const uint16_t FFT_GAIN = 10;
const uint16_t FFT_GATE = 0;

const uint16_t FFT_UPDATE_INTERVAL = 1000 / 15;
const uint16_t RENDER_INTERVAL = 1000 / 30;

const uint16_t ESTIMATED_SAMPLE_READ_TIME = (1000 * FFT_SIZE / FFT_SAMPLE_RATE);
const uint16_t READ_INTERVAL = max(0, FFT_UPDATE_INTERVAL - (int32_t) ESTIMATED_SAMPLE_READ_TIME * 2);

const uint16_t WINDOW_DURATION = 5000;
const uint16_t WINDOW_SAMPLES = WINDOW_DURATION / FFT_UPDATE_INTERVAL;

Max72xxPanel matrix = Max72xxPanel(MATRIX_PIN_CS, MATRIX_WIDTH, MATRIX_HEIGHT);

SpectrumAnalyzer<FFT_SIZE> spectrum_analyzer(FFT_GAIN);
const uint16_t FFT_OUT_SIZE = spectrum_analyzer.SPECTRUM_SIZE;

LogScale log_scale(WINDOW_SAMPLES);

typedef Resample<MATRIX_HEIGHT * 8, FFT_OUT_SIZE> ResampleT;
ResampleT resample(FFT_SAMPLE_RATE / FFT_OUT_SIZE, FFT_SAMPLE_RATE);

AnalogReader reader(FFT_SIZE, A0, FFT_SAMPLE_RATE);

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

void render();

static uint16_t fft_1[FFT_OUT_SIZE];
static uint16_t fft_2[FFT_OUT_SIZE];

uint16_t *current_fft = fft_1;
uint16_t *prev_fft = fft_2;

unsigned long last_render = 0;

void loop() {
    if (millis() - last_render >= RENDER_INTERVAL) {
        render();
        last_render = millis();
    }

    if (millis() - last_fft_update >= READ_INTERVAL) {
        reader.read_if_needed();
    }

    if (millis() - last_fft_update >= FFT_UPDATE_INTERVAL) {
        const auto start = millis();

        uint16_t *tmp = current_fft;
        current_fft = prev_fft;
        prev_fft = tmp;

        spectrum_analyzer.dft(reader.data(), current_fft);
        log_scale.scale(current_fft, FFT_OUT_SIZE);

        last_fft_update = start;

        const auto elapsed = millis() - start;
        if (elapsed > RENDER_INTERVAL) D_PRINTF("OMG! Too long: %lu ms\n", elapsed);
    }
}

void render() {
    matrix.fillScreen(LOW);

    const auto k = min<uint8_t>(255, (millis() - last_fft_update) * 255 / FFT_UPDATE_INTERVAL);

    int prev_index = 0;
    for (int16_t i = 0; i < matrix.width(); ++i) {
        const int index = resample[i];

        int32_t accumulated = 0;
        int32_t significant_cnt = 0;
        for (int j = prev_index; j <= index; ++j) {
            const auto value = prev_fft[j] - ((int32_t) prev_fft[j] - current_fft[j]) * k / 255;
            accumulated += value;

            if (value >= FFT_GATE) ++significant_cnt;
        }

        if (significant_cnt > 0) accumulated /= significant_cnt;
        else accumulated = 0;

        const auto height = (int16_t) (accumulated * matrix.height() / log_scale.MAX_VALUE);
        matrix.drawLine(i, matrix.height(), i, matrix.height() - height, HIGH);

        prev_index = index;
    }

    matrix.write();
}