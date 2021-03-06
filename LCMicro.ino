//#include <Arduino.h>

#include "CommandLine.h"
#include "cb.h"
#include "font7seg.h"
#include "ht16k33.h"
#include "timer.h"
#include <stdint.h>

CommandLine commandLine(Serial, (char *)("> "));

ControlData_t ControlData;

// Class to control the ht16k33 display chip
HT16K33 htd;

// PIN configruation
const uint8_t tempPin = 0;
const uint8_t pwmPin = 6;
const uint8_t rpmPin = 7;
extern const uint8_t resetPin1 = 11;
extern const uint8_t resetPin2 = 12;

// Temperature reading setup
#define TEMP_READING_COUNT 10
uint8_t readingsIndex = 0;
double readings[TEMP_READING_COUNT] = {0};

const uint16_t loopDelay = 100;
const uint16_t cyclesPerSecond = 1000 / loopDelay;
const uint16_t cyclesPerFiveSecond = (5 * 1000) / loopDelay;
const uint32_t cyclesPerMinute = (60L * 1000L) / loopDelay;

const int fanDivisor = 2; // unipole hall effect sensor

// interrupt handler for fan rpm reading
int rpmCounter = 0;
void rpmCallback() { rpmCounter++; }

void setup() {
    htd.define7segFont(font7s);
    htd.begin(0x70);
    htd.setBrightness(2);

    pinMode(resetPin1, INPUT);
    pinMode(resetPin2, INPUT);

    // fan pwm control
    pinMode(pwmPin, OUTPUT);

    // fan rpm feedback
    pinMode(rpmPin, INPUT);
    attachInterrupt(digitalPinToInterrupt(rpmPin), rpmCallback, RISING);

    // debug port
    Serial.begin(115200);

    // commands
    commandLine.add((char *)"help", helpCallback);
    commandLine.add((char *)"reset", resetCallback);
    commandLine.add((char *)"bright", brightCallback);
    commandLine.add((char *)"status", statusCallback);
}

double readTempC(double tempPin) {
    uint16_t tempReading = analogRead(tempPin);
    double tempK = log(10000.0 * ((1024.0 / tempReading - 1.0)));
    tempK = 1 / (0.001129148 +
                 (0.000234125 + (0.0000000876741 * tempK * tempK)) *
                     tempK);                   //  Temp Kelvin
    double tempC = tempK - 273.15;             // Convert Kelvin to Celcius
    double tempF = (tempC * 9.0) / 5.0 + 32.0; // Convert Celcius to Fahrenheit
    return tempC;
}

void outputTempC(double tempC) {
    if (tempC < 100) {
        // display 10ths of a degree
        uint16_t tens = (uint16_t)tempC / 10;
        uint16_t ones = (uint16_t)tempC % 10;
        uint16_t tenths = (uint16_t)(tempC * 10 - (floor(tempC)) * 10);

        htd.set7Seg(0, tens, false);
        htd.set7Seg(1, ones, true);
        // address 2 is for the clock ':'
        htd.set7Seg(3, tenths, false);
        htd.set7Seg(4, FONT7S_C, false);
    }
}

double averagedTemp(double *list, uint16_t elementCount) {
    double sum = 0;
    for (uint16_t i = 0; i < elementCount; i++) {
        sum += list[i];
    }
    return sum / elementCount;
}

// max effort of "100%" translates to analog pwm of 255
void controlFans(uint8_t effortPercentage) {
    double fractionalEffort = effortPercentage / 100.0;
    uint8_t byteEffort = 255.0 * fractionalEffort;
    analogWrite(pwmPin, byteEffort);
}

void limitFanEffort(uint8_t &e)
{
    if (e < 1) e = 1;
    if (e > 100) e = 100;
}

uint32_t cycleCounter = 0;
uint32_t elapsedMicro;
void loop() {
    cycleCounter++;

    static timer tempReadTimer;
    elapsedMicro = tempReadTimer.getDeltaMicro();
    if (elapsedMicro > 100000) {
        double liquidTemp = readTempC(tempPin);
        readings[readingsIndex] = liquidTemp;
        readingsIndex++;
        tempReadTimer.set();
    }

    if (readingsIndex == TEMP_READING_COUNT) {
        readingsIndex = 0;
        ControlData.avgTemp = averagedTemp(readings, TEMP_READING_COUNT);

        // Serial.println(avgTemp, DEC);
        // output to i2c 4-digit display
        outputTempC(ControlData.avgTemp);
        htd.sendLed();
    }

    // sets the PWM control value
    controlFans(ControlData.fanEffort);

    static timer rpmTimer;
    elapsedMicro = rpmTimer.getDeltaMicro();
    // recalculate fan rpm every 10 seconds
    if (elapsedMicro > 10000000) {
        // get rpm value with interrupts disabled and zero counter
        noInterrupts();
        uint32_t rpmCounterRead = rpmCounter;
        rpmCounter = 0;
        rpmTimer.set();
        interrupts();

        double scaleFactor = (60000000 / elapsedMicro);

        ControlData.fanRPM = (rpmCounterRead * scaleFactor) / fanDivisor;

        // if we're in fixed RPM mode now adjust the fan effort
        if (ControlData.mode == MODE_FIXED_RPM)
        {
            if (ControlData.fanRPM > ControlData.fanRPMTarget)
            {
                Serial.println("lowering fan effort");
                ControlData.fanEffort--;
            }
            if (ControlData.fanRPM < ControlData.fanRPMTarget)
            {
                Serial.println("raising fan effort");
                ControlData.fanEffort++;
            }

            limitFanEffort(ControlData.fanEffort);
        }

        // Serial.print("Elapsed = ");
        // Serial.print(elapsedMicro / 1000000.0, DEC);
        // Serial.print("  Fan RPM = ");
        // Serial.println(fanRPM, DEC);
    }

    commandLine.update();

    delay(1);
}
