# Architecture

## Boucle principale

Le sketch utilisateur appelle seulement `ESPBase::begin()` et
`ESPBase::loop()`. La façade initialise identité, journal, stockage, Wi-Fi et
Web puis fait avancer les services et l'interface série sans attente longue.

## Composants

- `PlatformCompat` : façade des API dépendantes de la puce.
- `DeviceIdentity` : produit une identité et un hostname stables.
- `LogService` : anneau de 16 lignes de 96 octets, également écrit sur Serial.
- `ConfigStore` : contrat de lecture/écriture indépendant du backend. La phase
  1 emploie EEPROM sur ESP8266 avec magic, version et checksum.
- `WiFiService` : machine d'état STA/AP, timeout et reconnexion périodique.
- `WebService` : serveur HTTP, diagnostic et provisioning depuis l'AP. Il
  utilise `ESP8266WebServer`; l'alias de classe isole le futur header ESP32.
- `ESPBase` : façade publique et orchestration des services.
- `Version` : valeurs par défaut du nom et de la version.

## États Wi-Fi

```text
configuration absente -> AP de secours
configuration présente -> connexion STA
connexion réussie      -> STA, arrêt de l'AP
timeout/perte durable  -> AP+STA et nouvelles tentatives périodiques
```

L'AP n'est pas un portail captif : l'utilisateur ouvre `192.168.4.1`. Le
formulaire n'est affiché et son POST n'est accepté que lorsque l'AP est actif.
Le provisioning Serial reste disponible comme solution de secours.

## Web

Les réponses sont construites avec de petits buffers fixes ou envoyées par
fragments. Les routes métier ne font pas encore partie de l'API publique :
elles seront ajoutées à la façade lorsque le premier cas réel le justifiera,
sans système de plugins généraliste. Voir [`docs/WEB_API.md`](docs/WEB_API.md).

## Bibliothèque Arduino

`library.properties`, `src/` et `examples/Minimal/` suivent la structure
Arduino standard. Un projet inclut uniquement `<ESPBase.h>` ; les autres
classes sont des détails internes et ne doivent pas être orchestrées par son
sketch.

## Portabilité future

Les includes propres aux plateformes restent dans `PlatformCompat`,
`WiFiService` et le backend de `ConfigStore`. Des branches ESP32 préparatoires
existent, mais elles ne sont ni compilées ni validées dans le jalon ESP8266
Base V1. Leur activation future ne devra changer ni les interfaces publiques
ni le sketch.

