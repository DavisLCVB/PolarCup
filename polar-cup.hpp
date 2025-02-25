#ifndef POLAR_CUP_HPP
#define POLAR_CUP_HPP
#include <Adafruit_MLX90614.h>
#include <Arduino.h>
#include <EEPROM.h>
#include <Firebase_ESP_Client.h>
#include <HX711.h>
#include <WiFi.h>
#include <Wire.h>
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
  const c8* databaseUrl{"https://cooler-5a7a4-default-rtdb.firebaseio.com"};
  const c8* apiKey{"AIzaSyD0BxAdYvrRWcY0_laAxBfWA2nrC1i8gN8"};
  const c8* deviceId{"wawita20"};
  const c8* basePath{"devices/"};
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

class FirebaseDatabase {
 public:
  FirebaseDatabase() = default;
  void setup(Variables* variables);
  u64 sendDataPrevMillis = 0;
  FirebaseData fbdo;
  c8* temperaturePath;
  c8* coolingPath;
  c8* volumePath;
  bool singupOk = false;

 private:
  FirebaseAuth auth;
  FirebaseConfig config;
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
  FirebaseDatabase firebaseDatabase;
  Variables variables;
};

#endif
