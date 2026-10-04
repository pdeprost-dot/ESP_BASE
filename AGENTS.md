# Instructions pour assistants

ESP_BASE est une bibliothèque Arduino compacte pour ESP8266, destinée ensuite
à ESP32. Les projets la consomment ; ils ne copient pas son infrastructure.

## Règles essentielles

- Conserver `examples/Minimal/Minimal.ino` comme modèle officiel minimal.
- L'orchestration appartient à `ESPBase`, jamais au sketch utilisateur.
- Utiliser des services `begin()` / `tick()` et éviter les attentes bloquantes.
- Isoler toute API propre à une famille dans `PlatformCompat` ou le backend du
  service concerné.
- Préférer les buffers de taille fixe ; éviter les allocations répétées et les
  gros objets `String`, particulièrement sur ESP8266.
- Ne jamais journaliser de secret.
- Web/API font partie du socle standard. Ne pas ajouter MQTT, OTA ou une
  dépendance sans besoin explicite.
- Compiler d'abord `examples/Minimal` avec `--library .` pour
  `esp8266:esp8266:nodemcuv2` après chaque changement.
- Ne jamais intégrer de code ou de vocabulaire SMA dans ce dépôt.

## Organisation

`ESPBase` orchestre `DeviceIdentity`, `ConfigStore`, `LogService`,
`WiFiService` et `WebService`. Le sketch inclut seulement `<ESPBase.h>`.
`ConfigStore` masque son backend et `PlatformCompat` les API du core.
`WebService` fournit `/`, `/api/status`, `/api/logs` et le POST de provisioning
réservé à l'AP. Ne jamais renvoyer ou journaliser un mot de passe.

Mettre à jour la documentation lorsqu'un contrat public ou le processus de
compilation change. Ne stocker aucun identifiant Wi-Fi réel dans Git.

