# Développer une application ESP_BASE avec une IA

Ce guide s'adresse à une personne qui souhaite confier une application
ESP_BASE à une IA de développement, quel que soit l'outil utilisé.

1. Installer ESP_BASE ou cloner son dépôt public.
2. Donner à l'IA accès au dépôt complet.
3. Lui demander de lire `README.md`, `AGENTS.md`, la documentation et les
   exemples avant de concevoir l'application.
4. Décrire le besoin applicatif, le matériel et les contraintes sans imposer
   une réimplémentation de l'infrastructure.
5. Laisser l'IA découvrir et employer l'API publique actuelle dans le dépôt.
6. Interdire toute modification d'ESP_BASE, sauf demande volontaire et
   explicitement motivée.
7. Demander une compilation avec warnings et un rapport distinguant clairement
   compilation, tests réalisés et validations matérielles restantes.

Le dépôt public doit rester la source de vérité. Une méthode par étapes est
recommandée : audit sans modification, validation humaine de l'analyse,
implémentation, puis build et tests. Le prompt ci-dessous applique cette méthode
à un cas d'utilisation complet.

## Prompt autonome — Gas Meter Monitor

```text
Tu dois développer une application Arduino complète de monitoring d'un compteur
de gaz pour ESP8266 en utilisant la bibliothèque publique ESP_BASE :

https://github.com/pdeprost-dot/ESP_BASE

Tu ne disposes d'aucun contexte antérieur sur ce projet, son historique ou son
environnement de développement. Le dépôt public ESP_BASE et sa documentation
sont ta seule source de vérité concernant la bibliothèque.

Le but est double :

1. obtenir un moniteur de compteur de gaz réellement fiable ;
2. vérifier qu'une IA sans historique peut construire une application réelle
   au-dessus d'ESP_BASE uniquement grâce au dépôt, à AGENTS.md, à la
   documentation, aux exemples et aux API publiques.

Il ne s'agit pas de fabriquer rapidement une démonstration en contournant
ESP_BASE. Si une capacité nécessaire est difficile ou impossible à réaliser
proprement avec l'API publique disponible, signale-le clairement. Ne contourne
pas silencieusement la bibliothèque.

============================================================
RÈGLES ESP_BASE
============================================================

Commence par lire intégralement `AGENTS.md`, puis `README.md`, la documentation,
les exemples et les headers publics pertinents du dépôt.

Avant d'écrire du code :

- identifie la version réellement disponible ;
- identifie les API publiques réellement disponibles ;
- vérifie les conventions de compilation et d'utilisation ;
- examine les exemples officiels ;
- lis `docs/ARDUINO_IDE.md` s'il existe dans cette version.

Considère ESP_BASE comme une bibliothèque externe stable.

NE MODIFIE PAS ESP_BASE pour faciliter cette application.

Ne réimplémente pas en parallèle ses services génériques. N'invente aucune API
ESP_BASE. Utilise seulement les contrats publics réellement présents et
documentés.

Si une capacité publique indispensable manque, arrête-toi et rapporte
précisément le besoin, la limite constatée et les solutions possibles avant de
modifier la bibliothèque ou de créer un contournement.

============================================================
MATÉRIEL DU CAS D'ÉTUDE
============================================================

- Wemos D1 mini ;
- ESP8266 ;
- module TCRT5000 disposant d'une sortie analogique et d'une sortie digitale ;
- transport MQTT fourni par ESP_BASE.

Câblage de référence :

- `A0` : sortie analogique du TCRT5000 ;
- `D4` / GPIO2 : sortie digitale du TCRT5000.

Contraintes matérielles :

- `D4` correspond à GPIO2 ;
- GPIO2 intervient dans le démarrage de l'ESP8266 ;
- vérifie que le TCRT5000 et son état au démarrage ne compromettent pas le boot ;
- vérifie les niveaux électriques avant raccordement ;
- ne dépasse jamais la tension admissible sur A0 ;
- ne suppose pas les caractéristiques électriques exactes d'un module TCRT5000
  inconnu ;
- documente les vérifications et adaptations nécessaires au module réel.

============================================================
FONCTIONNALITÉS
============================================================

L'application doit détecter les impulsions du compteur et fournir au minimum :

- `pulse_count` ;
- `total_m3` ;
- `flow_m3_h` ;
- `hour_m3` ;
- `day_m3` ;
- `week_m3` ;
- `analog_raw` ;
- `digital_state`.

Le paramètre `volume_par_impulsion_m3` doit être configurable. La valeur
initiale proposée est `0.01 m³ / impulsion`, mais elle ne doit pas être présentée
comme universelle.

Le débit instantané doit être calculé principalement à partir de l'intervalle
entre deux impulsions et revenir à zéro après un timeout configurable ou
justifié.

La convention hebdomadaire est lundi → dimanche.

============================================================
ACQUISITION ET FILTRAGE
============================================================

Le comptage doit être robuste, déterministe et non bloquant.

Prévois un filtrage configurable permettant de rejeter :

- les rebonds ;
- les doubles comptages ;
- les parasites ;
- les événements trop rapprochés ou physiquement impossibles.

Si une interruption GPIO est utilisée, son ISR doit être extrêmement courte.

Sont interdits dans l'ISR :

- MQTT ;
- écriture Flash ;
- traitement Web ;
- allocation dynamique ;
- calcul lourd ;
- logs volumineux.

L'ISR peut uniquement capturer l'information minimale nécessaire. La validation,
les calculs et les actions applicatives restent hors ISR.

La logique de comptage ne doit jamais dépendre de la disponibilité du Wi-Fi, du
Web ou de MQTT.

============================================================
CALIBRATION TCRT5000
============================================================

Utilise réellement A0 pour le diagnostic, le positionnement physique du capteur
et la calibration.

L'interface Web doit permettre d'observer facilement :

- `analog_raw` ;
- `digital_state`.

Le rafraîchissement doit être assez rapide pour positionner et calibrer le
capteur, sans générer un trafic réseau ou une charge excessive.

Ne stocke jamais continuellement les lectures A0 en Flash.

============================================================
TEMPS ET PÉRIODES
============================================================

Ne présume pas qu'ESP_BASE fournit un service NTP ou une API de temps.

1. Examine les API publiques réellement disponibles dans la version utilisée.
2. Utilise un service ESP_BASE seulement s'il existe réellement et convient au
   besoin.
3. Sinon, détermine ce qui doit appartenir proprement à l'application.
4. N'invente jamais une API ESP_BASE.

Applique le même principe au stockage applicatif.

Gère correctement :

- le démarrage sans heure valide ;
- une synchronisation obtenue ultérieurement ;
- les changements d'heure, de jour et de semaine ;
- le redémarrage ;
- le fuseau horaire ;
- le passage de lundi à une nouvelle semaine.

Explique le comportement des statistiques heure/jour/semaine avant et après la
synchronisation de l'heure.

============================================================
PERSISTANCE FLASH
============================================================

Le total du compteur doit survivre aux redémarrages et aux coupures brutales.

Ne jamais écrire la Flash à chaque impulsion.

Le comptage actif reste en RAM. Effectue un checkpoint approximativement toutes
les cinq minutes, mais seulement lorsque l'état persistant a réellement changé.

Principe fondamental :

ZÉRO CONSOMMATION = ZÉRO ÉCRITURE PÉRIODIQUE INUTILE

Prévois :

- une version du format persistant ;
- un contrôle d'intégrité ;
- des valeurs par défaut sûres ;
- la détection de données invalides ou corrompues ;
- une restauration sûre ;
- la coupure brutale comme situation normale.

Documente :

- les données persistées ;
- la fréquence réelle des écritures ;
- la perte maximale possible en cas de coupure ;
- le nombre maximal théorique d'écritures par jour ;
- l'impact estimé sur l'endurance Flash.

Si ESP_BASE offre réellement un stockage applicatif public adapté, utilise-le.
Sinon, implémente le stockage dans l'application sans modifier ESP_BASE.

CONFIGURATION UTILISATEUR et ÉTAT DU COMPTEUR sont deux choses différentes.

Une modification de configuration ne doit jamais effacer le total. Toute remise
à zéro éventuelle doit être explicite, protégée et confirmée.

============================================================
INTERFACE WEB
============================================================

Utilise l'API Web publique réellement disponible dans ESP_BASE. Ne crée pas un
deuxième serveur HTTP si cette API permet de réaliser le besoin.

Ajoute une page `/gas`, ou un nom équivalent cohérent, intégrée à l'interface
ESP_BASE.

Elle doit afficher au minimum :

CAPTEUR

- `analog_raw` ;
- `digital_state` ;
- dernière impulsion ;
- âge de la dernière impulsion.

CONSOMMATION

- `pulse_count` ;
- `total_m3` ;
- `flow_m3_h` ;
- `hour_m3` ;
- `day_m3` ;
- `week_m3`.

SYSTÈME

- heure ;
- état du temps ou de la synchronisation ;
- état MQTT ;
- dernier checkpoint ;
- dernière publication MQTT.

Prévois une configuration applicative pour :

- le volume par impulsion ;
- le filtre ou temps mort entre impulsions ;
- l'intervalle MQTT.

L'intervalle MQTT par défaut est 300 secondes.

Utilise les routes GET/POST publiques réellement disponibles. Valide strictement
toutes les entrées. Une modification de configuration ne doit jamais
réinitialiser accidentellement le compteur.

============================================================
MQTT
============================================================

Utilise exclusivement le service MQTT public ESP_BASE.

Ne crée ni deuxième connexion MQTT, ni deuxième instance PubSubClient.

Publie périodiquement, et non à chaque impulsion. L'intervalle par défaut est
300 secondes.

Publie au minimum :

- `pulse_count` ;
- `total_m3` ;
- `flow_m3_h` ;
- `hour_m3` ;
- `day_m3` ;
- `week_m3` ;
- `analog_raw` ;
- `digital_state`.

Ajoute lorsque pertinent :

- timestamp ;
- uptime ;
- `time_synced` ;
- âge de la dernière impulsion.

Utilise des topics relatifs applicatifs et laisse ESP_BASE gérer son mécanisme
de topic racine conformément à sa documentation. Les données d'état peuvent
être retained si le contrat public ESP_BASE le permet et si ce choix est
pertinent.

Documente les topics, le payload, les unités et un exemple sans identifiant
local.

============================================================
ARCHITECTURE INDICATIVE
============================================================

L'organisation suivante est indicative :

- Sensor → acquisition analogique et digitale ;
- PulseCounter → validation et filtrage ;
- GasStatistics → calculs ;
- Persistence → checkpoint et restauration ;
- GasWeb → affichage et configuration Web ;
- GasMqtt → publication.

Observe d'abord les conventions du dépôt ESP_BASE et adapte cette organisation
proprement. Ne multiplie pas les abstractions sans utilité.

Le code doit rester simple, lisible, déterministe, non bloquant et raisonnable
pour les ressources d'un ESP8266. Évite les allocations dynamiques répétées.

============================================================
ARDUINO IDE ET LIVRABLES
============================================================

Le projet doit être utilisable manuellement dans Arduino IDE.

Produis un projet autonome dont le dossier et le fichier `.ino` principal
respectent les conventions Arduino, par exemple :

GasMeterMonitor/GasMeterMonitor.ino

Ajoute uniquement les fichiers applicatifs nécessaires.

Livre :

- les sources ;
- un README ;
- une description de l'architecture ;
- le câblage ;
- la procédure de calibration du TCRT5000 ;
- le contrat MQTT ;
- la stratégie de persistance Flash ;
- la procédure Arduino IDE ;
- la procédure de test ;
- un CHANGELOG ;
- une version initiale clairement définie.

Le README doit expliquer l'installation d'ESP_BASE, les dépendances, l'ouverture
du projet, la sélection Wemos D1 mini, le choix du port série, la compilation,
le téléversement et l'utilisation du moniteur série.

Ne mets dans les sources ou la documentation :

- aucun SSID réel ;
- aucun mot de passe ;
- aucune IP privée propre à une installation ;
- aucun broker personnel ;
- aucun credential MQTT ;
- aucun port série supposé ;
- aucun chemin local ;
- aucun secret.

============================================================
TESTS DÉTERMINISTES
============================================================

Prévois et exécute autant que possible des tests couvrant :

- première impulsion ;
- impulsions successives ;
- rejet d'une impulsion trop rapprochée ;
- conversion impulsions → m³ ;
- calcul du débit ;
- retour du débit à zéro après timeout ;
- changement d'heure ;
- changement de jour ;
- changement de semaine ;
- restauration après reboot ;
- données persistantes corrompues ;
- checkpoint après changement ;
- absence de checkpoint lorsque rien n'a changé.

============================================================
VALIDATION MATÉRIELLE CONDITIONNELLE
============================================================

Ne suppose jamais qu'un matériel est connecté.

Si un Wemos D1 mini / ESP8266 compatible est physiquement accessible à la
machine de développement :

- identifie avec certitude la carte et son port avant tout flash ;
- ne flashe jamais arbitrairement un périphérique ;
- compile et téléverse pour la cible correcte ;
- observe le boot au moniteur série ;
- vérifie ESP_BASE, le Web, A0, D4, MQTT et la persistance ;
- teste les impulsions si le TCRT5000 est réellement raccordé ;
- vérifie que GPIO2 ne perturbe pas le boot.

Si aucun matériel compatible ou aucun TCRT5000 n'est disponible, ne bloque pas
inutilement le projet. Réalise le build et tous les tests logiciels possibles,
puis marque explicitement le reste :

NON TESTÉ PHYSIQUEMENT

Ne prétends jamais avoir réalisé un test que tu n'as pas réellement effectué.

============================================================
MÉTHODE OBLIGATOIRE PAR ÉTAPES
============================================================

ÉTAPE 1 — AUDIT

NE MODIFIE RIEN.

- lis intégralement AGENTS.md ;
- lis README, la documentation, les exemples et l'API publique ESP_BASE ;
- identifie la version et les capacités réellement disponibles ;
- identifie les dépendances et contraintes de la cible ;
- compare les besoins de l'application aux API publiques ;
- si du matériel est accessible, identifie le Wemos sans le flasher.

Produis un rapport court indiquant :

- ce que fournit ESP_BASE ;
- ce qui doit appartenir à l'application ;
- les limites ou capacités manquantes ;
- l'architecture proposée ;
- le plan de build et de validation ;
- les questions ou décisions réellement nécessaires.

Puis STOP et attends la validation humaine avant toute implémentation.

ÉTAPE 2 — IMPLÉMENTATION

Seulement après validation explicite de l'audit. Implémente l'application sans
modifier ESP_BASE.

ÉTAPE 3 — BUILD ET TESTS

Compile avec les warnings activés et exécute les tests logiciels déterministes.

ÉTAPE 4 — FLASH RÉEL

Uniquement si un matériel compatible a été identifié avec certitude et si le
flash est autorisé.

ÉTAPE 5 — VALIDATION

Vérifie le comportement applicatif, ESP_BASE, le Web, MQTT, la persistance et le
matériel réellement disponible.

ÉTAPE 6 — BILAN

Distingue toujours clairement :

- TESTÉ RÉELLEMENT ;
- NON TESTÉ / À TESTER.

Avant de terminer, vérifie également que :

- ESP_BASE n'a pas été modifié ;
- aucun service ESP_BASE n'a été réimplémenté ;
- aucune API inexistante n'a été inventée ;
- aucun secret ou paramètre local n'a été ajouté ;
- les résultats physiques annoncés correspondent à des observations réelles.

Commence maintenant uniquement par l'ÉTAPE 1 — AUDIT.
```
