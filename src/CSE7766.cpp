#include "CSE7766.h"

CSE7766::CSE7766(uint8_t rxPin, uint16_t baudrate)
{
    this->serial = new SoftwareSerial(rxPin, SW_SERIAL_UNUSED_PIN, false, 32);
    this->serial->enableIntTx(false);
    this->serial->begin(baudrate);
    Cse.power_invalid = CSE_MAX_INVALID_POWER;
}

void CSE7766::cseReceived()
{
    //  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23
    // F2 5A 02 F7 60 00 03 61 00 40 10 05 72 40 51 A6 58 63 10 1B E1 7F 4D 4E - F2 = Power cycle exceeds range - takes too long - No load
    // 55 5A 02 F7 60 00 03 5A 00 40 10 04 8B 9F 51 A6 58 18 72 75 61 AC A1 30 - 55 = Ok, 61 = Power not valid (load below 5W)
    // 55 5A 02 F7 60 00 03 AB 00 40 10 02 60 5D 51 A6 58 03 E9 EF 71 0B 7A 36 - 55 = Ok, 71 = Ok
    // Hd Id VCal---- Voltage- ICal---- Current- PCal---- Power--- Ad CF--- Ck

    uint8_t header = this->rawData[0];
    if ((header & 0xFC) == 0xFC)
    {
        //Debug::AddInfo(PSTR("CSE: Abnormal hardware"));
        return;
    }

    // Get chip calibration data (coefficients) and use as initial defaults
    if (HLW_UREF_PULSE == this->voltageCalibration)
    {
        long voltage_coefficient = 191200; // uSec
        if (CSE_NOT_CALIBRATED != header)
        {
            voltage_coefficient = this->rawData[2] << 16 | this->rawData[3] << 8 | this->rawData[4];
        }
        this->voltageCalibration = voltage_coefficient / CSE_UREF;
    }
    if (HLW_IREF_PULSE == this->currentCalibration)
    {
        long current_coefficient = 16140; // uSec
        if (CSE_NOT_CALIBRATED != header)
        {
            current_coefficient = this->rawData[8] << 16 | this->rawData[9] << 8 | this->rawData[10];
        }
        this->currentCalibration = current_coefficient;
    }
    if (HLW_PREF_PULSE == this->powerCalibration)
    {
        long power_coefficient = 5364000; // uSec
        if (CSE_NOT_CALIBRATED != header)
        {
            power_coefficient = this->rawData[14] << 16 | this->rawData[15] << 8 | this->rawData[16];
        }
        this->powerCalibration = power_coefficient / CSE_PREF;
    }

    uint8_t adjustement = this->rawData[20];
    Cse.voltage_cycle = this->rawData[5] << 16 | this->rawData[6] << 8 | this->rawData[7];
    Cse.current_cycle = this->rawData[11] << 16 | this->rawData[12] << 8 | this->rawData[13];
    Cse.power_cycle = this->rawData[17] << 16 | this->rawData[18] << 8 | this->rawData[19];
    Cse.cf_pulses = this->rawData[21] << 8 | this->rawData[22];

    if (adjustement & 0x40) // Voltage valid
    {
        Energy.voltage = (float)(this->voltageCalibration * CSE_UREF) / (float)Cse.voltage_cycle;
    }
    if (adjustement & 0x10) // Power valid
    {
        Cse.power_invalid = 0;
        if ((header & 0xF2) == 0xF2) // Power cycle exceeds range
        {
            Energy.active_power = 0;
        }
        else
        {
            if (0 == Cse.power_cycle_first)
            {
                Cse.power_cycle_first = Cse.power_cycle;
            } // Skip first incomplete Cse.power_cycle
            if (Cse.power_cycle_first != Cse.power_cycle)
            {
                Cse.power_cycle_first = -1;
                Energy.active_power = (float)(this->powerCalibration * CSE_PREF) / (float)Cse.power_cycle;
            }
            else
            {
                Energy.active_power = 0;
            }
        }
    }
    else
    {
        if (Cse.power_invalid < CSE_MAX_INVALID_POWER) // Allow measurements down to about 1W
        {
            Cse.power_invalid++;
        }
        else
        {
            Cse.power_cycle_first = 0;
            Energy.active_power = 0; // Powered on but no load
        }
    }
    if (adjustement & 0x20) // Current valid
    {
        if (0 == Energy.active_power)
        {
            Energy.current = 0;
        }
        else
        {
            Energy.current = (float)this->currentCalibration / (float)Cse.current_cycle;
        }
    }
}

void CSE7766::loop()
{
    while (this->serial->available())
    {
        uint8_t serial_in_byte = this->serial->read();

        if (Cse.received)
        {
            this->rawData[this->rawDataIndex++] = serial_in_byte;
            if (24 == this->rawDataIndex)
            {
                //char strHex[100];
                //hex2Str(this->rawData, 23, strHex, true);
                //Debug::AddInfo(PSTR("CSE7766 Data: %s"), strHex);
                uint8_t checksum = 0;
                for (uint8_t i = 2; i < 23; i++)
                {
                    checksum += this->rawData[i];
                }
                if (checksum == this->rawData[23])
                {
                    Energy.data_valid = 0;
                    cseReceived();
                    Cse.received = false;
                    this->rawDataIndex = 0;
                }
                else
                {
                    do
                    { // Sync buffer with data (issue #1907 and #3425)
                        memmove(this->rawData, this->rawData + 1, 24);
                        this->rawDataIndex--;
                    } while ((this->rawDataIndex > 2) && (0x5A != this->rawData[1]));
                    if (0x5A != this->rawData[1])
                    {
                        Cse.received = false;
                        this->rawDataIndex = 0;
                    }
                }
            }
        }
        else
        {
            if ((0x5A == serial_in_byte) && (1 == this->rawDataIndex)) // 0x5A - Packet header 2
            {
                Cse.received = true;
            }
            else
            {
                this->rawDataIndex = 0;
            }
            this->rawData[this->rawDataIndex++] = serial_in_byte;
        }
    }
}

bool CSE7766::everySecond()
{
    if (Energy.data_valid <= ENERGY_WATCHDOG)
    {
        Energy.data_valid++;
        if (Energy.data_valid > ENERGY_WATCHDOG)
        {
            // Reset energy registers
            Energy.voltage = 0;
            Energy.current = 0;
            Energy.active_power = 0;

            if (!isnan(Energy.apparent_power))
            {
                Energy.apparent_power = NAN;
            }
            if (!isnan(Energy.reactive_power))
            {
                Energy.reactive_power = NAN;
            }
            if (!isnan(Energy.power_factor))
            {
                Energy.power_factor = NAN;
            }
        }
    }

    if (Energy.data_valid > ENERGY_WATCHDOG)
    {
        Cse.voltage_cycle = 0;
        Cse.current_cycle = 0;
        Cse.power_cycle = 0;
    }
    else
    {
        long cf_frequency = 0;

        if (CSE_PULSES_NOT_INITIALIZED == Cse.cf_pulses_last_time)
        {
            Cse.cf_pulses_last_time = Cse.cf_pulses; // Init after restart
        }
        else
        {
            if (Cse.cf_pulses < Cse.cf_pulses_last_time)
            { // Rolled over after 65535 pulses
                cf_frequency = (65536 - Cse.cf_pulses_last_time) + Cse.cf_pulses;
            }
            else
            {
                cf_frequency = Cse.cf_pulses - Cse.cf_pulses_last_time;
            }
            if (cf_frequency && Energy.active_power)
            {
                unsigned long delta = (cf_frequency * this->powerCalibration) / 36;
                // prevent invalid load delta steps even checksum is valid (issue #5789):
                //        if (delta <= (3680*100/36) * 10 ) {  // max load for S31/Pow R2: 3.68kW
                // prevent invalid load delta steps even checksum is valid but allow up to 4kW (issue #7155):
                if (delta <= (4000 * 100 / 36) * 10)
                { // max load for S31/Pow R2: 4.00kW
                    Cse.cf_pulses_last_time = Cse.cf_pulses;
                    Energy.kWhtoday_delta += delta;
                }
                else
                {
                    //Debug::AddInfo(PSTR("CSE: Load overflow"));
                    Cse.cf_pulses_last_time = CSE_PULSES_NOT_INITIALIZED;
                }
                //EnergyUpdateToday();
                return true;
            }
        }
    }
    return false;
}