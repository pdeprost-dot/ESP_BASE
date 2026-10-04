# Conventions

- C++ Arduino lisible, classes petites et responsabilités explicites.
- API publique normale limitée à `ESPBase`; services internes hors du sketch.
- Un nouveau projet part de `examples/Minimal`, sans copier `src/`.
- Méthodes de cycle de vie nommées `begin()` et `tick()`.
- Temporisations calculées avec `millis()` et comparaisons sûres au débordement.
- Pas d'attente active dans les services.
- Buffers fixes dimensionnés et toujours terminés par `\0`.
- Secrets acceptés en entrée mais jamais affichés ni journalisés.
- Logs courts, bornés et préfixés par le sous-système.
- Configuration persistante versionnée et validée avant utilisation.
- Dépendances externes ajoutées seulement si le core Arduino ne suffit pas.
- La cible de compilation de référence est `esp8266:esp8266:nodemcuv2`.
- La compilation de référence utilise `--library . examples/Minimal`.

