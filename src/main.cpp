#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <ESP8266WiFi.h>
#include <Max72xxPanel.h>

#include "debug.h"
#include "spectrum_analyzer.h"

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

const uint16_t WINDOW_DURATION = 5000;

Max72xxPanel matrix = Max72xxPanel(MATRIX_PIN_CS, MATRIX_WIDTH, MATRIX_HEIGHT);

SpectrumAnalyzer<FFT_SIZE, MATRIX_HEIGHT * 8, A0> spectrum_analyzer(
        FFT_SAMPLE_RATE, FFT_UPDATE_INTERVAL, WINDOW_DURATION, FFT_GAIN, FFT_GATE);

unsigned long last_render = 0;

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

void loop() {
    const auto start = millis();

    if (millis() - last_render >= RENDER_INTERVAL) {
        render();
        last_render = millis();
    }

    spectrum_analyzer.tick();

    const auto elapsed = millis() - start;
    if (elapsed > RENDER_INTERVAL) D_PRINTF("OMG! Too long: %lu ms\n", elapsed);
}

void render() {
    matrix.fillScreen(LOW);

    const auto k = min<uint8_t>(255, spectrum_analyzer.delta() * 255 / FFT_UPDATE_INTERVAL);

    for (int16_t i = 0; i < matrix.width(); ++i) {
        const auto value = spectrum_analyzer.get(i, k);
        const auto height = (int16_t) (value * matrix.height() / decltype(spectrum_analyzer)::MAX_VALUE);

        matrix.drawLine(i, matrix.height(), i, matrix.height() - height, HIGH);
    }

    matrix.write();
}