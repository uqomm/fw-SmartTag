# Changelog - Sniffer-Tag Tools

Registro de cambios para las herramientas de control del firmware sniffer-tag.

## [Unreleased]

### Added

#### UWB Timeout Configuration (sniffer_tag_gui.py)
- **8 parámetros UWB configurables** para optimización del protocolo DS-TWR:
  - **RX Timeouts**:
    - PRE_TIMEOUT: Timeout de detección de preámbulo (PAC units, 1-64)
    - RESP_RX_TIMEOUT: Timeout recepción de RESPONSE (μs)
    - FINAL_RX_TIMEOUT: Timeout recepción de FINAL (μs)
  - **TX → RX Delays**:
    - POLL_TX_RESP_RX: Delay POLL envío → RESPONSE recepción (μs)
    - RESP_TX_FINAL_RX: Delay RESPONSE envío → FINAL recepción (μs)
  - **RX → TX Delays**:
    - POLL_RX_RESP_TX: Delay POLL recepción → RESPONSE envío (μs)
    - RESP_RX_FINAL_TX: Delay RESPONSE recepción → FINAL envío (μs)
  - **Ranging Cycle**:
    - RNG_DELAY: Delay entre ciclos de ranging (ms, 100-10000)

- **Comandos LTEL 0x30-0x37**: Mapeo completo para configuración de timeouts
  - 0x30: PRE_TIMEOUT (PAC units)
  - 0x31: RESP_RX_TIMEOUT (microsegundos)
  - 0x32: FINAL_RX_TIMEOUT (microsegundos)
  - 0x33: POLL_TX_RESP_RX (microsegundos)
  - 0x34: RESP_TX_FINAL_RX (microsegundos)
  - 0x35: POLL_RX_RESP_TX (microsegundos)
  - 0x36: RESP_RX_FINAL_TX (microsegundos)
  - 0x37: RNG_DELAY (milisegundos)

- **UI mejorada para timeouts**:
  - Sección dedicada "UWB Protocol Timeouts" con 4 subsecciones
  - Validación por tipo de parámetro (PAC/μs/ms)
  - Diálogo de confirmación con todos los valores y unidades
  - Valores por defecto optimizados para rango >20m

- **Mejoras críticas para largo alcance**:
  - PRE_TIMEOUT aumentado a 12 PAC (mejora detección >20m de 0% a ~50%)
  - RNG_DELAY configurable para balance consumo/frecuencia de medición

### Changed

#### Log Improvements
- **Formato conciso** para errores múltiples (>10):
  - Muestra primeros 3 y últimos 3 errores
  - Línea de separación "... [N] errores intermedios omitidos ..."
  - Mantiene total de errores en resumen final
  - Reduce sobrecarga de logs en condiciones adversas

#### Documentation
- **Documentos técnicos agregados** (documentos/):
  - FORMATO_MEJORADO_LOG_ERRORES.md: Especificación del formato de logs
  - IMPLEMENTACION_UWB_TIMEOUT_CONFIG.md: Guía de implementación firmware
  - RESUMEN_MEJORAS_LOGS_Y_TIMEOUTS.md: Resumen completo de mejoras
  - Análisis de impacto de timeouts para diferentes rangos de operación

### Migration Notes
- **Reorganización de comandos**: gui_tool.py → sniffer_tag_gui.py
  - Comandos de detección mantenidos (0x17-0x21)
  - Comandos de consulta mantenidos (0x11, 0x12, 0x15, 0x16)
  - Comandos de potencia mantenidos (0x20, 0x23, 0x24)
  - **Nuevos comandos 0x30-0x37** para timeouts UWB

### Firmware Integration Required
- [ ] Implementar handlers para comandos 0x30-0x37 en firmware
- [ ] Agregar variables globales para timeouts configurables
- [ ] Integrar con funciones DW3000: `dwt_setpreambledetecttimeout()`, `dwt_setrxtimeout()`
- [ ] Testing en hardware para validar rangos y defaults

### Testing Recommendations
- Validar PRE_TIMEOUT en distancias >20m (mejora esperada: 0% → 50% success rate)
- Medir impacto de RNG_DELAY en consumo de energía
- Verificar estabilidad de timeouts RX bajo condiciones de señal débil
- Documentar valores óptimos por escenario (indoor/outdoor, corto/largo alcance)

---

## [Previous] - October 2025

### Initial Release
- GUI básica para control de sniffer-tag
- Modos de detección (one/multiple)
- Configuración de sleep times y readings
- Monitoreo de tags activos en tiempo real
- Log serial con colores y timestamps
- Estadísticas de detección
- Guardado de logs

