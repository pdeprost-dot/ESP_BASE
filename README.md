# ESP_BASE

ESP_BASE est une bibliothèque Arduino réutilisable, pas un firmware applicatif.
Elle fournit l'infrastructure commune aux petits projets ESP derrière une
façade unique. La première cible réellement validée est l'ESP8266.

## Nouveau projet

Copier `examples/Minimal` puis ajouter uniquement la logique métier :

```cpp
#include <ESPBase.h>

ESPBase espBase("MonProjet", "1.0.0");

void setup() {
  espBase.begin();
}

void loop() {
  espBase.loop();
}
```

Le constructeur sans argument utilise `ESP_BASE` et `2.2.0`. Les chaînes
fournies configurent le nom/version visibles dans les logs, la page et l'API.

Un projet personnalisé indique simplement son identité :

```cpp
ESPBase espBase("Mon Projet", "1.0.0");
```

Une API métier GET s'ajoute avant `begin()` :

```cpp
void handleValue(WebResponse& response, void*) {
  response.sendJson("{\"value\":42}");
}

espBase.addGetRoute("/api/value", handleValue);
```

Une page métier bénéficie du shell et de la navbar ESP_BASE :

```cpp
void handlePage(WebResponse& response, void*) {
  response.beginPage("Capteur");
  response.write("<section class='card'>Valeur</section>");
  response.endPage();
}

espBase.addPage("Capteur", "/capteur", handlePage);
```

ESP_BASE prend en charge l'infrastructure ; le projet consommateur conserve
uniquement sa logique métier.

## Fonctions actuelles

- identité stable dérivée de la puce ;
- configuration Wi-Fi persistante en EEPROM ;
- connexion STA et reconnexion sans attente bloquante ;
- point d'accès de secours lorsque le STA n'est pas configuré ou joignable ;
- petit journal circulaire en RAM ;
- configuration par le moniteur série ou par formulaire Web depuis l'AP ;
- page de diagnostic et API HTTP en lecture seule ;
- routes GET applicatives bornées via `ESPBase::addGetRoute()` ;
- shell Web responsive et pages applicatives via `ESPBase::addPage()`.
- administration Wi-Fi locale et mises à jour ArduinoOTA/Web OTA intégrées au
  socle, sans authentification applicative dans la phase 2.2 actuelle.

MQTT et logique métier sont volontairement absents. ESP32 n'est pas encore
validé sur matériel.

## Compiler

Prérequis validés : Arduino CLI et core `esp8266:esp8266` 3.1.2.

```powershell
arduino-cli compile --warnings all --fqbn esp8266:esp8266:nodemcuv2 `
  --library . examples/Minimal
```

Le dépôt peut aussi être cloné dans le dossier `libraries` d'Arduino puis
ouvert depuis **File > Examples > ESP_BASE > Minimal**. Aucune dépendance
externe au core ESP8266 n'est requise.

## Jalon matériel ESP8266 Base V1

La première plateforme physiquement validée est :

- ESP8266EX ;
- NodeMCU 1.0, FQBN `esp8266:esp8266:nodemcuv2` ;
- flash 4 Mo ;
- Arduino ESP8266 core 3.1.2.

Les fonctions validées sur ce matériel sont l'identité stable, le hostname,
la configuration persistante, le Wi-Fi STA, la reconnexion, l'AP de secours,
le provisioning série, le journal circulaire et le diagnostic de mémoire par
`STATUS`.

Ce tag V1 reste récupérable tel quel. Le développement courant 2.2 ajoute
Web/API, Wi-Fi dual-STA et OTA. Web OTA et ArduinoOTA sont validés sur Wemos
D1 mini ; ESP32 est validé par compilation uniquement et MQTT reste absent.

## Premier démarrage

Ouvrir le moniteur série à 115200 bauds. Sans configuration, l'appareil crée
un AP `<hostname>-setup` protégé par le mot de passe usine public
`ESPbaseSetup` et affiche les commandes disponibles :

```text
WIFI mon-ssid|mon-mot-de-passe
STATUS
LOGS
CLEAR
HELP
```

`WIFI` accepte un mot de passe vide pour un réseau ouvert. La configuration
est enregistrée puis la connexion démarre. `CLEAR` efface uniquement la
configuration Wi-Fi. En mode AP, ouvrir `http://192.168.4.1/` pour utiliser le
formulaire Web. La procédure standard, utilisable sans USB ni Serial, est :

1. rechercher `espbase-XXXXXX-setup` ;
2. se connecter avec `ESPbaseSetup` ;
3. ouvrir `http://192.168.4.1` ;
4. configurer le Wi-Fi LAN ;
5. attendre la connexion STA et l'arrêt automatique de l'AP.

Le propriétaire peut remplacer le mot de passe AP depuis `/wifi`. Il reste
persistant, masqué par défaut dans le formulaire, affichable à la demande et
peut être explicitement rétabli à `ESPbaseSetup`. Aucun mot de passe n'est
renvoyé par l'API ou journalisé.

> **Sécurité 2.2.0 :** l'interface Web, Web OTA et ArduinoOTA ne disposent
> volontairement d'aucune authentification applicative. Ne jamais exposer
> directement ESP_BASE à Internet ou à un réseau non fiable.

## Documentation

- [ARCHITECTURE.md](ARCHITECTURE.md) : composants et flux d'exécution ;
- [CONVENTIONS.md](CONVENTIONS.md) : règles de développement ;
- [AGENTS.md](AGENTS.md) : contexte autonome pour les assistants de code.
- [docs/WEB_API.md](docs/WEB_API.md) : routes, provisioning et sécurité.

