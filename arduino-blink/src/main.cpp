#include <Arduino.h>

void setup()
{
  // initialize LED digital pin as an output.
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  Serial.begin(115200);
}

void loop()
{
  // turn the LED on (HIGH is the voltage level)
  digitalWrite(4, HIGH);
  digitalWrite(5, LOW);

  // wait for a second
  delay(500);
  // turn the LED off by making the voltage LOW
  digitalWrite(4, LOW);
  digitalWrite(5, HIGH);

   // wait for a second
  delay(500);
  Serial.println("Hello world ...");
}
