#include "polar-cup.hpp"

MedianFilter::MedianFilter(int size, f32 threshold)
    : bufferSize(size), buffer(size, 0), bufferIndex(0), changeThreshold(threshold) {}

f32 MedianFilter::computeStandardDeviation()
{
  f32 mean = std::accumulate(buffer.begin(), buffer.end(), 0.0) / bufferSize;
  f32 variance = 0.0;
  for (f32 val : buffer)
  {
    variance += std::pow(val - mean, 2);
  }
  variance /= bufferSize;
  return std::sqrt(variance);
}

f32 MedianFilter::filter(f32 measurement)
{
  f32 dynamicThreshold = computeStandardDeviation() * changeThreshold;

  if (!isBufferInitialized)
  {
    Serial.println("Inicializando buffer...");
    std::fill(buffer.begin(), buffer.end(), measurement);
    isBufferInitialized = true;
  }
  else
  {
    if (bufferIndex > 0 &&
        std::abs(measurement - buffer[(bufferIndex - 1 + bufferSize) % bufferSize]) >
            dynamicThreshold)
    {

      measurement = buffer[(bufferIndex - 1 + bufferSize) % bufferSize];
    }

    buffer[bufferIndex] = measurement;
    bufferIndex = (bufferIndex + 1) % bufferSize;
  }

  return calculateMedian();
}

float MedianFilter::calculateMedian()
{
  f32 median = 0;
  vec<f32> tempBuffer(buffer);
  if (bufferSize % 2 == 1)
  {
    std::nth_element(tempBuffer.begin(), tempBuffer.begin() + bufferSize / 2, tempBuffer.end());
    median = tempBuffer[bufferSize / 2];
  }
  else
  {
    std::sort(tempBuffer.begin(), tempBuffer.end());
    int mid = bufferSize / 2;
    median = (tempBuffer[mid - 1] + tempBuffer[mid]) / 2.0f;
  }

  return median;
}

void TemperatureSensor::setup(Variables *variables)
{
  this->variables = variables;
  if (!mlx.begin())
  {
    Serial.println("Error al iniciar el sensor de temperatura");
  }
}

f32 TemperatureSensor::getTemperature()
{
  return mlx.readObjectTempC();
}

void WeightSensor::setup(Variables *variables)
{
  this->variables = variables;
  scale.begin(this->doutPin, this->sckPin);
  f32 scaleVal = -0.92;
  EEPROM.get(0, scaleVal);
  Serial.print("Scale value: ");
  Serial.println(scaleVal);
  Serial.println("Press + or - to calibrate");
  scale.set_scale(scaleVal);
  delay(2000);
  if (Serial.available())
  {
    char temp = Serial.read();
    if (temp == '+' || temp == '-')
    {
      calibrate();
    }
  }
}

f32 WeightSensor::getWeight()
{

  f32 weight = scale.get_units(10);

  return filter.filter(weight);
}

f32 WeightSensor::getVolume()
{
  return getWeight() * 0.9982;
}

void WeightSensor::calibrate()
{
  bool conf = true;
  i64 adcLec;
  scale.read();
  scale.set_scale();
  scale.tare(20);
  Serial.println("Put the scale weigth");
  delay(1000);
  while (conf)
  {
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

void FreezeSystem::setup(Variables *variables)
{
  this->variables = variables;
  pinMode(platePin, OUTPUT);
  pinMode(disPin, OUTPUT);
  digitalWrite(platePin, HIGH);
  digitalWrite(disPin, LOW);
}

void FreezeSystem::switchFreeze(bool state)
{
  digitalWrite(platePin, state ? LOW : HIGH);
  digitalWrite(disPin, state ? HIGH : LOW);
}

void MQTTServer::setup(Variables *variables)
{
  this->variables = variables;

  WiFi.begin(this->variables->ssid, this->variables->password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  client.setServer(this->variables->mqtt_server, this->variables->mqtt_port);
}

void MQTTServer::reconect()
{
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(this->variables->deviceId))
    {
      Serial.println("connected");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
    }
  }
}

void MQTTServer::sendData()
{
  if (millis() - sendDataPrevMillis > 5000)
  {
    std::string payload = "{" 
                          "\"deviceId\": \"" + std::string(this->variables->deviceId) + "\", "
                          "\"volume\": " + std::to_string(this->variables->volume) + ", "
                          "\"temperature\": " + std::to_string(this->variables->liqTemp) + ", "
                          "\"cooling\": " + std::to_string(this->variables->needsCooling) +
                          "}";
    client.publish(this->variables->topic, payload.c_str());
    Serial.println("📡 Datos enviados al servidor MQTT:");
    Serial.println(payload.c_str());
  }
}

void PolarCup::setup()
{
  temperatureSensor.setup(&this->variables);
  weightSensor.setup(&this->variables);
  freezeSystem.setup(&this->variables);
  mqttServer.setup(&this->variables);
  Serial.println("End Setup");
}

void PolarCup::loop()
{

  if (!mqttServer.client.connected())
  {
    mqttServer.reconect();
  }

  mqttServer.client.loop();

  variables.liqTemp = temperatureSensor.getTemperature();
  variables.volume = weightSensor.getVolume() < 0
                         ? 10
                         : (weightSensor.getVolume() > 1000 ? 1000 : weightSensor.getVolume());
  variables.time = (variables.liqTemp - variables.optTemp) / variables.efficiency;
  variables.needsCooling = variables.liqTemp > variables.optTemp;
  if (variables.needsCooling)
  {
    freezeSystem.switchFreeze(true);
  }
  else
  {
    freezeSystem.switchFreeze(false);
  }

  mqttServer.sendData();
  delay(5000);
}
