# Architecture

## Boucle principale

`setup()` initialise les services dans l'ordre identité, journal, stockage et
Wi-Fi. `loop()` appelle fréquemment `WiFiService::tick()` et traite une petite
interface série de provisioning. Aucun `delay()` long n'est utilisé.

## Composants

- `PlatformCompat` : façade des API dépendantes de la puce.
- `DeviceIdentity` : produit une identité et un hostname stables.
- `LogService` : anneau de 16 lignes de 96 octets, également écrit sur Serial.
- `ConfigStore` : contrat de lecture/écriture indépendant du backend. La phase
  1 emploie EEPROM sur ESP8266 avec magic, version et checksum.
- `WiFiService` : machine d'état STA/AP, timeout et reconnexion périodique.

## États Wi-Fi

```text
configuration absente -> AP de secours
configuration présente -> connexion STA
connexion réussie      -> STA, arrêt de l'AP
timeout/perte durable  -> AP+STA et nouvelles tentatives périodiques
```

L'AP n'est pas un portail captif dans cette phase. La saisie des identifiants
se fait par Serial ; cette limite garde la première couche petite et testable.

## Portabilité future

Les includes propres aux plateformes restent dans `PlatformCompat`,
`WiFiService` et le backend de `ConfigStore`. Des branches ESP32 préparatoires
existent, mais elles ne sont ni compilées ni validées dans le jalon ESP8266
Base V1. Leur activation future ne devra changer ni les interfaces publiques
ni le sketch.

