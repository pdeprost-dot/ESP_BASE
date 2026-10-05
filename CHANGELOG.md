# Changelog

## 2.4.1

- Release de packaging et de distribution Arduino, sans changement fonctionnel
  du coeur ESP_BASE.
- Ajout des exemples officiels Minimal, DHT22 et NTPSystemMonitor.
- Ajout de la licence MIT et finalisation des metadonnees Arduino.

## 2.4.0

- Vue publique `WebRequest` pour lire les arguments GET applicatifs.
- Surcharges compatibles de `addGetRoute()` et `addPage()`.
- `addPostRoute()` pour les formulaires POST applicatifs classiques.

## 2.3.0

- `MqttService` générique optionnel basé sur PubSubClient 2.8.
- Configuration persistante migrée sans perte depuis 2.2.0 et page `/mqtt`.
- Topics applicatifs relatifs, publication, quatre souscriptions bornées et
  réabonnement automatique.
- Backoff 5/15/30/60 secondes, diagnostic et suspension pendant OTA.

## 2.2.0

- Wi-Fi V2 : STA1/STA2, failover non bloquant avec settling, scan, saisie
  manuelle, fallback AP et AP Always ON.
- Configuration Web persistante avec migration ConfigStore et mots de passe
  Wi-Fi masqués/affichables.
- OTA générique : ArduinoOTA et Web OTA avec progression, erreurs, état busy et
  reboot différé.
- Validation matérielle sur Wemos D1 mini ESP8266 ; compilation ESP8266
  NodeMCU/Wemos et ESP32.
- Aucune authentification applicative dans cette version : déploiement limité
  à un LAN de confiance.
