# Changelog - Persona Tag

Registro de cambios para el firmware del dispositivo Persona (tag personal).

## [Unreleased]

### Added
- **Protocolo UWB responder** (DW3000)
  - Respuesta a queries DS-TWR del sniffer
  - Timestamps de alta precisión
  - Soporte para múltiples sniffers
  - Configuración optimizada para 850 Kbps (2025-11-04)
  - Actualización de timeouts para rate de 850 Kbps

- **Gestión avanzada de energía**
  - Cargador de batería BQ25150
  - Monitoreo de estado de carga
  - Ship mode para ultra-bajo consumo
  - Modos de sleep optimizados:
    - sleep_time_received: Cuando hay comunicación activa
    - sleep_time_not_received: Cuando no hay actividad

- **Indicadores visuales LED RGB**
  - Control de LED WS2812 (PcbLed class)
  - Colores configurables para estados (Ws2812Color)
  - Indicación de estado de batería
  - Señales de comunicación UWB

- **Optimización de consumo**
  - Ciclos de sleep/wake configurables
  - Wake-up automático en queries UWB
  - Gestión inteligente de periféricos
  - Análisis continuo de timeouts y comunicación con sniffer (2025-10-28)

- **Configuración STM32CubeIDE** (2025-11-03/04)
  - Archivos de proyecto y launch settings
  - Configuración de .gitignore para metadata de Eclipse

### Changed
- **Optimización de ciclo de sleep/wake** para mayor eficiencia energética
- **Timeouts UWB actualizados** para 850 Kbps (2025-11-04)
- **Valores de configuración DW3000** ajustados para mejor rendimiento
- Mejoras en respuesta a queries del sniffer
- Migración de configuración main.cpp a 850 Kbps (2025-11-04)

### Fixed
- Estabilidad en transiciones sleep/wake
- Timing de respuestas UWB para largo alcance
- Ajustes en SFD timeout (2025-11-04)
- Corrección de valores en migración a 850 Kbps (2025-11-03)

## [0.1.0] - Inicial
### Added
- Versión inicial del firmware
- Soporte básico para STM32U535VE
- Integración de DW3000 como responder
- Sistema básico de gestión de batería
- Control básico de LED RGB
