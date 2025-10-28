# TODO - Sniffer-Tag Firmware

## Trabajo Completado (27-28 Oct 2025)

### 1. Migración y Organización del Proyecto
**Tiempo estimado: 2 horas**
- Migración inicial del proyecto sniffer-tag
- Actualización de .gitignore y configuración
- Organización de estructura de archivos
- Commits: `a7d3929`, `aa74315`

### 2. Formato de Logs Conciso
**Tiempo estimado: 2 horas**
- Implementación de formato conciso para errores >10
- Muestra primeros 3 + últimos 3 errores
- Reduce sobrecarga de logs en condiciones adversas
- Archivo: `sniffer-tag/Core/Src/sniffer_tag.cpp`
- Commit: `e8667ce`

### 3. GUI Tool para Sniffer-Tag
**Tiempo estimado: 4 horas**
- Creación de `sniffer_tag_gui.py` desde cero
- Interfaz completa con modos de detección
- Tabla de tags activos con estadísticas
- Log serial con colores y timestamps
- Guardado de logs con resumen
- Commit: `699a0c7`

### 4. Configuración de UWB Timeouts (6 parámetros iniciales)
**Tiempo estimado: 3 horas**
- Implementación inicial de 6 timeouts UWB
- Comandos LTEL 0x30-0x35
- UI con validación de rangos
- Documentación técnica inicial
- Commit: `97bcc95`

### 5. Documentación Técnica Completa
**Tiempo estimado: 2 horas**
- FORMATO_MEJORADO_LOG_ERRORES.md
- IMPLEMENTACION_UWB_TIMEOUT_CONFIG.md
- RESUMEN_MEJORAS_LOGS_Y_TIMEOUTS.md
- Análisis de impacto de timeouts
- Commit: `4f15bb3`

### 6. Logs de Ejemplo para Testing
**Tiempo estimado: 0.5 horas**
- Creación de ejemplos de logs
- Commit: `e727133`

### 7. Agregar Timeouts Faltantes (PRE_TIMEOUT + RNG_DELAY)
**Tiempo estimado: 3 horas**
- Análisis exhaustivo de documentación
- Búsqueda en 100+ matches de documentos
- Implementación de PRE_TIMEOUT (12 PAC)
- Implementación de RNG_DELAY (1000 ms)
- Reorganización UI en 4 secciones
- Actualización de comandos 0x30-0x37
- Commit: `b9be263`

### 8. Actualización de CHANGELOG y README
**Tiempo estimado: 1.5 horas**
- Creación de CHANGELOG.md completo
- Actualización de README con 8 parámetros UWB
- Sección de troubleshooting expandida
- Ejemplos de comandos con timeouts
- Commit: `ec3e1d3`

---

## Resumen de Tiempos

| Actividad | Horas |
|-----------|-------|
| Migración y organización | 2.0 |
| Logs concisos | 2.0 |
| GUI Tool | 4.0 |
| UWB Timeouts (inicial) | 3.0 |
| Documentación técnica | 2.0 |
| Logs de ejemplo | 0.5 |
| Timeouts faltantes | 3.0 |
| CHANGELOG y README | 1.5 |
| **TOTAL** | **18.0 horas** |

---

## Pendientes para Implementación en Firmware

### Alta Prioridad

#### 1. Implementar Comandos 0x30-0x37 en Firmware
**Estimado: 4-6 horas**
- [ ] Agregar constexpr en `CommandMessage.hpp`:
  ```cpp
  static constexpr uint8_t SET_PRE_TIMEOUT = 0x30;
  static constexpr uint8_t SET_RESP_RX_TIMEOUT = 0x31;
  static constexpr uint8_t SET_FINAL_RX_TIMEOUT = 0x32;
  static constexpr uint8_t SET_POLL_TX_RESP_RX = 0x33;
  static constexpr uint8_t SET_RESP_TX_FINAL_RX = 0x34;
  static constexpr uint8_t SET_POLL_RX_RESP_TX = 0x35;
  static constexpr uint8_t SET_RESP_RX_FINAL_TX = 0x36;
  static constexpr uint8_t SET_RNG_DELAY = 0x37;
  ```

- [ ] Implementar handlers en `main.cpp` o `sniffer_tag.cpp`:
  ```cpp
  case CommandMessage::SET_PRE_TIMEOUT:
      pre_timeout_pac = extract_uint32(data);
      dwt_setpreambledetecttimeout(pre_timeout_pac);
      break;
  
  case CommandMessage::SET_RESP_RX_TIMEOUT:
      resp_rx_timeout_uus = extract_uint32(data);
      dwt_setrxtimeout(resp_rx_timeout_uus);
      break;
  
  // ... etc para los 8 comandos
  ```

- [ ] Agregar variables globales para configuración
- [ ] Validar rangos en firmware (seguridad)
- [ ] Guardar configuración en EEPROM/Flash (opcional)

#### 2. Testing en Hardware
**Estimado: 4-8 horas**
- [ ] **Test PRE_TIMEOUT a >20m**:
  - Validar mejora de 0% → 50% success rate
  - Comparar PRE_TIMEOUT=5 vs PRE_TIMEOUT=12
  - Documentar resultados reales

- [ ] **Test RNG_DELAY**:
  - Medir consumo de energía con diferentes valores
  - Balance frecuencia de medición vs consumo
  - Valores óptimos: 500ms, 1000ms, 2000ms

- [ ] **Test RX Timeouts**:
  - RESP_RX_TIMEOUT y FINAL_RX_TIMEOUT en diferentes rangos
  - Verificar estabilidad con señal débil
  - Ajustar valores por defecto según resultados

- [ ] **Test TX→RX y RX→TX Delays**:
  - Validar sincronización del protocolo DS-TWR
  - Verificar que no hay timeouts prematuros
  - Documentar valores óptimos por escenario

#### 3. Documentación de Resultados
**Estimado: 2-3 horas**
- [ ] Crear `RESULTADOS_TESTING_TIMEOUTS.md`
- [ ] Documentar valores óptimos por escenario:
  - Indoor corto alcance (<10m)
  - Indoor largo alcance (10-20m)
  - Outdoor largo alcance (>20m)
- [ ] Gráficas de success rate vs PRE_TIMEOUT
- [ ] Tabla de consumo vs RNG_DELAY

### Media Prioridad

#### 4. Integración con JIRA
**Estimado: 1-2 horas**
- [ ] Configurar archivo `.env` con credenciales
- [ ] Crear tareas en JIRA para trabajo completado
- [ ] Agregar worklogs (18h total)
- [ ] Marcar tareas completadas como "Done"
- [ ] Crear tareas pendientes (firmware implementation)

#### 5. Optimizaciones de GUI
**Estimado: 2-3 horas**
- [ ] Agregar botón "Load Defaults" para resetear timeouts
- [ ] Agregar perfiles predefinidos (Indoor/Outdoor/Long-range)
- [ ] Implementar "Save/Load Configuration" a archivo JSON
- [ ] Mejorar validación de rangos con tooltips explicativos

### Baja Prioridad

#### 6. Mejoras Adicionales
**Estimado: 4-6 horas**
- [ ] Agregar gráficas en tiempo real de distancias
- [ ] Implementar filtrado de tags por ID
- [ ] Export de datos a CSV para análisis
- [ ] Agregar modo "Expert" con configuración avanzada
- [ ] Implementar logging de configuraciones aplicadas

---

## Notas Técnicas

### Valores Críticos Documentados

**PRE_TIMEOUT**:
- Default: 12 PAC (optimizado para >20m)
- Rango: 1-64 PAC
- Impacto: Detección >20m mejora de 0% a ~50% cuando se incrementa de 5→12

**RNG_DELAY**:
- Default: 1000 ms
- Rango: 100-10000 ms
- Impacto: Balance entre frecuencia de medición y consumo de energía

**RX Timeouts**:
- RESP_RX_TIMEOUT: Crítico para recepción de RESPONSE en DS-TWR
- FINAL_RX_TIMEOUT: Crítico para recepción de FINAL en DS-TWR

### Referencias
- Documentación: `documentos/RELACION_TIMEOUTS_UWB.md`
- Análisis: `documentos/ANALISIS_IMPACTO_TIEMPOS_UWB.md`
- Firmware: `Persona/Core/Inc/uwb3000Fxx.h` (líneas 690-750)
- GUI: `tools/sniffer_tag_gui.py`

---

## Branch Information

**Branch actual**: `feature/logs-concise-and-uwb-timeout-config`
**Commits**: 9 commits organizados
**Estado**: Listo para merge a `dev` después de testing

**Próximo paso**: Merge request y comenzar implementación en firmware
