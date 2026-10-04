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
