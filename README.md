# ESP_BASE

ESP_BASE est une bibliothèque Arduino réutilisable pour ESP8266 et ESP32. Elle
sépare l'infrastructure commune (identité, configuration, réseau, Web, OTA et
MQTT) de la logique métier, qui reste dans le projet consommateur.

## Installation et sketch minimal

Installer ESP_BASE comme bibliothèque Arduino ainsi que `PubSubClient` 2.8,
puis utiliser la façade unique :

```cpp
#include <ESPBase.h>

ESPBase espBase;

void setup() {
  espBase.begin();
}

void loop() {
  espBase.loop();
}
```

`examples/Minimal/Minimal.ino` est le modèle officiel. Le constructeur sans
argument expose le nom `ESP_BASE` et la version `2.4.1`. Une application peut
fournir son propre nom et sa propre version :

```cpp
ESPBase espBase("Mon Projet", "1.0.0");
```

La version est accessible par `espBase.firmwareVersion()`, sur les pages Web,
dans `/api/status` et au démarrage Serial. Sa capacité est de 63 caractères
utiles plus le terminateur nul.

## Fonctions livrées

- `DeviceIdentity` : Device ID et hostname stables dérivés de la puce ;
- `ConfigStore` : configuration persistante et migrations versionnées ;
- Wi-Fi STA1 prioritaire, STA2 de secours, failover et reconnexion non bloquants ;
- AP de secours automatique et mode AP Always ON ;
- scan et configuration Wi-Fi depuis `/wifi` ;
- Web UI, API de diagnostic, journal circulaire et diagnostic mémoire ;
- routes GET et pages applicatives ajoutées sans modifier ESP_BASE ;
- Web OTA et ArduinoOTA avec progression et reboot ;
- MQTT optionnel : publish/subscribe, retained, reconnexion avec backoff
  5/15/30/60 s et réabonnement automatique ;
- suspension immédiate de MQTT pendant Web OTA ou ArduinoOTA.

Tous les services progressent via `begin()`/`loop()` sans attente longue dans
la boucle applicative.

## Premier démarrage et Wi-Fi

Sans réseau utilisable, ESP_BASE crée l'AP `<hostname>-setup` avec le mot de
passe usine `ESPbaseSetup`. Se connecter à cet AP, ouvrir
`http://192.168.4.1/`, puis configurer STA1 et éventuellement STA2. STA1 est
prioritaire ; STA2 est essayé si STA1 reste indisponible. Une fois un STA
connecté, l'AP s'arrête sauf si AP Always ON est activé.

La page `/wifi` permet aussi le scan, la saisie manuelle des SSID, le changement
du mot de passe AP et le mode AP permanent. Le provisioning Serial reste
disponible à 115200 bauds avec `WIFI <ssid>|<password>`, `STATUS`, `LOGS`,
`CLEAR` et `HELP`.

## Extension Web applicative

Les routes doivent être enregistrées avant `espBase.begin()`. Les callbacks
2.3.0 restent acceptés. Un callback peut aussi recevoir une vue temporaire de
la requête afin de lire les arguments GET déjà décodés par le serveur :

```cpp
void handleValue(WebRequest& request, WebResponse& response, void*) {
  const String value = request.arg("value");
  response.sendText(request.hasArg("value") ? value.c_str() : "missing", 200);
}

void setup() {
  espBase.addGetRoute("/api/value", handleValue);
  espBase.begin();
}
```

Une page ajoutée à la navbar utilise le shell Web commun :

```cpp
void handlePage(WebResponse& response, void*) {
  response.beginPage("Capteur");
  response.write("<section class='card'>Valeur</section>");
  response.endPage();
}

void setup() {
  espBase.addPage("Capteur", "/capteur", handlePage);
  espBase.begin();
}
```

`WebResponse` fournit `sendJson()`, `sendText()`, `beginPage()`, `write()` et
`endPage()`. Les callbacks acceptent un pointeur de contexte optionnel. La
capacité partagée est de quatre routes/pages applicatives.

Un formulaire POST classique réutilise le même callback et `WebRequest` :

```cpp
void saveTime(WebRequest& request, WebResponse& response, void*) {
  if (!request.hasArg("server") || !request.hasArg("timezone")) {
    response.sendText("missing", 400);
    return;
  }
  const String server = request.arg("server");
  const String timezone = request.arg("timezone");
  response.sendText("Saved");
}

void setup() {
  espBase.addPostRoute("/time/save", saveTime);
  espBase.begin();
}
```

`addPostRoute()` traite uniquement les arguments d'un formulaire POST classique.
Il n'expose ni corps brut, ni JSON générique, ni upload multipart. Les routes
GET, POST et pages partagent la capacité fixe de quatre routes applicatives.

## MQTT

MQTT est désactivé par défaut. L'utilisateur configure depuis `/mqtt` le
broker, le port, le username/password optionnel et le root topic. Les topics
passés par l'application sont relatifs : ESP_BASE préfixe automatiquement le
root configuré.

```cpp
void onCommand(const char* topic, const uint8_t* payload,
               size_t length, void* context) {
  // Callback court et non bloquant ; payload n'est pas nécessairement terminé par \0.
}

void setup() {
  espBase.begin();
  espBase.mqtt().subscribe("commands/test", onCommand);
}

void publishMeasurement() {
  if (espBase.mqtt().enabled() && espBase.mqtt().connected()) {
    espBase.mqtt().publish("temperature", "24.8", true);
  }
}
```

`retained=true` conserve le dernier état côté broker. Jusqu'à quatre
souscriptions sont enregistrées en mémoire et réinstallées après reconnexion.
Le transport utilise un backoff 5/15/30/60 secondes lorsque le broker est
indisponible. MQTT ne choisit jamais le réseau Wi-Fi. Au début d'une OTA,
ESP_BASE le suspend et le déconnecte immédiatement ; le fonctionnement normal
reprend après le reboot.

## OTA

La page `/ota` accepte un binaire compilé pour la carte cible. ArduinoOTA est
annoncé avec le hostname ESP_BASE lorsque le STA est connecté. Les deux chemins
conservent la configuration persistante, suspendent MQTT et redémarrent après
succès. Web OTA et ArduinoOTA ont été validés physiquement avec l'USB conservé
comme observateur Serial ; le scénario spécifique sans USB reste à vérifier.

## Compilation

Depuis la racine du dépôt :

```powershell
arduino-cli compile --warnings all --fqbn esp8266:esp8266:nodemcuv2 `
  --library . examples/Minimal
```

Plateformes vérifiées pour 2.4.1 :

- ESP8266 NodeMCU et Wemos D1 mini : compilation et validation physique ;
- ESP32 générique : compilation validée, sans campagne physique équivalente.

## Sécurité

ESP_BASE 2.4.1 cible un LAN de confiance. Il n'offre pas encore
d'authentification générale pour l'interface Web, Web OTA ou ArduinoOTA, et le
transport MQTT n'utilise pas TLS. Ne jamais exposer directement ces services à
Internet ou à un réseau non fiable. Les mots de passe Wi-Fi/MQTT ne sont jamais
renvoyés par l'API ni écrits dans les logs ; les formulaires les masquent par
défaut mais permettent leur affichage local volontaire.

## Documentation complémentaire

- [docs/ARDUINO_IDE.md](docs/ARDUINO_IDE.md) : installation et premiers pas
  dans Arduino IDE ;
- [docs/AI_DEVELOPMENT.md](docs/AI_DEVELOPMENT.md) : création d'une application
  ESP_BASE avec une IA de développement ;
- [ARCHITECTURE.md](ARCHITECTURE.md) : composants et orchestration ;
- [CONVENTIONS.md](CONVENTIONS.md) : règles de développement ;
- [AGENTS.md](AGENTS.md) : contexte autonome pour le développement assisté par
  IA ;
- exemples officiels : [`Minimal`](examples/Minimal/Minimal.ino),
  [`DHT22`](examples/DHT22/DHT22.ino) et
  [`NTPSystemMonitor`](examples/NTPSystemMonitor/NTPSystemMonitor.ino) ;
- [docs/WEB_API.md](docs/WEB_API.md) : routes, provisioning et limites Web.
