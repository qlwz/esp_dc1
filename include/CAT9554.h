#ifndef _CAT9554_h
#define _CAT9554_h

#include "Arduino.h"

/// Modes for CAT9554 pins
enum CAT9554GPIOMode : uint8_t
{
    CAT9554_INPUT = INPUT,
    CAT9554_OUTPUT = OUTPUT,
};

enum CAT9554Commands
{
    INPUT_REG = 0x00,
    OUTPUT_REG = 0x01,
    CONFIG_REG = 0x03,
};

#define CAT9554_ADDRESS 0x20
#define I2C_RETRY_COUNTER 5

class CAT9554
{
protected:
    bool readByte(uint8_t reg, uint8_t *data);
    bool writeByte(uint8_t reg, uint16_t value);
    bool readGpio();
    bool writeGpio();
    bool configGpio();
    bool readConfig();

    /// Mask for the pin mode - 1 means output, 0 means input
    uint16_t configMask;
    /// The mask to write as output state - 1 means HIGH, 0 means LOW
    uint16_t outputMask;
    /// The state read in read_gpio_ - 1 means HIGH, 0 means LOW
    uint16_t inputMask;
    /// IRQ is enabled.
    bool enableIrq;
    /// IRQ pin.
    uint8_t irqPin;
    static void gpioIntr();

public:
    CAT9554(uint8_t sda, uint8_t scl, uint16_t frequency = 20000);

    /// Need update GPIO
    static bool updateGpio;
    /// Check i2c availability and setup masks
    void setup();
    /// Helper function to read the value of a pin.
    bool digitalRead(uint8_t pin);
    /// Helper function to write the value of a pin.
    bool digitalWrite(uint8_t pin, bool value);
    /// Helper function to set the pin mode of a pin.
    void pinMode(uint8_t pin, uint8_t mode);
    /// Setup irq pin.
    void setIrqPin(uint8_t irq_pin)
    {
        enableIrq = true;
        irqPin = irq_pin;
    };
};
#endif