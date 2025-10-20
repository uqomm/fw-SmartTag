# Changelog - Sniffer

## [Unreleased]
### Changed
- Intervalos LoRa ajustados: 7 s con tags, 15 s sin detección (antes 5 s/10 s) [a231aab]
- Diagrama de flujo simplificado en README principal [a231aab]
- Mejorado .gitignore para archivos build y configuración IDE [9cd1f15]

### Added
- Documento DECISION_TIMEOUTS.md explicando ajuste conservador [a231aab]
- README específico del proyecto sniffer [9cd1f15]
- CHANGELOG con historial de commits [9cd1f15]

### Removed
- Proyectos rdss_master_rx_cpp y rdss_master_tx_cpp [9cd1f15]
- Carpetas Debug/ y archivos temporales [9cd1f15]

## [0.2.0] - 2024-10
### Changed
- Modificación Wait_rx_timeout y debug en envío por LoRa [e1f6213]
- Cambio de puerto UART para consulta de parámetros LoRa [dd106b0, 70ea286]
- Recepción UART modificada: de interrupción de 13 bytes a 1 byte [0daa995]

### Removed
- Gateway LoRa eliminado [8a9fe06]

## [0.1.0] - Inicial
### Added
- Detección UWB de tags (DW3000)
- Comunicación LoRa (SX1278)
- Cálculo de distancia con DistanceHandler
- Serialización de tags detectados
- Manejo de comandos UART
- Soporte EEPROM para configuración
