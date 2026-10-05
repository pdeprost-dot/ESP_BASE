# NTPSystemMonitor

Exemple ESP8266 sans matériel externe. Il ajoute une page NTP à la navbar,
configure le serveur et le fuseau POSIX avec une route POST ESP_BASE, applique
les règles DST avec `configTime()`, et conserve la configuration dans LittleFS.

Il publie périodiquement sur le topic MQTT relatif `ntp/status` un état retained
contenant l'identité, l'heure locale, l'uptime, le heap, le RSSI et l'adresse IP.

## Dépendances et utilisation

- ESP8266 Arduino core 3.1.2 ;
- PubSubClient 2.8, dépendance d'ESP_BASE ;
- aucune bibliothèque applicative ni aucun capteur supplémentaire.

Configurer d'abord le Wi-Fi et éventuellement MQTT dans ESP_BASE, puis ouvrir
`/ntp`. Le fuseau est une chaîne POSIX, par exemple
`CET-1CEST,M3.5.0,M10.5.0/3`.

Cet exemple est volontairement ESP8266 : il utilise `LittleFS`, `ESP` et les
diagnostics Wi-Fi du core ESP8266.
