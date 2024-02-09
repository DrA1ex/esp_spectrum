#pragma once

#include <Arduino.h>

#include "debug.h"

class AnalogSample {
    uint16_t *_data;
    size_t _size;
    size_t _offset;

public:
    AnalogSample(uint16_t *data, size_t size, size_t offset) : _data(data), _size(size), _offset(offset) {};

    AnalogSample(const AnalogSample &) = delete;
    AnalogSample &operator=(const AnalogSample &) = delete;

    [[nodiscard]] inline size_t size() const { return _size; }

    inline uint16_t &operator[](size_t index) { return const_cast<uint16_t &>(std::as_const(*this)[index]); }

    inline const uint16_t &operator[](size_t index) const {
        auto _pos = index + _offset;
        return _data[_pos >= _size ? _pos - _size : _pos];
    }
};

class AnalogReader {
    size_t _size;
    uint16_t *_data;

    uint8_t _pin;
    uint16_t _sample_rate;
    uint16_t _read_interval;

    unsigned long _last_read_time = 0;
    size_t _index = 0;

public:
    explicit AnalogReader(size_t size, uint8_t pin, uint16_t sample_rate);
    ~AnalogReader();

    void tick();

    [[nodiscard]] inline AnalogSample get() { return {_data, _size, _index}; };
};

AnalogReader::AnalogReader(size_t size, uint8_t pin, uint16_t sample_rate) : _size(size), _pin(pin), _sample_rate(sample_rate) {
    _data = new uint16_t[size];
    _read_interval = (uint32_t) 1000000 / _sample_rate;
}

AnalogReader::~AnalogReader() {
    delete[] _data;
}

void AnalogReader::tick() {
    if (micros() - _last_read_time < _read_interval) return;

    _data[_index] = analogRead(_pin);
    _last_read_time = micros();

    if (++_index == _size) _index = 0;
}