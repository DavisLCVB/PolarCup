#include "polar-cup.hpp"

void TemperatureSensor::setup(Variables* variables) {
  this->variables = variables;
  i2c.begin(sdaPin, sclPin, 400000);
  if (!mlx.begin(0x5A, &i2c)) {
    Serial.println("Error al iniciar el sensor de temperatura");
  }
}

void WeightSensor::setup(Variables* variables) {
  this->variables = variables;
  scale.begin();
}

void FreezeSystem::setup(Variables* variables) {
  this->variables = variables;
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH);
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

void PolarCup::loop() {}
