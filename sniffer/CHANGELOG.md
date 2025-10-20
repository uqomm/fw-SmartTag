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

## [0.3.0] - 2024-03
### Changed
- Modificación Wait_rx_timeout y debug en envío por LoRa [e1f6213]
- Cambio de puerto UART para consulta de parámetros LoRa mediante mini PC [dd106b0, 70ea286]
- Recepción UART modificada: de interrupción de 13 bytes a 1 byte [0daa995]
- Modificación tamaño buffer de transmisión LoRa [ba9d21b]
- Arreglo de función transmit de Lora (size de buffer) [f9d7538]

### Added
- Configuración local de LoRa mediante UART (software y mini PC) [215c52e, 36c74d6]
- Guardado de configuración LoRa en memoria EEPROM [0c97280, d3eab23, a6cee0d]
- Implementación getter y setter para configuración LoRa desde FskTester [8272cad]
- Funcionalidad para configurar LoRa con datos guardados en EEPROM [99b22a1]
- Interrupciones UART1 y UART2 para configuración desde FskTester y mini PC [2776492]
- Check de configuración previamente almacenada [027602f, a768bba]

### Fixed
- Corrección de comandos get/set del Lora de forma local [92dbf50]
- Corrección de índices de enum de la clase Lora [92dbf50]

### Removed
- Gateway LoRa eliminado [8a9fe06]

## [0.2.0] - 2024-02
### Changed
- Modificación de frecuencias LoRa: downlink 149.5 MHz, uplink 173.5 MHz [03494be, 5d0c887]
- Cambio de baudrate conversor serial de 255000 a 115200 [5d0c887]
- Modificación UART para recepción de comandos de configuración LoRa [ea71ee0]
- Cambio colores LED Tag Portátil [22349fe]
- Refactor: conversión else-if a switch-case [c7afedb]

### Added
- Comando ShipMode para tags (bajo consumo extremo) [cb49077, 6a2351b]
- Botón para condicionar envío de comando ShipMode [5d0c887]

### Fixed
- Corrección reseteo de variable tag.ship_mode [c59018c]

## [0.1.0] - Inicial
### Added
- Detección UWB de tags (DW3000)
- Comunicación LoRa (SX1278)
- Cálculo de distancia con DistanceHandler
- Serialización de tags detectados
- Manejo de comandos UART
- Soporte EEPROM para configuración
