#ifndef POLAR_CUP_HPP
#define POLAR_CUP_HPP
#include <HX711.h>
#include <Adafruit_MLX90614.h>
#include <Arduino.h>
#include <EEPROM.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <Wire.h>
#include "SPIFFS.h"
#include "types.hpp"

struct Variables {
  f32 optTemp{25.0};
  f32 efficiency{0.5};
  f32 liqTemp{0.0};
  f32 weight{0.0};
  f32 volume{0.0};
  f32 time{0.0};
  f32 calWeigth{213.0};
  bool needsCooling{false};
  const c8* ssid{"Davis"};
  const c8* password{"12345678"};
};

class TemperatureSensor {
 public:
  TemperatureSensor() = default;
  void setup(Variables* variables);
  f32 getTemperature();

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
  void calibrate();
  f32 getWeight();
  f32 getVolume();

 private:
  u8 doutPin = 4;
  u8 sckPin = 5;
  HX711 scale;
  Variables* variables;
};

class FreezeSystem {
 public:
  FreezeSystem() = default;
  void setup(Variables* variables);
  void switchFreeze(bool state);

 private:
  u8 platePin = 18;
  u8 disPin = 19;
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
