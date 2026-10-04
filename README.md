# ESP_BASE

Socle Arduino minimal et réutilisable pour de petits projets ESP. La première
cible réellement validée est l'ESP8266. Les accès propres à la plateforme sont
isolés afin de permettre un portage ultérieur vers ESP32.

## Fonctions de la phase 1

- identité stable dérivée de la puce ;
- configuration Wi-Fi persistante en EEPROM ;
- connexion STA et reconnexion sans attente bloquante ;
- point d'accès de secours lorsque le STA n'est pas configuré ou joignable ;
- petit journal circulaire en RAM ;
- configuration minimale par le moniteur série.

MQTT, serveur Web, OTA et logique métier sont volontairement absents.

## Compiler

Prérequis validés : Arduino CLI et core `esp8266:esp8266` 3.1.2.

```powershell
arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 firmware/ESP_BASE
```

Le sketch n'utilise aucune bibliothèque externe au core ESP8266.

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

Le serveur Web, Web OTA, ArduinoOTA, MQTT et les plateformes ESP32 ne sont pas
implémentés ou validés dans ce jalon.

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
configuration Wi-Fi. L'AP permet de retrouver et d'identifier l'appareil ; un
portail Web sera étudié dans une phase ultérieure. Son mot de passe généré est
affiché sur Serial au démarrage de l'AP, mais n'est pas conservé dans les logs.

## Documentation

- [ARCHITECTURE.md](ARCHITECTURE.md) : composants et flux d'exécution ;
- [CONVENTIONS.md](CONVENTIONS.md) : règles de développement ;
- [AGENTS.md](AGENTS.md) : contexte autonome pour les assistants de code.

