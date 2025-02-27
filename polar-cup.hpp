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

#include <algorithm>
#include <cmath>
#include <numeric>

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
  const c8* databaseUrl{"[]"};
  const c8* apiKey{"[]"};
  const c8* deviceId{"[]"};
  const c8* basePath{"[]"};
};

class MedianFilter {
 public:
  MedianFilter(int size, f32 threshold);
  f32 filter(f32 measurement);

 private:
  int bufferSize;
  vec<f32> buffer;
  int bufferIndex;
  f32 changeThreshold;
  bool isBufferInitialized{false};
  f32 computeStandardDeviation();
  f32 calculateMedian();
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
  MedianFilter filter = MedianFilter(5, 100.0);
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
