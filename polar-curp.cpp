#include "polar-cup.hpp"

void TemperatureSensor::setup(Variables* variables) {
  this->variables = variables;
  i2c.begin(sdaPin, sclPin, 400000);
  if (!mlx.begin(0x5A, &i2c)) {
    Serial.println("Error al iniciar el sensor de temperatura");
  }
}

f32 TemperatureSensor::getTemperature() {
  return mlx.readObjectTempC();
}

void WeightSensor::setup(Variables* variables) {
  this->variables = variables;
  scale.begin();
  scale.tareA(0);
}

f32 WeightSensor::getWeight() {
  return scale.readChannelBlocking(CHAN_A_GAIN_128);
}

f32 WeightSensor::getVolume() {
  return getWeight() * 0.9982;
}

void FreezeSystem::setup(Variables* variables) {
  this->variables = variables;
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);
}

void FreezeSystem::switchFreeze(bool state) {
  digitalWrite(relayPin, state);
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
                      ", \"needsCooling\": " + String(this->variables->needsCooling) +
                      "}";
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

void PolarCup::setup() {
  webServer.setup(&variables);
}

void PolarCup::loop() {
  variables.liqTemp = temperatureSensor.getTemperature();
  variables.volume = weightSensor.getVolume();
  variables.time = (variables.liqTemp - variables.optTemp) / variables.efficiency;
  variables.needsCooling = variables.liqTemp > variables.optTemp;
  if (variables.needsCooling) {
    freezeSystem.switchFreeze(true);
  } else {
    freezeSystem.switchFreeze(false);
  }
}
