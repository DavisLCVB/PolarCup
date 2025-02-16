#include <Arduino.h>
#include "polar-cup.hpp"

PolarCup polarCup;

void setup() {
  Serial.begin(115200);
  polarCup.setup();
}

void loop() {
  polarCup.loop();
}
