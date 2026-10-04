# Web et API

Le serveur HTTP écoute sur le port 80 en STA comme sur l'AP de secours.
Il est démarré automatiquement par `ESPBase::begin()` et servi par
`ESPBase::loop()` ; le sketch utilisateur n'inclut pas `WebService.h`.

| Méthode | Route | Fonction |
|---|---|---|
| `GET` | `/` | diagnostic léger ; formulaire Wi-Fi lorsque l'AP est actif |
| `GET` | `/api/status` | état JSON du firmware, Wi-Fi et de la mémoire |
| `GET` | `/api/logs` | journal circulaire en texte UTF-8 |
| `POST` | `/api/wifi` | enregistrement SSID/mot de passe, uniquement en mode AP |

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
routes réservées `/`, `/wifi`, `/logs`, `/system`, `/api/status`, `/api/logs`
et `/api/wifi`. Le serveur natif ESP8266/ESP32 n'est jamais exposé.

Les limites volontaires de 2.1.0 sont : GET uniquement, quatre routes/pages
applicatives, libellés de 20 caractères, chemins de 47 caractères, fragments
HTML applicatifs et aucun accès public aux paramètres HTTP. ESP32 est validé
par compilation uniquement. OTA, MQTT, WebSocket et SSE ne sont pas inclus.

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
| `/wifi` | état réseau et provisioning uniquement lorsque l'AP est actif |
| `/logs` | journal circulaire lisible en HTML |
| `/system` | identité et diagnostic mémoire |

La navbar est générée par ESP_BASE dans l'ordre : Accueil, pages applicatives,
Wi-Fi, Logs, Système.

Le status expose notamment version, identité, uptime, heap courant/minimum,
heap avant/après démarrage Web, état/mode Wi-Fi, SSID, IP, RSSI et état AP.
Le mot de passe Wi-Fi n'est jamais renvoyé ni journalisé. Le scan Wi-Fi est
reporté : il n'est pas nécessaire au fonctionnement et demanderait une machine
d'état supplémentaire.

## Provisioning

Sans configuration valide, se connecter à l'AP `<hostname>-setup`, puis ouvrir
`http://192.168.4.1/`. Après enregistrement, l'ESP tente le STA et arrête l'AP
une fois connecté. Le provisioning Serial reste disponible.

## Limites de sécurité

HTTP n'est ni chiffré ni authentifié. Le diagnostic est destiné à un LAN de
confiance et le formulaire au seul AP de secours. Le POST Wi-Fi est refusé en
mode STA. Ne pas exposer le port 80 à Internet. Reboot, effacement Web, OTA et
commandes arbitraires ne sont pas implémentés.
