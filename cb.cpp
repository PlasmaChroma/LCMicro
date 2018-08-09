#include "ht16k33.h"
#include "Arduino.h"

// externals
extern HT16K33 htd;
extern const uint8_t resetPin1;
extern const uint8_t resetPin2;

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
  char* brightStr = strtok(NULL, " ");
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

