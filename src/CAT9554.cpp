#include <Wire.h>
#include "CAT9554.h"

bool CAT9554::updateGpio = false;

uint32_t i2c_buffer = 0;
bool I2cValidRead(uint8_t addr, uint8_t reg, uint8_t size)
{
    uint8_t retry = I2C_RETRY_COUNTER;
    bool status = false;

    i2c_buffer = 0;
    while (!status && retry)
    {
        if (retry != I2C_RETRY_COUNTER)
        {
            delayMicroseconds(10 * 1000);
        }
        Wire.beginTransmission(addr); // start transmission to device
        Wire.write(reg);              // sends register address to read from
        if (0 == Wire.endTransmission(false))
        {                                           // Try to become I2C Master, send data and collect bytes, keep master status for next request...
            Wire.requestFrom((int)addr, (int)size); // send data n-bytes read
            if (Wire.available() == size)
            {
                for (uint32_t i = 0; i < size; i++)
                {
                    i2c_buffer = i2c_buffer << 8 | Wire.read(); // receive DATA
                }
                status = true;
            }
        }
        retry--;
    }
    return status;
}

bool I2cWrite(uint8_t addr, uint8_t reg, uint32_t val, uint8_t size)
{
    uint8_t x = I2C_RETRY_COUNTER;
    do
    {
        if (x != I2C_RETRY_COUNTER)
        {
            delayMicroseconds(10 * 1000);
        }
        Wire.beginTransmission((uint8_t)addr); // start transmission to device
        Wire.write(reg);                       // sends register address to write to
        uint8_t bytes = size;
        while (bytes--)
        {
            Wire.write((val >> (8 * bytes)) & 0xFF); // write data
        }
        x--;
    } while (Wire.endTransmission(true) != 0 && x != 0); // end transmission
    return (x);
}

CAT9554::CAT9554(uint8_t sda, uint8_t scl, uint16_t frequency)
{
    Wire.begin(sda, scl);
    Wire.setClock(frequency);
}

bool CAT9554::readByte(uint8_t reg, uint8_t *data)
{
    bool status = I2cValidRead(CAT9554_ADDRESS, reg, 1);
    *data = (uint8_t)i2c_buffer;
    return status;
}

bool CAT9554::writeByte(uint8_t reg, uint16_t value)
{
    return I2cWrite(CAT9554_ADDRESS, reg, value, 1);
}

bool CAT9554::readGpio()
{
    uint8_t data;
    if (!this->readByte(INPUT_REG, &data))
    {
        return false;
    }
    this->inputMask = data;
    return true;
}

bool CAT9554::writeGpio()
{
    if (!this->writeByte(OUTPUT_REG, this->outputMask))
    {
        return false;
    }
    return true;
}

bool CAT9554::configGpio()
{
    if (!this->writeByte(INPUT_REG, this->configMask))
    {
        return false;
    }
    if (!this->writeByte(CONFIG_REG, this->configMask))
    {
        return false;
    }
    if (!this->writeByte(INPUT_REG, 0x00))
    {
        return false;
    }
    return true;
}

bool CAT9554::readConfig()
{
    uint8_t data;
    if (!this->readByte(CONFIG_REG, &data))
    {
        return false;
    }
    this->configMask = data;
    return true;
}

void ICACHE_RAM_ATTR CAT9554::gpioIntr()
{
    CAT9554::updateGpio = true;
}

void CAT9554::setup()
{
    if (!this->readGpio())
    {
        return;
    }

    if (this->enableIrq)
    {
        attachInterrupt(this->irqPin, gpioIntr, FALLING);
        CAT9554::updateGpio = false;
    }
    this->readGpio();
    this->readConfig();
}

bool CAT9554::digitalRead(uint8_t pin)
{
    if (!this->enableIrq || CAT9554::updateGpio)
    {
        this->readGpio();
        this->updateGpio = false;
    }
    return this->inputMask & (1 << pin);
}

bool CAT9554::digitalWrite(uint8_t pin, bool value)
{
    if (value)
    {
        this->outputMask |= (1 << pin);
    }
    else
    {
        this->outputMask &= ~(1 << pin);
    }
    return this->writeGpio();
}

void CAT9554::pinMode(uint8_t pin, uint8_t mode)
{
    if (mode == CAT9554_INPUT)
    {
        // Clear mode mask bit
        this->configMask |= (1 << pin);
    }
    else if (mode == CAT9554_OUTPUT)
    {
        // Set mode mask bit
        this->configMask &= ~(1 << pin);
    }
    this->configGpio();
}
