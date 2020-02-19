#ifndef _CSE7766_h
#define _CSE7766_h

#include "Arduino.h"
#include <SoftwareSerial.h>

#define CSE_MAX_INVALID_POWER 128 // Number of invalid power receipts before deciding active power is zero
#define CSE_NOT_CALIBRATED 0xAA
#define CSE_PULSES_NOT_INITIALIZED -1
#define CSE_PREF 1000
#define CSE_UREF 100

#define ENERGY_WATCHDOG 4           // Allow up to 4 seconds before deciding no valid data present

const uint16_t MAX_POWER_HOLD = 10;         // Time in SECONDS to allow max agreed power
const uint16_t MAX_POWER_WINDOW = 30;       // Time in SECONDS to disable allow max agreed power
const uint8_t MAX_POWER_RETRY = 5;          // Retry count allowing agreed power limit overflow

typedef struct _CSE
{
    long voltage_cycle = 0;
    long current_cycle = 0;
    long power_cycle = 0;
    long power_cycle_first = 0;
    long cf_pulses = 0;
    long cf_pulses_last_time = CSE_PULSES_NOT_INITIALIZED;

    uint8_t power_invalid = 0;
    bool received = false;
} CSE;

typedef struct _ENERGY
{
    float voltage = 0.0f;       // 123.1 V
    float current = 0.0f;       // 123.123 A
    float active_power = 0.0f;  // 123.1 W
    float apparent_power = NAN; // 123.1 VA
    float reactive_power = NAN; // 123.1 VAr
    float power_factor = NAN;   // 0.12

    float daily = 0; // 123.123 kWh
    float total = 0; // 12345.12345 kWh total energy

    unsigned long kWhtoday_delta = 0; // 1212312345 Wh 10^-5 (deca micro Watt hours) - Overflows to Energy.kWhtoday (HLW and CSE only)
    unsigned long kWhtoday;           // 12312312 Wh * 10^-2 (deca milli Watt hours) - 5764 = 0.05764 kWh = 0.058 kWh = Energy.daily

    uint8_t data_valid = 0;

    uint16_t power_history[3] = {0};
    uint8_t power_steady_counter = 8; // Allow for power on stabilization
    bool power_delta = false;

    uint16_t mplh_counter = 0;
    uint16_t mplw_counter = 0;
    uint8_t mplr_counter = 0;
    uint8_t mplv_counter = 0;
} ENERGY;

const uint32_t HLW_PREF_PULSE = 12530; // was 4975us = 201Hz = 1000W
const uint32_t HLW_UREF_PULSE = 1950;  // was 1666us = 600Hz = 220V
const uint32_t HLW_IREF_PULSE = 3500;  // was 1666us = 600Hz = 4.545A

class CSE7766
{
protected:
    SoftwareSerial *serial;

    uint8_t rawData[24];
    uint8_t rawDataIndex = 0;

    void cseReceived();
    unsigned long powerCalibration = HLW_PREF_PULSE;   // 364
    unsigned long voltageCalibration = HLW_UREF_PULSE; // 368
    unsigned long currentCalibration = HLW_IREF_PULSE; // 36C

public:
    CSE7766(uint8_t rxPin, uint16_t baudrate = 4800);
    CSE Cse;
    ENERGY Energy;

    void loop();
    bool everySecond();
};
#endif