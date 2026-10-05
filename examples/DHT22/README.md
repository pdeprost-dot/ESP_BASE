# DHT22

Exemple de capteur DHT22 utilisant ESP_BASE pour le Wi-Fi, le Web, l'OTA et
MQTT. Il expose une page `/dht22`, une API `/api/dht22`, publie les mesures
retained sur `temperature` et `humidity`, et écoute `commands/test`.

## Matériel et dépendances

- Wemos D1 mini ESP8266 ;
- DHT22, broche DATA sur `D4` / GPIO2 ;
- DHT sensor library 1.4.7 par Adafruit ;
- Adafruit Unified Sensor 1.1.15.

Sur ESP32, l'exemple utilise GPIO2. Vérifier que cette broche convient à la
carte choisie avant câblage.

Configurer le Wi-Fi puis, si nécessaire, MQTT depuis les pages ESP_BASE.
