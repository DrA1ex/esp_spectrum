#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <ESP8266WiFi.h>
#include <Max72xxPanel.h>

#include "debug.h"

#include "spectrum_analyzer.h"
#include "volume_analyzer.h"

#define USE_SPECTRUM

constexpr int MATRIX_PIN_CS = 5;
constexpr int MATRIX_WIDTH = 1;
constexpr int MATRIX_HEIGHT = 4;

constexpr int MATRIX_ROTATION = 1;
constexpr int MATRIX_INTENSITY = 1;

const uint16_t SAMPLE_RATE = 9400;

const uint16_t AUDIO_GAIN = 10;
const uint16_t AUDIO_GATE = 0;

const uint16_t RENDER_INTERVAL = 1000 / 50;

const uint16_t WINDOW_DURATION = 5000;

Max72xxPanel matrix = Max72xxPanel(MATRIX_PIN_CS, MATRIX_WIDTH, MATRIX_HEIGHT);

#if defined(USE_SPECTRUM)
const uint16_t SAMPLE_SIZE = 256;
const uint16_t UPDATE_INTERVAL = 1000 / 15;

SpectrumAnalyzer<SAMPLE_SIZE, MATRIX_HEIGHT * 8, A0> analyzer(
        SAMPLE_RATE, UPDATE_INTERVAL, WINDOW_DURATION, AUDIO_GAIN, AUDIO_GATE);
#else
const uint16_t SAMPLE_SIZE = 16;
const uint16_t UPDATE_INTERVAL = 1000 / 40;

VolumeAnalyzer<SAMPLE_SIZE, MATRIX_HEIGHT * 8, A0> analyzer(
        SAMPLE_RATE, UPDATE_INTERVAL, WINDOW_DURATION, AUDIO_GAIN, AUDIO_GATE);
#endif

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

    analyzer.tick();

    const auto elapsed = millis() - start;
    if (elapsed > RENDER_INTERVAL) D_PRINTF("OMG! Too long: %lu ms\n", elapsed);
}

void render() {
    matrix.fillScreen(LOW);

    const auto k = min<uint8_t>(255, analyzer.delta() * 255 / UPDATE_INTERVAL);

    for (int16_t i = 0; i < matrix.width(); ++i) {
        const auto value = analyzer.get(i, k);
        const auto height = (int16_t) (value * matrix.height() / decltype(analyzer)::MAX_VALUE) / 2;

        matrix.drawLine(i, matrix.height() / 2 - height - 1, i, matrix.height() / 2 + height, HIGH);
    }

    matrix.write();
}