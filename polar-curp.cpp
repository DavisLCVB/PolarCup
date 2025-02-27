#include "polar-cup.hpp"

MedianFilter::MedianFilter(int size, f32 threshold)
    : bufferSize(size), buffer(size, 0), bufferIndex(0), changeThreshold(threshold) {}

f32 MedianFilter::computeStandardDeviation() {
  f32 mean = std::accumulate(buffer.begin(), buffer.end(), 0.0) / bufferSize;
  f32 variance = 0.0;
  for (f32 val : buffer) {
    variance += std::pow(val - mean, 2);
  }
  variance /= bufferSize;
  return std::sqrt(variance);
}

f32 MedianFilter::filter(f32 measurement) {
  f32 dynamicThreshold = computeStandardDeviation() * changeThreshold;

  if (!isBufferInitialized) {
    Serial.println("Inicializando buffer...");
    std::fill(buffer.begin(), buffer.end(), measurement);
    isBufferInitialized = true;

  } else {
    if (bufferIndex > 0 &&
        std::abs(measurement - buffer[(bufferIndex - 1 + bufferSize) % bufferSize]) >
            dynamicThreshold) {

      measurement = buffer[(bufferIndex - 1 + bufferSize) % bufferSize];
    }

    buffer[bufferIndex] = measurement;
    bufferIndex = (bufferIndex + 1) % bufferSize;
  }

  return calculateMedian();
}

float MedianFilter::calculateMedian() {
  f32 median = 0;
  vec<f32> tempBuffer(buffer);
  if (bufferSize % 2 == 1) {
    std::nth_element(tempBuffer.begin(), tempBuffer.begin() + bufferSize / 2, tempBuffer.end());
    median = tempBuffer[bufferSize / 2];
  } else {
    std::sort(tempBuffer.begin(), tempBuffer.end());
    int mid = bufferSize / 2;
    median = (tempBuffer[mid - 1] + tempBuffer[mid]) / 2.0f;
  }

  return median;
}

void TemperatureSensor::setup(Variables* variables) {
  this->variables = variables;
  if (!mlx.begin()) {
    Serial.println("Error al iniciar el sensor de temperatura");
  }
}

f32 TemperatureSensor::getTemperature() {
  return mlx.readObjectTempC();
}

void WeightSensor::setup(Variables* variables) {
  this->variables = variables;
  scale.begin(this->doutPin, this->sckPin);
  f32 scaleVal = -0.92;
  EEPROM.get(0, scaleVal);
  Serial.print("Scale value: ");
  Serial.println(scaleVal);
  Serial.println("Press + or - to calibrate");
  scale.set_scale(scaleVal);
  delay(2000);
  if (Serial.available()) {
    char temp = Serial.read();
    if (temp == '+' || temp == '-') {
      calibrate();
    }
  }
}

f32 WeightSensor::getWeight() {

  f32 weight = scale.get_units(10);

  return filter.filter(weight);
}

f32 WeightSensor::getVolume() {
  return getWeight() * 0.9982;
}

void WeightSensor::calibrate() {
  bool conf = true;
  i64 adcLec;
  scale.read();
  scale.set_scale();
  scale.tare(20);
  Serial.println("Put the scale weigth");
  delay(1000);
  while (conf) {
    adcLec = scale.get_value(100);
    Serial.print("ADC lec: ");
    Serial.println(adcLec);
    f32 scaleVal = adcLec / variables->calWeigth;
    EEPROM.put(0, scaleVal);
    Serial.print("Calibration value: ");
    Serial.println(scaleVal);
    scale.set_scale(scaleVal);
    delay(3000);
    conf = false;
  }
}

void FreezeSystem::setup(Variables* variables) {
  this->variables = variables;
  pinMode(platePin, OUTPUT);
  pinMode(disPin, OUTPUT);
  digitalWrite(platePin, HIGH);
  digitalWrite(disPin, LOW);
}

void FreezeSystem::switchFreeze(bool state) {
  digitalWrite(platePin, state ? LOW : HIGH);
  digitalWrite(disPin, state ? HIGH : LOW);
}

void WebServer::setup(Variables* variables) {
  this->variables = variables;
  initSPIFFS();
  initWiFi();
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    Serial.println("Enviando index.html");
    request->send(SPIFFS, "/index.html", "text/html");
  });
  server.on("/styles.css", HTTP_GET, [](AsyncWebServerRequest* request) {
    Serial.println("Enviando styles.css");
    request->send(SPIFFS, "/styles.css", "text/css");
  });
  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest* request) {
    Serial.println("Enviando script.js");
    request->send(SPIFFS, "/script.js", "text/javascript");
  });
  server.on("/data", HTTP_GET, [this](AsyncWebServerRequest* request) {
    Serial.println("Enviando data");
    String data = "{\"temperature\": " + String(this->variables->liqTemp) +
                  ", \"volume\": " + String(this->variables->volume) +
                  ", \"time\": " + String(this->variables->time) +
                  ", \"needsCooling\": " + String(this->variables->needsCooling) + "}";
    Serial.println(data);
    request->send(200, "application/json", data);
  });
  server.begin();
}

void WebServer::initSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("¡Error al montar SPIFFS!");
    return;
  }
  Serial.println("SPIFFS montado correctamente.");
}

void WebServer::initWiFi() {
  WiFi.begin(variables->ssid, variables->password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando a WiFi...");
  }
  Serial.println("Conectado a WiFi");
}

void FirebaseDatabase::setup(Variables* variables) {
  this->variables = variables;
  config.api_key = this->variables->apiKey;
  config.database_url = this->variables->databaseURL;

  if (Firebase.singUp(&auth, &config)) {
    this->singupOk = true;
    Serial.println("Sing up ok");
  } else {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  config.token_status_callback = tokenStatusCallback;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void PolarCup::setup() {
  temperatureSensor.setup(&this->variables);
  weightSensor.setup(&this->variables);
  freezeSystem.setup(&this->variables);
  webServer.setup(&variables);
  firebaseDatabase.setup(&variables);
  Serial.println("End Setup");
}

void PolarCup::loop() {
  variables.liqTemp = temperatureSensor.getTemperature();
  variables.volume = weightSensor.getVolume() < 0
                         ? 10
                         : (weightSensor.getVolume() > 1000 ? 1000 : weightSensor.getVolume());
  variables.time = (variables.liqTemp - variables.optTemp) / variables.efficiency;
  variables.needsCooling = variables.liqTemp > variables.optTemp;
  if (variables.needsCooling) {
    freezeSystem.switchFreeze(true);
  } else {
    freezeSystem.switchFreeze(false);
  }

  if (Firebase.ready() && firebaseDatabase.singupOk &&
      millis() - firebaseDatabase.sendDataPrevMillis > 3000) {
    firebaseDatabase.sendDataPrevMillis = millis();
    if (Firebase.RDTB.setFloat(&firebaseDatabase.fbdo, firebaseDatabase.temperaturePath,
                               variables.liqTemp)) {
      Serial.println("Temperature Updated");
    } else {
      Serial.println("Error updating temperature");
    }
    if (Firebase.RDTB.setFloat(&firebaseDatabase.fbdo, firebaseDatabase.volumePath,
                               variables.volume)) {
      Serial.println("Volume Updated");
    } else {
      Serial.println("Error updating volume");
    }
    if (Firebase.RDTB.setBool(&firebaseDatabase.fbdo, firebaseDatabase.coolingPath,
                              variables.needsCooling)) {
      Serial.println("Cooling Updated");
    } else {
      Serial.println("Error updating cooling");
    }
  }
}
