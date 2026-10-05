# Instructions pour assistants

ESP_BASE est une bibliothèque Arduino compacte pour ESP8266 et ESP32. Les
projets la consomment ; ils ne copient pas son infrastructure. Arduino IDE et
Arduino CLI sont les outils de référence : ne pas imposer PlatformIO.

## Règles essentielles

- Conserver `examples/Minimal/Minimal.ino` comme modèle officiel minimal.
- L'orchestration appartient à `ESPBase`, jamais au sketch utilisateur.
- L'API publique entre par `<ESPBase.h>`. Une application ne réimplémente pas
  l'identité, la configuration, le Wi-Fi, le fallback AP, les logs, le Web/API,
  l'OTA ou le transport MQTT déjà fournis.
- Utiliser des services `begin()` / `tick()` et éviter les attentes bloquantes.
- Isoler toute API propre à une famille dans `PlatformCompat` ou le backend du
  service concerné.
- Préférer les buffers de taille fixe ; éviter les allocations répétées et les
  gros objets `String`, particulièrement sur ESP8266.
- Ne jamais stocker ni journaliser de secret, SSID, mot de passe ou IP locale.
- Préserver la compatibilité ESP8266/ESP32 lorsque cela reste raisonnable, sans
  modifier la bibliothèque pour la seule commodité d'une application.
- Compiler d'abord `examples/Minimal` avec `--library .` pour
  `esp8266:esp8266:nodemcuv2` après chaque changement, puis les plateformes et
  exemples concernés après toute évolution d'API.
- Ne jamais déplacer ni réécrire un tag de release existant.
- Ne jamais intégrer de code ou de vocabulaire SMA dans ce dépôt.

## Organisation

`ESPBase` orchestre `DeviceIdentity`, `ConfigStore`, `LogService`,
`WiFiService`, `WebService`, `OtaService` et `MqttService`.
`ConfigStore` masque son backend et `PlatformCompat` les API du core.
`WebService` fournit le diagnostic, l'administration Wi-Fi, MQTT et Web OTA.
`MqttService` gère l'infrastructure ; les topics et payloads métier restent
dans le projet consommateur. Ne pas créer une seconde instance PubSubClient
applicative sans nécessité démontrée. Le mot de passe AP public par défaut est
`ESPbaseSetup` et sa personnalisation est persistante. ESP_BASE 2.4.1 n'emploie
aucune authentification applicative ; OTA reste limitée au LAN de confiance.

Avant `begin()`, les consommateurs peuvent ajouter jusqu'à quatre routes/pages
partagées avec `addGetRoute()`, `addPostRoute()` et `addPage()`. `WebRequest`
expose les arguments de formulaire pendant le callback et `WebResponse` produit
les réponses ou le shell commun. Le serveur HTTP natif reste interne et ne doit
pas être contourné lorsque cette API suffit. La configuration interne ESP_BASE
ne doit pas être contournée non plus. Une application peut cependant gérer son
propre stockage lorsqu'aucune API publique ne couvre son besoin.

## Exemples officiels

- `Minimal` : modèle de départ sans matériel externe.
- `DHT22` : capteur, page/API applicative et MQTT métier.
- `NTPSystemMonitor` : application ESP8266 plus complète, avec routes GET/POST,
  NTP, MQTT et stockage applicatif LittleFS propre.

Mettre à jour la documentation lorsqu'un contrat public ou le processus de
compilation change. Ne stocker aucun identifiant Wi-Fi réel dans Git.

