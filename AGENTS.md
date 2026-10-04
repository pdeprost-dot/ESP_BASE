# Instructions pour assistants

ESP_BASE est un socle Arduino compact pour ESP8266, destiné ensuite à ESP32.
La priorité est : faire fonctionner sur matériel, valider, puis généraliser.

## Règles essentielles

- Conserver `ESP_BASE.ino` court ; placer l'infrastructure dans `src/`.
- Utiliser des services `begin()` / `tick()` et éviter les attentes bloquantes.
- Isoler toute API propre à une famille dans `PlatformCompat` ou le backend du
  service concerné.
- Préférer les buffers de taille fixe ; éviter les allocations répétées et les
  gros objets `String`, particulièrement sur ESP8266.
- Ne jamais journaliser de secret.
- Ne pas ajouter MQTT, Web, OTA ou une dépendance sans besoin explicite.
- Compiler d'abord `esp8266:esp8266:nodemcuv2` après chaque changement.
- Ne jamais intégrer de code ou de vocabulaire SMA dans ce dépôt.

## Organisation

Le sketch orchestre `DeviceIdentity`, `ConfigStore`, `LogService` et
`WiFiService`. `ConfigStore` masque son backend. `PlatformCompat` masque
l'identité matérielle, le hostname et les informations propres au core.

Mettre à jour la documentation lorsqu'un contrat public ou le processus de
compilation change. Ne stocker aucun identifiant Wi-Fi réel dans Git.

