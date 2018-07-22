#include "font7seg.h"
#include "ht16k33.h"
#include "timer.h"
#include "CommandLine.h"
#include <stdint.h>

CommandLine commandLine(Serial, (char*)("> "));

// Class to control the ht16k33 display chip
HT16K33 htd;

// PIN configruation
const uint8_t tempPin = 0;
const uint8_t pwmPin = 6;
const uint8_t rpmPin = 7;
const uint8_t resetPin1 = 11;
const uint8_t resetPin2 = 12;

// Temperature reading setup
#define TEMP_READING_COUNT 10
uint8_t readingsIndex = 0;
double readings[TEMP_READING_COUNT] = { 0 };

const uint16_t loopDelay = 100;
const uint16_t cyclesPerSecond = 1000 / loopDelay;
const uint16_t cyclesPerFiveSecond = (5 * 1000) / loopDelay;
const uint32_t cyclesPerMinute = (60L * 1000L) / loopDelay;

const int fanDivisor = 2; // unipole hall effect sensor

// interrupt handler for fan rpm reading
int rpmCounter = 0;
void rpmCallback()
{
  rpmCounter++;
}

// **********************************
// **    Command line callbacks    **
// **********************************
void helpCallback(char* tokens)
{
  Serial.println("bright <x> : change brightness (0:off - 15:max)");
  Serial.println("status     : print current temp & pwm info");
  Serial.println("reset      : toggle motherboard reset");
  
}

void resetCallback(char* tokens)
{
  int val1 = digitalRead(resetPin1);
  int val2 = digitalRead(resetPin2);
  Serial.print("Pin Detect V1:");
  Serial.print(val1, DEC);
  Serial.print(" V2:");
  Serial.println(val2, DEC);

  Serial.println("asserting reset in 0.2 second");
  delay(200);

  // depending on how this is wired, pull down the active pin
  if (val1 == HIGH && val2 == LOW)
  {
    pinMode(resetPin1, OUTPUT);
    digitalWrite(resetPin1, LOW);
  }
  if (val1 == LOW && val2 == HIGH)
  {
    pinMode(resetPin2, OUTPUT);
    digitalWrite(resetPin2, LOW);   
  }
  if (val1 == LOW && val2 == LOW)
  {
    Serial.println("could not detect active pin");
  }

  delay(1); // 1 millisecond

  // stop output, set pins back to input
  pinMode(resetPin1, INPUT);
  pinMode(resetPin2, INPUT);
}

void brightCallback(char* tokens)
{
  char buffer[32];
  char* brightStr = strtok(NULL, " ");
  sprintf(buffer, "brightStr: %s", brightStr);
  Serial.println(buffer);
  uint8_t bright = atoi(brightStr);
  if (bright > 0 && bright <= 15) {
    Serial.print("Brightness is now: ");
    Serial.println(bright, DEC);
    htd.displayOn();
    htd.setBrightness(bright - 1);
  }
  if (bright == 0) {
    htd.displayOff();
  }
  if (bright > 15) {
    Serial.println("Please enter number from 0 to 15");
  }
}

void setup()
{
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
  commandLine.add((char*)"help", helpCallback);
  commandLine.add((char*)"reset", resetCallback);
  commandLine.add((char*)"bright", brightCallback);

}

double readTempC(double tempPin)
{
  uint16_t tempReading = analogRead(tempPin);
  double tempK = log(10000.0 * ((1024.0 / tempReading - 1.0)));
  tempK = 1 / (0.001129148 + (0.000234125 + (0.0000000876741 * tempK * tempK )) * tempK );       //  Temp Kelvin
  double tempC = tempK - 273.15;            // Convert Kelvin to Celcius
  double tempF = (tempC * 9.0)/ 5.0 + 32.0; // Convert Celcius to Fahrenheit
  return tempC;
}

void outputTempC(double tempC)
{
  if (tempC < 100)
  {
    // display 10ths of a degree
    uint16_t tens   = (uint16_t)tempC / 10;
    uint16_t ones   = (uint16_t)tempC % 10;
    uint16_t tenths = (uint16_t)(tempC * 10 - (floor(tempC)) * 10);

    htd.set7Seg(0, tens, false);
    htd.set7Seg(1, ones, true);
    // address 2 is for the clock ':'
    htd.set7Seg(3, tenths, false);
    htd.set7Seg(4, FONT7S_C, false);
  }
}

double averagedTemp(double* list, uint16_t elementCount)
{
  double sum = 0;
  for (uint16_t i = 0; i < elementCount; i++)
  {
    sum += list[i];
  }
  return sum / elementCount;
}

uint8_t pwmValue = 64;
void controlFans(int override)
{
  if (pwmValue == 255)
  { 
    pwmValue = 32;
  }

  if (override == 256)
    analogWrite(pwmPin, pwmValue++);
  else
    analogWrite(pwmPin, override);
}

uint32_t cycleCounter = 0;
uint32_t elapsedMicro;
void loop()
{
  cycleCounter++;

  static timer tempReadTimer;
  elapsedMicro = tempReadTimer.getDeltaMicro();
  if (elapsedMicro > 100000)
  {
    double liquidTemp = readTempC(tempPin);
    readings[readingsIndex] = liquidTemp;
    readingsIndex++;
    tempReadTimer.set();
  }

  if (readingsIndex == TEMP_READING_COUNT)
  {
    readingsIndex = 0;
    double avgTemp = averagedTemp(readings, TEMP_READING_COUNT);

    //Serial.println(avgTemp, DEC);
    // output to i2c 4-digit display
    outputTempC(avgTemp);
    htd.sendLed();
  }

  // sets the PWM control value
  controlFans(1);

  static timer rpmTimer;
  elapsedMicro = rpmTimer.getDeltaMicro();
  if (elapsedMicro > 10000000)
  {
    // get rpm value with interrupts disabled and zero counter
    noInterrupts();
    uint32_t rpmCounterRead = rpmCounter;
    rpmCounter = 0;
    rpmTimer.set();
    interrupts();

    double scaleFactor = (60000000 / elapsedMicro);
    
    uint32_t fanRPM = (rpmCounterRead * scaleFactor) / fanDivisor;
//    Serial.print("Elapsed = ");
//    Serial.print(elapsedMicro / 1000000.0, DEC);
//    Serial.print("  Fan RPM = ");
//    Serial.println(fanRPM, DEC);
    
  }

  commandLine.update();

  delay(1);
}
