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
- Web/API et OTA font partie du socle 2.2. Ne pas ajouter MQTT ou une
  dépendance sans besoin explicite.
- Compiler d'abord `examples/Minimal` avec `--library .` pour
  `esp8266:esp8266:nodemcuv2` après chaque changement.
- Ne jamais intégrer de code ou de vocabulaire SMA dans ce dépôt.

## Organisation

`ESPBase` orchestre `DeviceIdentity`, `ConfigStore`, `LogService`,
`WiFiService`, `WebService` et `OtaService`. Le sketch inclut seulement `<ESPBase.h>`.
`ConfigStore` masque son backend et `PlatformCompat` les API du core.
`WebService` fournit le diagnostic, l'administration Wi-Fi et Web OTA. Le mot
de passe AP par défaut public est `ESPbaseSetup`; sa personnalisation est
persistante. La version 2.2.0 n'emploie aucune authentification applicative ; OTA
reste limitée au LAN de confiance. Ne jamais renvoyer ou journaliser un mot de
passe.
Les consommateurs ajoutent au plus quatre routes GET avec
`ESPBase::addGetRoute()` avant `begin()` ; le serveur natif reste interne.
`addPage()` partage cette capacité et ajoute une entrée au shell/navbar. Une
page métier utilise `WebResponse::beginPage()`, `write()` et `endPage()`.

Mettre à jour la documentation lorsqu'un contrat public ou le processus de
compilation change. Ne stocker aucun identifiant Wi-Fi réel dans Git.

