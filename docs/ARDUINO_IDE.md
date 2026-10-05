# Utiliser ESP_BASE avec Arduino IDE

## Installation

Installer d'abord le core correspondant à la carte depuis le gestionnaire de
cartes Arduino IDE : ESP8266 ou ESP32.

Pour installer une release ESP_BASE au format ZIP :

1. ouvrir **Croquis > Inclure une bibliothèque > Ajouter la bibliothèque
   .ZIP…** ;
2. sélectionner le fichier `ESP_BASE-<version>.zip` de la release ;
3. laisser Arduino IDE installer l'archive, sans la décompresser manuellement.

ESP_BASE nécessite **PubSubClient 2.8**. Si Arduino IDE ne propose pas ou
n'installe pas automatiquement cette dépendance, ouvrir le gestionnaire de
bibliothèques, rechercher `PubSubClient`, sélectionner la version 2.8 et
l'installer. PubSubClient n'est pas embarqué dans l'archive ESP_BASE.

## Vérifier l'installation

Ouvrir **Fichier > Exemples > ESP_BASE**. Les exemples suivants doivent être
visibles :

- `Minimal` ;
- `DHT22` ;
- `NTPSystemMonitor`.

Commencer par `Minimal` : sélectionner la carte et son port série, ouvrir
l'exemple, compiler, téléverser, puis observer le démarrage sur le moniteur
série à 115200 bauds.

## Premier démarrage et Wi-Fi

La configuration Wi-Fi est persistante. ESP_BASE tente les réseaux STA
configurés. Si aucun n'est utilisable, il démarre l'AP de secours
`<hostname>-setup`. Se connecter à cet AP avec le mot de passe public par défaut
`ESPbaseSetup`, puis ouvrir `http://192.168.4.1/` pour configurer STA1 et, si
nécessaire, STA2. Une fois un STA connecté, l'AP s'arrête sauf si le mode AP
permanent est activé.

## Exemples

### Minimal

Socle ESP_BASE sans matériel externe. C'est le premier test recommandé.

### DHT22

Exemple de capteur DHT22, historiquement câblé sur `D4` / GPIO2 avec ESP8266.
Installer depuis le gestionnaire de bibliothèques :

- DHT sensor library 1.4.7 ;
- Adafruit Unified Sensor 1.1.15.

### NTPSystemMonitor

Exemple actuellement réservé à ESP8266, sans matériel externe. Il combine NTP,
fuseau horaire et DST, LittleFS, MQTT et informations système. Il montre aussi
l'utilisation de `WebRequest`, `addPage()` et `addPostRoute()` dans une
application qui conserve sa propre configuration.

## Utiliser ESP_BASE dans un sketch

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

La logique applicative s'ajoute autour de cette façade sans recopier les
services génériques ESP_BASE. Les traitements périodiques doivent rester non
bloquants.

## Mettre à jour ESP_BASE

Fermer les sketches qui utilisent ESP_BASE, supprimer l'ancienne copie de la
bibliothèque si l'IDE ne la remplace pas, puis installer le ZIP de la nouvelle
release. Ne conserver qu'une seule copie de `ESP_BASE` dans les dossiers de
bibliothèques Arduino afin d'éviter une résolution ambiguë.

## Dépannage

- `ESPBase.h: No such file or directory` : vérifier que le ZIP a été installé
  comme bibliothèque et qu'une seule copie ESP_BASE est présente.
- `PubSubClient.h: No such file or directory` : installer PubSubClient 2.8 avec
  le gestionnaire de bibliothèques.
- Erreur de compilation liée à la cible : vérifier la carte et le core
  ESP8266/ESP32 sélectionnés.
- Échec de téléversement ou port absent : vérifier le port série et le pilote
  USB de la carte.
- `DHT.h` ou `Adafruit_Sensor.h` introuvable : installer les deux dépendances
  indiquées pour l'exemple DHT22.
- Mauvaise version utilisée : rechercher et supprimer les copies multiples de
  ESP_BASE dans les différents sketchbooks ou dossiers `libraries`.
