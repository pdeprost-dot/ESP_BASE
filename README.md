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

Le constructeur sans argument utilise `ESP_BASE` et `2.0.0`. Les chaînes
fournies configurent le nom/version visibles dans les logs, la page et l'API.

## Fonctions actuelles

- identité stable dérivée de la puce ;
- configuration Wi-Fi persistante en EEPROM ;
- connexion STA et reconnexion sans attente bloquante ;
- point d'accès de secours lorsque le STA n'est pas configuré ou joignable ;
- petit journal circulaire en RAM ;
- configuration par le moniteur série ou par formulaire Web depuis l'AP ;
- page de diagnostic et API HTTP en lecture seule.

MQTT, OTA et logique métier sont volontairement absents. ESP32 n'est pas
encore validé.

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

Ce tag V1 reste récupérable tel quel. Le développement courant ajoute Web/API,
mais Web OTA, ArduinoOTA, MQTT et les plateformes ESP32 restent non validés.

## Premier démarrage

Ouvrir le moniteur série à 115200 bauds. Sans configuration, l'appareil crée
un AP `<hostname>-setup` et affiche les commandes disponibles. Le port série
est le canal de provisioning de cette phase :

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
formulaire Web. Son mot de passe généré est affiché sur Serial au démarrage de
l'AP, mais n'est pas conservé dans les logs.

## Documentation

- [ARCHITECTURE.md](ARCHITECTURE.md) : composants et flux d'exécution ;
- [CONVENTIONS.md](CONVENTIONS.md) : règles de développement ;
- [AGENTS.md](AGENTS.md) : contexte autonome pour les assistants de code.
- [docs/WEB_API.md](docs/WEB_API.md) : routes, provisioning et sécurité.

