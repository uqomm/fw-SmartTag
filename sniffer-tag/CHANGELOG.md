# Changelog - Sniffer-Tag Firmware

Registro de cambios para el firmware del dispositivo sniffer-tag.

## [Unreleased]

### Added
- **Protocolo UWB DS-TWR** con DW3000
  - Ranging bidireccional con múltiples tags
  - Soporte para hasta 255 tags por sniffer
  - Timestamps de alta precisión para cálculo de distancia
  - Diagnósticos OTP para chips duales DW3000 (2025-10-28)
  - Estructuras pdw3000local separadas para mejor gestión

- **Comunicación LoRa**
  - Módulo SX1278 para transmisión de distancias
  - Configuración de frecuencia, potencia y spreading factor
  - Envío de paquetes con ID de tag y distancia medida
  - LoRa send timeout aumentado a 16 segundos (2025-12-10)
  - Actualización de tiempos de envío Sniffer a Servidor (2025-12-15)

- **Sistema de comandos UART**
  - Interfaz de configuración vía UART1
  - Comandos para configuración de timeouts UWB (0x30-0x37)
  - Comandos de control y diagnóstico
  - Tooltips informativos para parámetros UWB (2025-10-28)

- **Gestión de memoria EEPROM**
  - Almacenamiento persistente de configuración
  - IDs de tags conocidos
  - Parámetros UWB configurables

- **8 parámetros UWB configurables** (2025-10-28):
  - PRE_TIMEOUT: Timeout de detección de preámbulo (PAC units)
  - RESP_RX_TIMEOUT: Timeout recepción de RESPONSE (μs)
  - FINAL_RX_TIMEOUT: Timeout recepción de FINAL (μs)
  - POLL_TX_RESP_RX: Delay POLL envío → RESPONSE recepción (μs)
  - RESP_TX_FINAL_RX: Delay RESPONSE envío → FINAL recepción (μs)
  - POLL_RX_RESP_TX: Delay POLL recepción → RESPONSE envío (μs)
  - RESP_RX_FINAL_TX: Delay RESPONSE recepción → FINAL envío (μs)
  - RNG_DELAY: Delay entre ciclos de ranging (ms)

- **Sistema de logging diferido** (2025-10-23)
  - Reducción de overhead en tiempo real
  - Formato conciso para errores repetitivos (>10 errores)
  - Muestra primeros 3 y últimos 3 errores con separador

- **Configuración STM32CubeIDE** (2025-11-03/04)
  - Archivos de proyecto y launch settings
  - Configuración de .gitignore para metadata de Eclipse

### Changed
- **Optimización de timeouts UWB para largo alcance (>20m)**
  - PRE_TIMEOUT aumentado para mejor detección >20m (2025-10-27)
  - Configuración optimizada para 850 Kbps (2025-11-04)
  - Estados de detección mejorados en loop principal

- **HAL_Delay crítico aumentado a 8ms** (2025-12-09)
  - Arreglo provisorio para detección con 1 tag
  - Funciona correctamente en pruebas de campo

### Fixed
- **Eliminación de HAL_Delay crítico** en MULTIPLE_DETECTION >20m (2025-10-28)
- Corrección de discordancias en lecturas UWB
- Resolución de problemas con canal A SPI CS
- Corrección de valores de timeouts en migración a 850 Kbps (2025-11-03)

## [0.1.0] - Inicial
### Added
- Versión inicial del firmware
- Soporte básico para STM32G474RE
- Integración básica de DW3000
- Comunicación LoRa básica
