# Architecture

## Boucle principale

Le sketch utilisateur appelle seulement `ESPBase::begin()` et
`ESPBase::loop()`. La façade initialise identité, journal, stockage, Wi-Fi et
Web et OTA puis fait avancer les services et l'interface série sans attente
longue.

## Composants

- `PlatformCompat` : façade des API dépendantes de la puce.
- `DeviceIdentity` : produit une identité et un hostname stables.
- `LogService` : anneau de 16 lignes de 96 octets, également écrit sur Serial.
- `ConfigStore` : stockage versionné STA1/STA2/AP, migration et backend EEPROM
  sur ESP8266 ou Preferences sur ESP32.
- `WiFiService` : machine d'état dual-STA/AP, settling non bloquant, scan,
  failover et reconnexion périodique.
- `WebService` : serveur HTTP, diagnostic, configuration Wi-Fi et Web OTA.
- `OtaService` : ArduinoOTA, écriture Web OTA, progression, erreurs et reboot
  différé. Les transitions Wi-Fi volontaires sont suspendues pendant l'OTA.
- `ESPBase` : façade publique et orchestration des services.
- `Version` : valeurs par défaut du nom et de la version.

## États Wi-Fi

```text
configuration absente       -> AP de secours
STA1 disponible             -> CONNECTED_STA1
STA1 indisponible           -> settling -> tentative STA2
STA1 et STA2 indisponibles  -> AP de secours et retries périodiques
AP Always ON                -> AP+STA lorsque le STA est connecté
```

L'AP n'est pas un portail captif : l'utilisateur ouvre `192.168.4.1`. Le
provisioning de récupération est disponible lorsque l'AP est actif. La page
`/wifi` administre STA1, STA2, le scan et l'AP. Le provisioning Serial reste
disponible comme solution de secours.

## Web

Les réponses sont construites avec de petits buffers fixes ou envoyées par
fragments. Les consommateurs peuvent ajouter quatre routes/pages GET bornées,
sans accès au serveur natif. Voir [`docs/WEB_API.md`](docs/WEB_API.md).

## Bibliothèque Arduino

`library.properties`, `src/` et `examples/Minimal/` suivent la structure
Arduino standard. Un projet inclut uniquement `<ESPBase.h>` ; les autres
classes sont des détails internes et ne doivent pas être orchestrées par son
sketch.

## Portabilité future

Les includes propres aux plateformes restent dans `PlatformCompat`, les
services concernés et le backend de `ConfigStore`. ESP32 est validé par
compilation pour 2.2.0 ; la validation matérielle reste à effectuer.

