#include "font7seg.h"
#include "ht16k33.h"

#define TEMP_READING_COUNT 10

HT16K33 htd;
const int tempPin = 0;
int readingsIndex = 0;
double readings[TEMP_READING_COUNT] = { 0 };

double readTempC(double tempPin)
{
  int tempReading = analogRead(tempPin);
  double tempK = log(10000.0 * ((1024.0 / tempReading - 1)));
  tempK = 1 / (0.001129148 + (0.000234125 + (0.0000000876741 * tempK * tempK )) * tempK );       //  Temp Kelvin
  double tempC = tempK - 273.15;            // Convert Kelvin to Celcius
  double tempF = (tempC * 9.0)/ 5.0 + 32.0; // Convert Celcius to Fahrenheit

  Serial.print("Raw:");
  Serial.print(tempReading, DEC);
  Serial.print("  C:");
  Serial.print(tempC, DEC);
  Serial.print("  F:");
  Serial.print(tempF, DEC);
  Serial.println("");

  return tempC;
}

void outputTempC(double tempC)
{
  if (tempC < 100)
  {
    // display 10ths of a degree
    int tens = (int)tempC / 10;
    int ones = (int)tempC % 10;
    int tenths = int(tempC * 10 - (floor(tempC)) * 10);

    htd.set7Seg(0, tens, false);
    htd.set7Seg(1, ones, true);
    // address 2 is for the clock ':'
    htd.set7Seg(3, tenths, false);
    htd.set7Seg(4, FONT7S_C, false);
  }
}

void setup()
{
  htd.define7segFont(font7s);
  htd.begin(0x70);
  htd.setBrightness(1);

  // debug port
  Serial.begin(9600);
}

double averagedTemp(double* list, int elementCount)
{
  double sum = 0;
  for (int i = 0; i < elementCount; i++)
  {
    sum += list[i];
  }
  return sum / elementCount;
}

void loop()
{
  double liquidTemp = readTempC(tempPin);
  readings[readingsIndex] = liquidTemp;
  readingsIndex++;

  if (readingsIndex == TEMP_READING_COUNT)
  {
    readingsIndex = 0;
    double avgTemp = averagedTemp(readings, TEMP_READING_COUNT);

    Serial.println(avgTemp, DEC);
    // output to i2c 4-digit display
    outputTempC(avgTemp);
    htd.sendLed();
  }

  delay(100);
}
