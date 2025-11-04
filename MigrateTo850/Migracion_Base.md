# Migración a 850 Kbps - Plan Base

## Introducción
Este documento describe el paso a paso para migrar el sistema SmartLocate (Sniffer y Tag Persona) de 6.8 Mbps a 850 Kbps. El objetivo es extender el rango a 30-50m con +6 dB de sensibilidad adicional, manteniendo compatibilidad y precisión.

**Justificación**: Basado en manual DW3000, hoja de datos y análisis de código. Cambios incluyen data rate, preámbulo, PAC y timeouts escalados.

**Riesgo**: Bajo (cambios reversibles). Tiempo estimado: 2-3 días desarrollo + 1-2 días testing.

---

## Paso 1: Preparación y Backup
- **Backup**: Crear rama Git o copias de `sniffer-tag/Core` y `Persona/Core`.
- **Revisar Configuración Actual**: Confirmar valores en `uwb3000Fxx.h` (PRE_TIMEOUT_6M8=8, etc.).
- **Herramientas**: Compilador STM32CubeIDE, analizador de logs.

---

## Paso 2: Cambios en Sniffer (`sniffer-tag/Core`)
### 2.1 Actualizar `uwb3000Fxx.h`
- Usar `DWT_BR_850K` en lugar de `DWT_BR_6M8` (las definiciones ya existen; actualizar referencias en código)
- Usar `DWT_PLEN_1024` en lugar de `DWT_PLEN_128`
- Usar `DWT_PAC32` en lugar de `DWT_PAC8`
- Actualizar timeouts:
  - `POLL_TX_TO_RESP_RX_DLY_UUS_6M8 = 700` → `POLL_TX_TO_RESP_RX_DLY_UUS_850K = 5600` (escalado ×8)
  - `RESP_RX_TIMEOUT_UUS_6M8 = 250` → `RESP_RX_TIMEOUT_UUS_850K = 2000` (escalado ×8)
- Actualizar `PRE_TIMEOUT_6M8 = 8` (mantener), `PRE_TIMEOUT_850K = 32` (calculado)

### 2.2 Actualizar `uwb3000Fxx.c`
- Modificar `defatult_dwt_config` para usar nuevos valores.
- Ajustar SFD timeout: `(1025 + 1 + 8 - 32) = 1002` (recalculado).
- Verificar antenna delay y calibración.

### 2.4 Actualizar `sniffer_tag.cpp`
- Actualizar constantes de timeout y delays:
  - `POLL_RX_TO_RESP_TX_DLY_UUS_6M8` → `POLL_RX_TO_RESP_TX_DLY_UUS_850K`
  - `RESP_RX_TIMEOUT_UUS_6M8` → `RESP_RX_TIMEOUT_UUS_850K`
  - `PRE_TIMEOUT_6M8` → `PRE_TIMEOUT_850K`
  - `RESP_TX_TO_FINAL_RX_DLY_UUS_6M8` → `RESP_TX_TO_FINAL_RX_DLY_UUS_850K`
  - `FINAL_RX_TIMEOUT_UUS_6M8` → `FINAL_RX_TIMEOUT_UUS_850K`
- Actualizar configuración del DW3000 en `init_uwb_device()` y `reset_actual_hw()`: `DWT_BR_6M8` → `DWT_BR_850K`, `DWT_PLEN_128` → `DWT_PLEN_1024`, `DWT_PAC8` → `DWT_PAC32`.
- Ajustar SFD timeout en funciones: `(129 + 8 - 8)` → `(1025 + 1 + 8 - 32)`.
- Actualizar `setup_and_transmit()` y `setup_and_transmit_for_timestamp_query()` para usar nuevas constantes.

### 2.3 Compilar y Flashear
- Compilar proyecto Sniffer.
- Flashear en dispositivo.
- Verificar logs iniciales (sin errores de configuración).

---

## Paso 3: Cambios en Tag Persona (`Persona/Core`)
### 3.1 Actualizar `uwb3000Fxx.h`
- Mismos cambios que Sniffer (data rate, preámbulo, PAC, timeouts).
- Asegurar consistencia entre dispositivos.

### 3.2 Actualizar `uwb3000Fxx.c`
- Modificar configuración DW3000 idéntica al Sniffer.
- Verificar delays de respuesta.

### 3.3 Actualizar `human_tag.c`
- Actualizar constantes de timeout y delays:
  - `POLL_RX_TO_RESP_TX_DLY_UUS_6M8` → `POLL_RX_TO_RESP_TX_DLY_UUS_850K`
  - `PRE_TIMEOUT_6M8` → `PRE_TIMEOUT_850K`
  - `RESP_TX_TO_FINAL_RX_DLY_UUS_6M8` → `RESP_TX_TO_FINAL_RX_DLY_UUS_850K`
  - `FINAL_RX_TIMEOUT_UUS_6M8` → `FINAL_RX_TIMEOUT_UUS_850K`
- Actualizar funciones como `process_first_tag_information()`, `process_response()`, `send_response_with_timestamps()` y `start_tag_reception_inmediate()` para usar nuevas constantes.

### 3.4 Compilar y Flashear
- Compilar proyecto Tag.
- Flashear en dispositivo.
- Verificar indicadores LED (WS2812) y batería (BQ25150).

---

## Paso 4: Testing y Validación
### 4.1 Pruebas Básicas
- **Conectividad**: Verificar que Sniffer detecte Tag a distancias cortas (5-10m).
- **Logs**: Revisar UART para errores de timeout o configuración.
- **Latencia**: Medir tiempo por ciclo (objetivo: ~200-600ms por tag).

### 4.2 Pruebas de Rango
- **Distancia**: Probar @ 20m, 30m, 40m en LOS y NLOS.
- **Modos**: Validar Multiple Detection (escaneo secuencial) y One Detection (medición simple).
- **Antenas**: Confirmar alternancia y triangulación (R:5 vs R:3).

### 4.3 Métricas de Éxito
- Detección >70% @ 30m en LOS.
- Latencia <1s por tag.
- Consumo aceptable (+50% estimado).

### 4.4 Rollback si Falla
- Revertir cambios en `uwb3000Fxx.h` a valores 6.8M.
- Recompilar y retestear.

---

## Paso 5: Documentación y Optimizaciones
- **Actualizar Docs**: Modificar `Resumen_Ejecutivo.md` con nuevos valores.
- **Optimizaciones**: Si latencia alta, ajustar HAL_Delay(1) o delays aleatorios.
- **Próximos Pasos**: Testing prolongado, 10+ tags simultáneos.

---

## Paso 6: Optimizaciones Avanzadas (Opcional)
- Aplicar solo si los cambios iniciales no alcanzan métricas de éxito (ej. detección <70% @30m, errores RXSTO/RXFTO recurrentes).
- **Parámetros a Modificar**:
  - **SFD Timeout**: Aumentar ×8 (ej. `SFD_TIMEOUT_850K = 520` µs) en `uwb3000Fxx.c` (registro `DRX_SFDTOC`).
  - **RX Frame Wait Timeout**: Aumentar ×8 (ej. `RX_FWTO_850K = 80000` µs) en `uwb3000Fxx.h` y referencias.
  - **Antenna Delay**: Recalibrar TX/RX delays en `uwb3000Fxx.c` (función de calibración).
  - **STS Configuration**: Habilitar STS (ej. `STS_LEN_850K = 128`) para inmunidad a interferencias.
  - **SNIFF Mode**: Ajustar `SNIFF_ON` y `SNIFF_OFF` para ahorro de energía.
  - **DGC y RX Tuning**: Retunear `DGC_CFG` si hay problemas de sensibilidad.
- **Subpasos**:
  - Actualizar constantes en `uwb3000Fxx.h` y `uwb3000Fxx.c`.
  - Modificar referencias en `sniffer_tag.cpp` y `human_tag.c`.
  - Compilar, flashear y retestear con métricas extendidas.
- **Tiempo Adicional**: +1 día desarrollo + testing.

---

## Notas Técnicas
- **Escalado**: Todos los timeouts ×8 por data rate (6.8M → 850K).
- **Compatibilidad**: Ambos dispositivos deben usar configuración idéntica.
- **Referencias**: Basado en DW3000 Manual (Appendix 1), código fuente y análisis previos.

**Responsable**: [Tu Nombre]  
**Fecha Inicio**: [Fecha]  
**Estado**: Planificado