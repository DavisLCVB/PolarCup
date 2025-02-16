#ifndef POLAR_CUP_HPP
#define POLAR_CUP_HPP
#include <Adafruit_HX711.h>
#include <Adafruit_MLX90614.h>
#include <Arduino.h>
#include <EEPROM.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <Wire.h>
#include "SPIFFS.h"
#include "types.hpp"

struct Variables {
  f32 umbralTemp{0.0};
  f32 ambTemp{0.0};
  f32 refEfficiency{0.0};
  const c8* ssid{"Davis"};
  const c8* password{"12345678"};
};

class TemperatureSensor {
 public:
  TemperatureSensor() = default;
  void setup(Variables* variables);

 private:
  u8 sdaPin = 21;
  u8 sclPin = 22;
  Adafruit_MLX90614 mlx = Adafruit_MLX90614();
  TwoWire i2c = TwoWire(0);
  Variables* variables;
};

class WeightSensor {
 public:
  WeightSensor() = default;
  void setup(Variables* variables);

 private:
  u8 doutPin = 4;
  u8 sckPin = 5;
  Adafruit_HX711 scale = Adafruit_HX711(doutPin, sckPin);
  Variables* variables;
};

class FreezeSystem {
 public:
  FreezeSystem() = default;
  void setup(Variables* variables);

 private:
  u8 relayPin = 18;
  Variables* variables;
};

class WebServer {
 public:
  WebServer() = default;
  void setup(Variables* variables);

 private:
  void initSPIFFS();
  void initWiFi();
  AsyncWebServer server = AsyncWebServer(80);
  Variables* variables;
};

class PolarCup {
 public:
  PolarCup() = default;
  void setup();
  void loop();

 private:
  TemperatureSensor temperatureSensor;
  WeightSensor weightSensor;
  FreezeSystem freezeSystem;
  WebServer webServer;
  Variables variables;
};

#endif
