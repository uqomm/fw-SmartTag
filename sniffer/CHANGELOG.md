# Changelog - Sniffer

## [Unreleased]
### Changed
- Intervalos LoRa ajustados: 7 s con tags, 15 s sin detección (antes 5 s/10 s)
- Diagrama de flujo simplificado en README principal

### Added
- Documento DECISION_TIMEOUTS.md explicando ajuste conservador

## [0.1.0] - Inicial
### Added
- Detección UWB de tags (DW3000)
- Comunicación LoRa (SX1278)
- Cálculo de distancia con DistanceHandler
- Serialización de tags detectados
- Manejo de comandos UART
- Soporte EEPROM para configuración
