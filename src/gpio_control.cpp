/*
 * GPIO Control Implementation
 */

#include "gpio_control.h"

const uint8_t GPIOControl::_validPins[] = {0, 2, 4, 5, 12, 13, 14, 15, 16};
const uint8_t GPIOControl::_validPinCount = sizeof(_validPins) / sizeof(_validPins[0]);

void GPIOControl::init() {
    // Set LED pin as output (active low on most ESP boards)
    pinMode(STATUS_LED_PIN, OUTPUT);
    digitalWrite(STATUS_LED_PIN, HIGH); // Off
    Serial.println(F("[GPIO] GPIO control initialized"));
}

bool GPIOControl::isValidPin(uint8_t pin) {
    for (uint8_t i = 0; i < _validPinCount; i++) {
        if (_validPins[i] == pin) return true;
    }
    return false;
}

bool GPIOControl::setPin(uint8_t pin, bool value) {
    if (!isValidPin(pin)) return false;
    pinMode(pin, OUTPUT);
    digitalWrite(pin, value ? HIGH : LOW);
    Serial.printf("[GPIO] Pin %d = %d\n", pin, value);
    return true;
}

bool GPIOControl::setPinPWM(uint8_t pin, uint16_t value) {
    if (!isValidPin(pin)) return false;
    if (pin == 16) return false; // GPIO16 doesn't support PWM
    pinMode(pin, OUTPUT);
    analogWrite(pin, value);
    Serial.printf("[GPIO] Pin %d PWM = %d\n", pin, value);
    return true;
}

int GPIOControl::readPin(uint8_t pin) {
    if (!isValidPin(pin)) return -1;
    pinMode(pin, INPUT);
    return digitalRead(pin);
}

int GPIOControl::readAnalog() {
    return analogRead(A0);
}

bool GPIOControl::setPinMode(uint8_t pin, uint8_t mode) {
    if (!isValidPin(pin)) return false;
    pinMode(pin, mode);
    return true;
}

String GPIOControl::getStatus() {
    String status = "GPIO Status:\n";
    for (uint8_t i = 0; i < _validPinCount; i++) {
        uint8_t pin = _validPins[i];
        status += "  GPIO" + String(pin) + ": " + String(digitalRead(pin)) + "\n";
    }
    status += "  A0: " + String(analogRead(A0)) + "\n";
    return status;
}
