/*
 * GPIO Control - Pin manipulation via CLI/MQTT/Web
 */

#ifndef GPIO_CONTROL_H
#define GPIO_CONTROL_H

#include <Arduino.h>

class GPIOControl {
public:
    static void init();

    static bool setPin(uint8_t pin, bool value);
    static bool setPinPWM(uint8_t pin, uint16_t value);
    static int readPin(uint8_t pin);
    static int readAnalog();

    static bool setPinMode(uint8_t pin, uint8_t mode);
    static bool isValidPin(uint8_t pin);

    static String getStatus();

private:
    static const uint8_t _validPins[];
    static const uint8_t _validPinCount;
};

#endif // GPIO_CONTROL_H
