# Web et API

Le serveur HTTP écoute sur le port 80 en STA comme sur l'AP de secours.
Il est démarré automatiquement par `ESPBase::begin()` et servi par
`ESPBase::loop()` ; le sketch utilisateur n'inclut pas `WebService.h`.

| Méthode | Route | Fonction |
|---|---|---|
| `GET` | `/` | diagnostic léger |
| `GET` | `/wifi` | administration STA1/STA2, scan et paramètres AP |
| `GET` | `/mqtt` | configuration et diagnostic MQTT |
| `GET` | `/setup` | provisioning de récupération lorsque l'AP est actif |
| `GET` | `/api/status` | état JSON du firmware, Wi-Fi et de la mémoire |
| `GET` | `/api/logs` | journal circulaire en texte UTF-8 |
| `POST` | `/api/wifi/config` | enregistrement STA1/STA2 |
| `GET/POST` | `/api/wifi/scan` | résultat/démarrage du scan asynchrone |
| `POST` | `/api/ap-password` | paramètres et mot de passe AP |
| `POST` | `/api/mqtt` | enregistrement et application de la configuration MQTT |
| `GET` | `/ota` | page Web OTA |
| `POST` | `/api/ota` | installation d'un firmware `.bin` sur le LAN STA |

## Routes applicatives

Une application peut enregistrer avant `begin()` jusqu'à quatre routes GET :

```cpp
void handleValue(WebResponse& response, void* context) {
  response.sendJson("{\"value\":42}");
}

void setup() {
  espBase.addGetRoute("/api/value", handleValue);
  espBase.begin();
}
```

Le callback peut recevoir un contexte utilisateur et répondre avec
`sendJson()` ou `sendText()`. L'enregistrement retourne `false` pour un chemin
invalide ou dupliqué, une capacité dépassée, un appel après `begin()`, ou les
routes réservées `/`, `/wifi`, `/mqtt`, `/logs`, `/system`, `/api/status`,
`/api/logs`, `/api/wifi`, `/api/mqtt`, `/api/ap-password`, `/ota` et
`/api/ota`. Le serveur natif
ESP8266/ESP32 n'est jamais exposé.

Les limites des routes applicatives restent : GET uniquement, quatre routes/pages
applicatives, libellés de 20 caractères, chemins de 47 caractères, fragments
HTML applicatifs et aucun accès public aux paramètres HTTP. ESP32 est validé
par compilation uniquement. WebSocket et SSE ne sont pas inclus.

Une route visible dans la navigation utilise la même capacité avec :

```cpp
void handlePage(WebResponse& response, void*) {
  response.beginPage("Capteur");
  response.write("<section class='card'>Valeur</section>");
  response.endPage();
}

espBase.addPage("Capteur", "/capteur", handlePage);
```

Le shell commun fournit le document HTML, le CSS responsive, le header, la
navigation et le pied de page. La capacité partagée reste de quatre routes ou
pages applicatives, avec des libellés de 20 caractères et chemins de 47
caractères maximum.

## Pages système

| Route | Fonction |
|---|---|
| `/` | accueil et état général |
| `/wifi` | état réseau, STA1/STA2, scan et configuration AP |
| `/mqtt` | broker, credentials, topic racine et diagnostic MQTT |
| `/setup` | provisioning de récupération via l'AP |
| `/logs` | journal circulaire lisible en HTML |
| `/ota` | mise à jour firmware `.bin` avec progression |
| `/system` | identité et diagnostic mémoire |

La navbar est générée par ESP_BASE dans l'ordre : Accueil, pages applicatives,
Wi-Fi, MQTT, Provisioning lorsque pertinent, Logs, OTA, Système.

Le status expose notamment version, identité, uptime, heap courant/minimum,
heap avant/après démarrage Web, état/mode Wi-Fi, SSID, IP, RSSI et état AP.
Il expose aussi l'activation, la connexion et les compteurs MQTT, mais jamais le password.
Le mot de passe Wi-Fi n'est jamais renvoyé par l'API ni journalisé. Le scan
Wi-Fi est asynchrone et ne conditionne pas le fonctionnement.

## Provisioning

Sans configuration valide, rechercher l'AP `<hostname>-setup`, se connecter
avec le mot de passe usine public `ESPbaseSetup`, puis ouvrir
`http://192.168.4.1/`. Après enregistrement, l'ESP tente le STA et arrête l'AP
une fois connecté. Le provisioning Serial reste disponible mais n'est pas
nécessaire à cette procédure.

La section « Point d'accès ESP_BASE » permet au propriétaire de définir un
mot de passe AP personnalisé de 8 à 63 caractères ASCII ou de restaurer
explicitement `ESPbaseSetup`, ainsi que d'activer le mode AP permanent. Les
secrets Wi-Fi ne sont jamais exposés par l'API ni les logs.

## OTA

`OtaService` fournit ArduinoOTA et l'installation Web d'un binaire applicatif.
Les deux mécanismes exigent une connexion STA. Pendant une écriture OTA,
`ESPBase` suspend les transitions volontaires de `WiFiService`, tout en laissant
la boucle applicative progresser. MQTT est suspendu et déconnecté immédiatement
dès le début accepté de l'OTA. Une OTA réussie redémarre l'appareil sans effacer
la configuration persistante.

La page `/ota` affiche la progression de l'upload navigateur. ArduinoOTA utilise
le hostname ESP_BASE et journalise début, progression bornée, fin et erreurs.

## Limites de sécurité

La version 2.3.0 ne comporte volontairement aucune authentification
applicative : ni HTTP Basic, ni mot de passe Web OTA, ni mot de passe
ArduinoOTA. Ces fonctions doivent rester strictement sur un LAN de confiance et
ne doivent pas être exposées à Internet. Les mots de passe conservés sont
ceux de STA1, STA2, de l'AP ESP_BASE et du broker MQTT. Le password MQTT est
masqué par défaut mais affichable sur `/mqtt`; cette page exige donc un LAN de confiance.
