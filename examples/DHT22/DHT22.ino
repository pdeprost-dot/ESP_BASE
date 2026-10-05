#include <ESPBase.h>
#include <DHT.h>

#if defined(ESP8266)
constexpr uint8_t DHT_PIN = D4;
#else
constexpr uint8_t DHT_PIN = 2;
#endif
constexpr uint8_t DHT_TYPE = DHT22;
constexpr uint32_t READ_INTERVAL_MS = 2500;
constexpr uint32_t MQTT_PUBLISH_INTERVAL_MS = 10000;

ESPBase espBase("DHT22 Example", "1.0.0");
DHT dht(DHT_PIN, DHT_TYPE);
uint32_t nextReadAt = 0;
uint32_t nextMqttPublishAt = 0;
uint32_t mqttCommandsReceived = 0;
float lastTemperature = NAN;
float lastHumidity = NAN;

void handleMqttCommand(const char* topic, const uint8_t*, size_t length, void*) {
  ++mqttCommandsReceived;
  Serial.print(F("MQTT command received topic="));
  Serial.print(topic);
  Serial.print(F(" length="));
  Serial.println(length);
}

void handleDhtApi(WebResponse& response, void*) {
  if (isnan(lastTemperature) || isnan(lastHumidity)) {
    response.sendJson("{\"valid\":false,\"temperature_c\":null,\"humidity_percent\":null}");
    return;
  }
  char json[144];
  snprintf(json, sizeof(json),
           "{\"valid\":true,\"temperature_c\":%.1f,\"humidity_percent\":%.1f,"
           "\"mqtt_commands\":%lu}",
           lastTemperature, lastHumidity,
           static_cast<unsigned long>(mqttCommandsReceived));
  response.sendJson(json);
}

void handleDhtPage(WebResponse& response, void*) {
  response.beginPage("DHT22");
  if (isnan(lastTemperature) || isnan(lastHumidity)) {
    response.write("<section class='card'><p class='muted'>Aucune mesure valide disponible.</p>"
                   "</section>");
  } else {
    char temperature[16];
    char humidity[16];
    dtostrf(lastTemperature, 0, 1, temperature);
    dtostrf(lastHumidity, 0, 1, humidity);
    char content[320];
    snprintf(content, sizeof(content),
             "<div class='grid'><section class='card'><div class='muted'>Temperature</div>"
             "<div class='hero'>%s &deg;C</div></section><section class='card'>"
             "<div class='muted'>Humidite</div><div class='hero'>%s %%</div></section>"
             "</div><p class='muted'>Derniere acquisition valide.</p>",
             temperature, humidity);
    response.write(content);
  }
  response.endPage();
}

void setup() {
  espBase.addPage("DHT22", "/dht22", handleDhtPage);
  espBase.addGetRoute("/api/dht22", handleDhtApi);
  espBase.begin();
  espBase.mqtt().subscribe("commands/test", handleMqttCommand);
  dht.begin();
}

void loop() {
  espBase.loop();

  const uint32_t now = millis();
  if (static_cast<int32_t>(now - nextReadAt) < 0) return;
  nextReadAt = now + READ_INTERVAL_MS;

  const float humidity = dht.readHumidity();
  const float temperature = dht.readTemperature();
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println(F("DHT22 read failed"));
    return;
  }

  lastHumidity = humidity;
  lastTemperature = temperature;
  if (static_cast<int32_t>(now - nextMqttPublishAt) < 0) return;
  nextMqttPublishAt = now + MQTT_PUBLISH_INTERVAL_MS;
  if (!espBase.mqtt().connected()) return;

  char value[16];
  dtostrf(lastTemperature, 0, 1, value);
  espBase.mqtt().publish("temperature", value, true);
  dtostrf(lastHumidity, 0, 1, value);
  espBase.mqtt().publish("humidity", value, true);
}
