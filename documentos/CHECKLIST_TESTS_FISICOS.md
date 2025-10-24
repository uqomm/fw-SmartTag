# 🧪 Checklist Tests Físicos - Corrección Detección >20m# 📋 TESTS SIMPLIFICADOS# 🧪 Checklist Tests Físicos - Corrección Detección >20m



**Fecha**: ___________ | **Responsable**: ___________ | **Versión FW**: ___________



---| Prueba | Validación | Procedimiento | Estado |**Fecha**: ___________ | **Responsable**: ___________ | **Versión FW**: ___________



## 📋 TESTS PRINCIPALES|--------|------------|---------------|--------|



| # | Prueba | Validación | Procedimiento | ✅ Estado || TEST-01: Detección básica | Verificar que el sistema detecta tags UWB a distancias cortas (1-5m) | 1. Encender sistema<br>2. Colocar tag a 1m<br>3. Verificar detección en logs<br>4. Repetir a 3m y 5m | ⏳ |---

|---|--------|------------|---------------|-----------|

| **TEST-01** | **Detección básica** | Sistema detecta tags UWB a distancias cortas (1-5m) | 1. Encender sistema<br>2. Colocar tag a 1m<br>3. Verificar detección en logs<br>4. Repetir a 3m y 5m | ⏳ || TEST-02: Timeout configuración | Confirmar que los timeouts están configurados correctamente para >20m | 1. Revisar configuración en uwb3000Fxx.h<br>2. Verificar valores: POLL_TX_TO_RESP_RX_DLY_UUS_6M8, RESP_RX_TIMEOUT_UUS_6M8, PRE_TIMEOUT_6M8<br>3. Validar compilación sin errores | ✅ |

| **TEST-02** | **Timeout configuración** | Timeouts configurados correctamente para >20m | 1. Revisar configuración en `uwb3000Fxx.h`<br>2. Verificar valores: `POLL_TX_TO_RESP_RX_DLY_UUS_6M8`, `RESP_RX_TIMEOUT_UUS_6M8`, `PRE_TIMEOUT_6M8`<br>3. Validar compilación sin errores | ✅ |

| **TEST-03** | **Detección distancia media** | Detección a 10-15m con obstáculos leves | 1. Configurar entorno de prueba<br>2. Colocar tag a 10m<br>3. Verificar detección consistente<br>4. Repetir con obstáculos (paredes delgadas) | ⏳ || TEST-03: Detección a distancia media | Probar detección a 10-15m con obstáculos leves | 1. Configurar entorno de prueba<br>2. Colocar tag a 10m<br>3. Verificar detección consistente<br>4. Repetir con obstáculos (paredes delgadas) | ⏳ |## ✅ PREPARACIÓN RÁPIDA

| **TEST-04** | **Detección distancia máxima** | Validación >20m en línea de vista | 1. Usar área abierta<br>2. Colocar tag a 25m<br>3. Verificar detección con nuevos timeouts<br>4. Medir tiempo de respuesta | ⏳ |

| **TEST-05** | **Estabilidad temporal** | Detección consistente durante periodos largos | 1. Ejecutar prueba continua por 30 min<br>2. Monitorear logs por pérdidas<br>3. Verificar estabilidad de conexión | ⏳ || TEST-04: Detección a distancia máxima | Validar detección >20m en línea de vista | 1. Usar área abierta<br>2. Colocar tag a 25m<br>3. Verificar detección con nuevos timeouts<br>4. Medir tiempo de respuesta | ⏳ |

| **TEST-06** | **Manejo de interferencias** | Sistema robusto con ruido electromagnético | 1. Introducir interferencias controladas<br>2. Verificar robustez de detección<br>3. Medir tasa de falsos positivos | ⏳ |

| TEST-05: Estabilidad temporal | Confirmar detección consistente durante periodos largos | 1. Ejecutar prueba continua por 30 min<br>2. Monitorear logs por pérdidas<br>3. Verificar estabilidad de conexión | ⏳ |- [ ] **Hardware**: Sniffer + Tag cargados, antenas separadas 2m, cable UART, cinta métrica 30m

---

| TEST-06: Manejo de interferencias | Probar sistema con ruido electromagnético | 1. Introducir interferencias controladas<br>2. Verificar robustez de detección<br>3. Medir tasa de falsos positivos | ⏳ |- [ ] **Software**: STM32CubeIDE, terminal serial, branch `fix/detection-over-20m`

## 🔧 TESTS DE IMPLEMENTACIÓN

- [ ] **Marcadores**: 10m, 15m, 20m, 25m, 30m

| # | Prueba | Validación | Procedimiento | ✅ Validado |

|---|--------|------------|---------------|-------------|## Notas de Implementación

| **IMP-01** | **Timeouts DW3000 aumentados** | Detección funciona >20m | 1. Modificar `uwb3000Fxx.h` → 700, 600, 8 (ambos equipos)<br>2. Compilar y flashear<br>3. Test 10-30m: ≥70% éxito a 25m | [ ] SÍ [ ] NO |

| **IMP-02** | **Validación de guardado** | No guarda datos incompletos | 1. Agregar validación en `main.cpp` líneas 542-546<br>2. Flashear sniffer<br>3. Test 3 escenarios: 15m, 28m, obstrucción | [ ] SÍ [ ] NO |- Los timeouts han sido ajustados para optimizar detección >20m---

| **IMP-03** | **Query timeout aumentado** | Más tiempo para queries lentas | 1. Cambiar `query_timeout` 1000→2000 en `main.cpp`<br>2. Flashear sniffer<br>3. Comparar 25m: mejora ≥20% | [ ] SÍ [ ] NO |

| **IMP-04** | **Sistema de logging** | Diagnóstico de fallos | 1. Agregar `log_rx_result()` en `sniffer_tag.cpp`<br>2. Flashear sniffer<br>3. Capturar logs UART durante tests | [ ] SÍ [ ] NO |- Se requiere validación física de todos los tests antes del release v0.4.0-fix-distance

| **IMP-05** | **Modo MULTIPLE obligatorio** | Siempre ambas antenas | 1. Eliminar `TAG_ONE_DETECTION` en `main.cpp`<br>2. Flashear sniffer<br>3. Test 15m: 100% ambas antenas | [ ] SÍ [ ] NO |

| **IMP-06** | **Timeouts agresivos** | Último recurso si IMP-01 falla | 1. Modificar → 1400, 700, 12 (ambos)<br>2. Repetir IMP-01<br>3. ≥70% a 25m | [ ] SÍ [ ] NO |- Documentar cualquier anomalía encontrada durante las pruebas## � TESTS SIMPLIFICADOS



---| # | Prueba | Valida | Cómo hacerlo | ✅ Validado |

|---|--------|--------|--------------|-------------|

## ✅ PREPARACIÓN RÁPIDA| **TEST-01** | **Timeouts DW3000 aumentados** | Detección funciona >20m | 1. Modificar `uwb3000Fxx.h` → 1000, 500, 8 (ambos equipos)<br>2. Compilar y flashear<br>3. Test 10-30m: ≥70% éxito a 25m | [ ] SÍ [ ] NO |

| **TEST-02** | **Validación de guardado** | No guarda datos incompletos | 1. Agregar validación en `main.cpp` líneas 542-546<br>2. Flashear sniffer<br>3. Test 3 escenarios: 15m, 28m, obstrucción | [ ] SÍ [ ] NO |

### Hardware| **TEST-03** | **Query timeout aumentado** | Más tiempo para queries lentas | 1. Cambiar `query_timeout` 1000→2000 en `main.cpp`<br>2. Flashear sniffer<br>3. Comparar 25m: mejora ≥20% | [ ] SÍ [ ] NO |

- [ ] **Sniffer + Tag**: Cargados y funcionales| **TEST-05** | **Sistema de logging** | Diagnóstico de fallos | 1. Agregar `log_rx_result()` en `sniffer_tag.cpp`<br>2. Flashear sniffer<br>3. Capturar logs UART durante tests | [ ] SÍ [ ] NO |

- [ ] **Antenas**: Separadas 2m mínimo| **TEST-06** | **Modo MULTIPLE obligatorio** | Siempre ambas antenas | 1. Eliminar `TAG_ONE_DETECTION` en `main.cpp`<br>2. Flashear sniffer<br>3. Test 15m: 100% ambas antenas | [ ] SÍ [ ] NO |

- [ ] **Cable UART**: Conectado para logs| **TEST-01B** | **Timeouts agresivos** | Último recurso si TEST-01 falla | 1. Modificar → 1400, 700, 12 (ambos)<br>2. Repetir TEST-01<br>3. ≥70% a 25m | [ ] SÍ [ ] NO |

- [ ] **Cinta métrica**: 30m para mediciones precisas

- [ ] **Marcadores**: 10m, 15m, 20m, 25m, 30m---



### Software## 📊 RESULTADO FINAL

- [ ] **STM32CubeIDE**: Instalado y configurado

- [ ] **Terminal serial**: Para monitoreo de logs### ✅ **Éxito Global**

- [ ] **Branch**: `dev` con últimos cambios- [ ] **OBJETIVO ALCANZADO**: Detección estable ≥70% a 25m

- [ ] **Compilación**: Sin errores en ambos proyectos- [ ] **PARCIAL**: Funciona pero <70%

- [ ] **FALLIDO**: Sin mejora significativa

---

### 🔧 **Configuración Final**

## 📊 RESULTADO FINAL```c

// Valores finales implementados

### ✅ **Éxito Global**POLL_TX_TO_RESP_RX_DLY_UUS_6M8 = _____  // Ambos equipos

- [ ] **OBJETIVO ALCANZADO**: Detección estable ≥70% a 25mRESP_RX_TIMEOUT_UUS_6M8 = _____         // Ambos equipos

- [ ] **PARCIAL**: Funciona pero <70%PRE_TIMEOUT_6M8 = _____                 // Ambos equipos

- [ ] **FALLIDO**: Sin mejora significativaquery_timeout = _____ ms               // Sniffer

```

### 🔧 **Configuración Final Implementada**

```c### 📝 **Resumen de Cambios**

// Valores finales en uwb3000Fxx.h (ambos equipos)- [ ] TEST-01 implementado

#define POLL_TX_TO_RESP_RX_DLY_UUS_6M8 700- [ ] TEST-02 implementado

#define RESP_RX_TIMEOUT_UUS_6M8        600  // Aumentado +100%- [ ] TEST-03 implementado

#define PRE_TIMEOUT_6M8                8    // Aumentado +60%- [ ] TEST-05 implementado

- [ ] TEST-06 implementado

// Valores en main.cpp (sniffer)- [ ] TEST-01B (solo si necesario)

#define QUERY_TIMEOUT_MS               2000 // Aumentado +100%

```---



### 📝 **Resumen de Cambios Implementados**## 📋 CHECKLIST DE EJECUCIÓN

- [x] IMP-01: Timeouts DW3000 aumentados

- [ ] IMP-02: Validación de guardado### Antes de Tests

- [ ] IMP-03: Query timeout aumentado- [ ] Preparación hardware/software completa

- [ ] IMP-04: Sistema de logging- [ ] Commit baseline realizado

- [ ] IMP-05: Modo MULTIPLE obligatorio- [ ] Logs UART configurados

- [ ] IMP-06: Timeouts agresivos (solo si necesario)

### Durante Tests

---- [ ] 3 minutos por distancia en TEST-01

- [ ] 20-30 ciclos por escenario

## 📋 CHECKLIST DE EJECUCIÓN- [ ] Logs guardados por sesión

- [ ] Fotos/videos de setup

### Antes de Tests

- [ ] Preparación hardware/software completa### Después de Tests

- [ ] Commit baseline realizado (`feat: baseline-pre-tests-fisicos`)- [ ] Commit final con cambios

- [ ] Logs UART configurados y probados- [ ] Tag `v0.4.0-fix-distance`

- [ ] CHANGELOG.md actualizado

### Durante Tests- [ ] Merge a `dev`

- [ ] 3 minutos por distancia en TEST-01/03/04

- [ ] 20-30 ciclos por escenario---

- [ ] Logs guardados por sesión

- [ ] Fotos/videos del setup de pruebas## 📝 NOTAS RÁPIDAS



### Después de Tests**Problemas encontrados**:

- [ ] Commit final con cambios (`feat: tests-fisicos-completados`)```

- [ ] Tag `v0.4.0-fix-distance` creado

- [ ] CHANGELOG.md actualizado```

- [ ] Merge a rama `main` vía PR

**Lecciones aprendidas**:

---```



## 📝 NOTAS Y OBSERVACIONES```



### Problemas Encontrados**Recomendaciones**:

``````



``````



### Lecciones Aprendidas---

```

**Completado el**: ___________ por: ___________

```

### Recomendaciones Futuras
```

```

---

## 📋 INFORMACIÓN DEL PROYECTO

- **Proyecto**: Sniffer Tag UWB Detection System
- **Objetivo**: Mejorar detección >20m con timeouts optimizados
- **Branch**: `dev`
- **JIRA**: FWSL-6 (Validación física de correcciones UWB >20m)
- **Completado el**: ___________ por: ___________