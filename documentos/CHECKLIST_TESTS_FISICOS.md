# 🧪 Checklist Tests Físicos - Corrección Detección >20m

**Fecha**: ___________ | **Responsable**: ___________ | **Versión FW**: ___________

---

## ✅ PREPARACIÓN RÁPIDA

- [ ] **Hardware**: Sniffer + Tag cargados, antenas separadas 2m, cable UART, cinta métrica 30m
- [ ] **Software**: STM32CubeIDE, terminal serial, branch `fix/detection-over-20m`
- [ ] **Marcadores**: 10m, 15m, 20m, 25m, 30m

---

## 📜 HISTORIAL DE INVESTIGACIÓN (Orden Cronológico)

### **TEST-00: PRE_TIMEOUT=5 (Baseline)** [✅ COMPLETADO - 25-Oct-2025]

**Configuración**: `PRE_TIMEOUT_6M8 = 5 PAC` (40 símbolos, baseline de fábrica)

**Resultado @ 23m**:
```
Canal A: 0% éxito (152 timeouts RX_PREAMBLE_DETECTION_TIMEOUT)
Canal B: 100% éxito (DistB: 22.88-23.42m, 0 errores)
```

**❌ Conclusión**: Configuración baseline **INSUFICIENTE** para detección >20m en Canal A. Canal B funciona perfectamente.

---

### **TEST-01: PRE_TIMEOUT=8 (Solución 1A)** [✅ COMPLETADO - 26-Oct-2025]

**Configuración**: `PRE_TIMEOUT_6M8 = 8 PAC` (64 símbolos, +60% vs baseline)

**Resultado @ 20m**:
```
Canal A: 3% éxito (96 timeouts, 3 lecturas: 19.77-19.89m)
Canal B: 9.7% éxito (28 timeouts, 3 lecturas: 19.93-20.18m)
```

**⚠️ Análisis**:
- ✅ Canal A: 0% → 3% mejora (progreso marginal desde baseline)
- ⚠️ Canal B: 100% @ 23m → 9.7% @ 20m (degradación inesperada, posible cambio de entorno)
- ⚠️ Ambos canales muestran baja tasa de éxito comparado con TEST-00 @ 23m

**⚠️ Conclusión**: PRE_TIMEOUT=8 **NO es suficiente**. Se requiere +50% adicional (PRE_TIMEOUT=12).

---

### **TEST-02: Swap Físico de Antenas** [✅ COMPLETADO - 28-Oct-2025]

**Objetivo**: Determinar si problema es hardware físico (antena/cable) o configuración lógica (Canal A del firmware).

**Evidencia**:
- **Configuración Original**: Canal A → Antena Física 1, Canal B → Antena Física 2
  - Canal A @ 21.7m: 0% éxito (100% timeout)
  - Canal B @ 21.7m: 100% éxito (51/51 detecciones)
  
- **Configuración Swap**: Canal A → Antena Física 2, Canal B → Antena Física 1
  - Canal A @ 21.7m: 0% éxito (100% timeout) ← **Problema sigue en Canal A**
  - Canal B @ 21.7m: 100% éxito ← **Funciona con cualquier antena**

**✅ Conclusión**: Problema es **SOFTWARE/CONFIGURACIÓN del Canal A lógico**, NO hardware de antenas.

---

### **TEST-03: Separar Estructuras `pdw3000local`** [✅ COMPLETADO - 28-Oct-2025]

**Hipótesis**: Ambos chips DW3000 (Canal A y B) comparten una sola estructura `dwt_local_data_t`, causando que Canal B sobrescriba calibración OTP de Canal A.

**Problema identificado**:
```c
// main.cpp línea 65 y 1117
dwt_local_data_t *pdw3000local;  // ← UNA SOLA estructura para AMBOS chips
```

**Solución implementada**:
- Creadas estructuras separadas: `pdw3000local_a`, `pdw3000local_b`, `pdw3000local` (puntero activo)
- Modificado `init_uwb_device()` para recibir `dwt_local_data_t *local_data`
- Actualizado `switch_hw()` y `switch_hw_timestamp_query()` para cambiar puntero
- Archivos modificados: `main.cpp`, `sniffer_tag.cpp`, `sniffer_tag.hpp`

**Resultado TEST @ 21m**:
```
Canal A: 0% éxito (50/50 RX_PREAMBLE_DETECTION_TIMEOUT)
Canal B: 100% éxito (DistB: 21.16-21.81m, 0 errores)
```

**Verificación OTP**:
```
CANAL A: PartID:0x624831E5  Bias:0x13  DGC_MODE:OTP ✅
CANAL B: PartID:0x6E483065  Bias:0x12  DGC_MODE:OTP ✅
```

**❌ Conclusión**: Hipótesis **INCORRECTA**. Estructuras separadas funcionan correctamente (PartID diferentes, calibraciones únicas), pero Canal A sigue fallando. Problema NO era calibración OTP compartida.

---

### **TEST-04: PRE_TIMEOUT=12 (Solución 1B)** [✅ COMPLETADO - 28-Oct-2025]

**Configuración**: `PRE_TIMEOUT_6M8 = 12 PAC` (96 símbolos, +140% vs baseline 5, +50% vs Solución 1A)

**Resultado @ 23m**:
```
Canal A: 0% éxito (152 timeouts RX_PREAMBLE_DETECTION_TIMEOUT)
Canal B: 100% éxito (DistB: 22.88-23.42m, 0 errores)
```

**❌ Análisis**:
- ❌ Canal A: **SIN MEJORA** - Permanece en 0% a pesar de +140% incremento desde baseline
- ✅ Canal B: Funciona perfectamente (100% éxito)
- ❌ PRE_TIMEOUT=12 **NO resuelve el problema** a pesar de alcanzar 75% del preámbulo total (96/128 símbolos)
- 🔴 **Optimización de PRE_TIMEOUT ha alcanzado su límite de utilidad**

**❌ Conclusión**: Canal A tiene problema **FUNDAMENTAL** que no se resuelve con optimizaciones de timeout. **Requiere solución más radical** (TEST-07: 850K data rate con +8dB sensibilidad) o reemplazo de hardware.

**⚠️ Nota crítica**: Resultado TEST-04 es **IDÉNTICO** a TEST-00 (baseline), lo que confirma que incrementar PRE_TIMEOUT de 5→8→12 **NO tiene efecto alguno** en Canal A. Esto es evidencia fuerte de problema hardware (LNA degradado, filtro RF, o chip defectuoso).

---

### **🔍 Conclusión del Diagnóstico**

Dado que:
1. ✅ **TEST-00**: PRE_TIMEOUT=5 @ 23m → Canal A: 0%, Canal B: 100%
2. ✅ **TEST-01**: PRE_TIMEOUT=8 @ 20m → Canal A: 3%, Canal B: 9.7%
3. ✅ **TEST-02**: Swap antenas → Problema sigue al Canal A lógico (no antena física)
4. ✅ **TEST-03**: Estructuras OTP separadas → Calibraciones válidas en ambos chips, pero Canal A sigue fallando
5. ✅ **TEST-04**: PRE_TIMEOUT=12 @ 23m → Canal A: 0% (IDÉNTICO a baseline), Canal B: 100%
6. ❌ **Incrementar PRE_TIMEOUT de 5→8→12 (+140%) NO tiene efecto en Canal A**

**Hipótesis actual**: **Problema HARDWARE CONFIRMADO en componentes del Canal A**

**Posibles causas técnicas**:
- **LNA degradado** (más probable): Baja sensibilidad (-10 dB típico)
- **Filtro RF desafinado**: Atenúa señal en Canal 5 (6.5 GHz)
- **PCB routing defectuoso**: Impedancia incorrecta en RF de Chip A
- **Cristal/PLL desviado**: Frecuencia ligeramente fuera de spec
- **Chip DW3000 A defectuoso**: Fallo de fábrica

**Test recomendado**: Medir potencia RX con analizador de espectro @ -90 dBm, comparar Canal A vs B (esperado: Canal A ~10 dB menos sensibilidad).

**Opciones restantes**:
1. **TEST-05 (TX Power +1dB)**: Última optimización SW incremental (mejora esperada: <10%)
2. **TEST-07 (850K data rate)**: Solución radical SW (+8dB sensibilidad, puede compensar hardware defectuoso)
3. **Reemplazo de hardware**: Cambiar PCB o chip DW3000 A si TEST-07 falla

---

## 🧪 TESTS PENDIENTES (Optimizaciones Software)

### **TEST-05: Aumentar TX Power** [� PRIORIDAD ALTA]

**Estado**: ⏳ **CONSIDERAR AHORA** - Última optimización SW incremental antes de solución radical (TEST-07)

**Configuración propuesta**:
```c
// En sniffer_tag.cpp y main.cpp línea ~56
static dwt_txconfig_t dwt_tx_cfg = { 
    0x34,       /* PG delay */
    0xFEFEFEFE, /* TX power: +1 dB (Coarse gain=2, Fine gain=63) */
    0x0         /* PG count */
};
```

**¿Por qué ayuda?**: Mayor potencia transmitida (+1 dB) = señal más fuerte en recepción a largas distancias.

**⚠️ ADVERTENCIA**: Aumentar power puede violar límites regulatorios (-41.3 dBm/MHz). Idealmente medir con analizador de espectro antes.

**Procedimiento**:
1. Modificar `TX_POWER` de 0xFDFDFDFD → 0xFEFEFEFE (+1 dB conservador)
2. Compilar y flashear ambos dispositivos
3. Test @ 20m con protocolo similar a TEST-02/03/04
4. Comparar tasas de éxito vs PRE_TIMEOUT=12

**Criterios de éxito**: +5-10% mejora en Canal A. Si Canal A alcanza ≥40% @ 20m → Considerar éxito parcial, pero **probablemente insuficiente** para operación confiable.

**⚠️ Evaluación realista**: Dado que TEST-04 (PRE_TIMEOUT=12) falló, +1dB TX power probablemente NO será suficiente. Considerar ir directamente a TEST-07 (850K).

---

### **TEST-06: RX Diagnostics (First Path Power)** [🟢 PRIORIDAD MEDIA]

**Estado**: ⏸️ **OPCIONAL** - Solo para confirmar diagnóstico de hardware antes de TEST-07

**Objetivo**: Determinar si problema es en TX o RX del Canal A mediante diagnósticos internos del DW3000.

**Implementación**:
```c
// Agregar en sniffer_tag.cpp después de dwt_rxenable()
dwt_rxdiag_t diag;
dwt_readdiagnostics(&diag);
Serial.printf("[DIAG-A] FP_PWR:%d RX_CNT:%d\n", diag.firstPath, diag.rxPreamCount);
```

**Procedimiento**:
1. Agregar logs de diagnóstico para CADA intento de RX
2. Test @ 20m capturando First Path Power y RX Preamble Count
3. Comparar valores Canal A vs Canal B

**Análisis esperado**:
- **FP_PWR Canal A < -100 dBm** → Confirma problema RX sensitivity (LNA defectuoso)
- **FP_PWR Canal A ≈ Canal B** pero RX_CNT bajo → Problema AGC/threshold
- **FP_PWR Canal A muy bajo + Canal B normal** → Confirma hardware defectuoso

---

### **TEST-07: Migración a Data Rate 850K** [� PRIORIDAD CRÍTICA]

**Estado**: ⏳ **SOLUCIÓN RECOMENDADA** - Después de fallar TEST-02, TEST-03, TEST-04, esta es la **única opción SW viable** sin reemplazo de hardware

**¿Por qué considerar esto?**:
- 📡 **+8 dB sensibilidad** vs 6M8 → Puede compensar hardware defectuoso de Canal A
- 🎯 **Rango 50-80m** (2.5-3× mejora) vs 20-25m actual
- 🔧 **Solución definitiva** sin reemplazo de hardware

**⚠️ Trade-offs**:
- ⏱️ **8× más lento**: 2 ms por frame vs 250 µs (latencia 800ms vs 100ms por tag)
- ⚡ **+50% consumo energético** en tags
- 📊 **Menor throughput**: 10 tags/seg vs 80 tags/seg (aceptable para 5-10 tags totales)

**Configuración propuesta**:
```c
static dwt_config_t dwt_cfg_850k = {
    5,          // Channel
    DWT_PLEN_128,
    DWT_PAC8,
    9,
    DWT_BR_850K,  // ← Cambio crítico de 6M8 a 850K
    // ... resto igual
};

// Timeouts ajustados (8× más largos):
#define POLL_TX_TO_RESP_RX_DLY_UUS_850K 5600  // Was 700
#define RESP_RX_TIMEOUT_UUS_850K 2400         // Was 300
#define PRE_TIMEOUT_850K 12                   // Mantener optimización
```

**Esfuerzo**: 1-2 días (cambiar config DWT, ajustar timeouts en ambos firmwares, testing).

**Criterios de decisión**:
- ✅ Si Canal A ≥70% @ 50m → **APLICAR** como solución definitiva
- ⚠️ Si latencia >1 seg/tag es inaceptable → Evaluar modo híbrido (6M8 corto, 850K largo)
- ❌ Si Canal A <30% @ 20m incluso con 850K → **Hardware reemplazo obligatorio**

---

### **TEST-08: Cambio de Canal UWB** [🔵 PRIORIDAD BAJA]

---

### **TEST-01: Separar Estructuras `pdw3000local`** [✅ COMPLETADO - 28-Oct-2025]

**Hipótesis**: Ambos chips DW3000 (Canal A y B) comparten una sola estructura `dwt_local_data_t`, causando que Canal B sobrescriba calibración OTP de Canal A.

**Problema identificado**:
```c
// main.cpp línea 65 y 1117
dwt_local_data_t *pdw3000local;  // ← UNA SOLA estructura para AMBOS chips
```

**Solución implementada**:
- Creadas estructuras separadas: `pdw3000local_a`, `pdw3000local_b`, `pdw3000local` (puntero activo)
- Modificado `init_uwb_device()` para recibir `dwt_local_data_t *local_data`
- Actualizado `switch_hw()` y `switch_hw_timestamp_query()` para cambiar puntero
- Archivos modificados: `main.cpp`, `sniffer_tag.cpp`, `sniffer_tag.hpp`

**Resultado TEST @ 21m**:
```
Canal A: 0% éxito (50/50 RX_PREAMBLE_DETECTION_TIMEOUT)
Canal B: 100% éxito (DistB: 21.16-21.81m, 0 errores)
```

**Verificación OTP**:
```
CANAL A: PartID:0x624831E5  Bias:0x13  DGC_MODE:OTP ✅
CANAL B: PartID:0x6E483065  Bias:0x12  DGC_MODE:OTP ✅
```

**❌ Conclusión**: Hipótesis **INCORRECTA**. Estructuras separadas funcionan correctamente (PartID diferentes, calibraciones únicas), pero Canal A sigue fallando. Problema NO era calibración OTP compartida.

---

### **🔍 Conclusión del Diagnóstico**

Dado que:
1. ✅ TEST-00: Problema sigue al Canal A lógico (no antena física)
2. ✅ TEST-01: Estructuras OTP separadas y calibraciones válidas en ambos chips
3. ❌ Canal A sigue fallando a >20m a pesar de configuración correcta
4. ✅ Canal B funciona perfectamente con cualquier antena

**Hipótesis actual**: **Problema HARDWARE en componentes del Canal A**

**Posibles causas técnicas**:
- **LNA degradado**: Baja sensibilidad (-10 dB típico)
- **Filtro RF desafinado**: Atenúa señal en Canal 5 (6.5 GHz)
- **PCB routing defectuoso**: Impedancia incorrecta en RF de Chip A
- **Cristal/PLL desviado**: Frecuencia ligeramente fuera de spec
- **Chip DW3000 A defectuoso**: Fallo de fábrica

**Test recomendado**: Medir potencia RX con analizador de espectro @ -90 dBm, comparar Canal A vs B (esperado: Canal A ~10 dB menos sensibilidad).

---

## 🧪 TESTS PENDIENTES (Optimizaciones Software)

### **TEST-02: PRE_TIMEOUT=12 @ 20-25m** [🔴 PRIORIDAD CRÍTICA]

**Estado**: ⏳ **CÓDIGO YA IMPLEMENTADO** - Pendiente validación en campo

**Configuración actual**:
```c
// uwb3000Fxx.h (Sniffer y Persona)
PRE_TIMEOUT_6M8 = 12 PAC  // +140% desde baseline (5 PAC)
```

**¿Por qué ayuda?**: Espera más tiempo (96 símbolos) para detectar inicio de preámbulo en señales débiles >20m.

**Resultados históricos**:
- PRE_TIMEOUT=5 @ 23m → Canal A: 0%, Canal B: 100%
- PRE_TIMEOUT=8 @ 20m → Canal A: 3%, Canal B: 9.7%
- PRE_TIMEOUT=12 @ ??? → **PENDIENTE VALIDAR**

**Procedimiento**:
1. Firmware actual ya tiene PRE_TIMEOUT=12 (commit actual)
2. Test @ 20m, 23m, 25m con ≥100 lecturas por distancia
3. Capturar tasas de éxito y logs de errores

**Criterios de éxito**:
- ✅ **Objetivo mínimo**: Canal A ≥30%, Canal B ≥50% @ 20m
- ✅ **Objetivo aceptable**: Ambos ≥50% @ 20m
- ✅ **Ideal**: Ambos ≥70% @ 25m

**Siguiente paso si falla**: Si Canal A <30% @ 20m → Considerar TEST-05 (850K data rate) como única solución viable.

---

### **TEST-03: Aumentar TX Power** [🟡 PRIORIDAD ALTA]

**Estado**: ⏸️ **BLOQUEADO** - Solo ejecutar si TEST-02 muestra Canal A <50% @ 20m

**Configuración propuesta**:
```c
// En sniffer_tag.cpp y main.cpp línea ~56
static dwt_txconfig_t dwt_tx_cfg = { 
    0x34,       /* PG delay */
    0xFEFEFEFE, /* TX power: +1 dB (Coarse gain=2, Fine gain=63) */
    0x0         /* PG count */
};
```

**¿Por qué ayuda?**: Mayor potencia transmitida (+1 dB) = señal más fuerte en recepción a largas distancias.

**⚠️ ADVERTENCIA**: Aumentar power puede violar límites regulatorios (-41.3 dBm/MHz). Idealmente medir con analizador de espectro antes.

**Procedimiento**:
1. Modificar `TX_POWER` de 0xFDFDFDFD → 0xFEFEFEFE (+1 dB conservador)
2. Compilar y flashear ambos dispositivos
3. Re-ejecutar TEST-02 @ 20m
4. Comparar tasas de éxito vs baseline

**Criterios de éxito**: +5-10% mejora en Canal A. Si Canal A alcanza ≥60% @ 20m → Aplicar cambio permanentemente.

---

### **TEST-04: RX Diagnostics (First Path Power)** [🟢 PRIORIDAD MEDIA]

**Estado**: ⏸️ **OPCIONAL** - Solo si TEST-02 y TEST-03 fallan

**Objetivo**: Determinar si problema es en TX o RX del Canal A mediante diagnósticos internos del DW3000.

**Implementación**:
```c
// Agregar en sniffer_tag.cpp después de dwt_rxenable()
dwt_rxdiag_t diag;
dwt_readdiagnostics(&diag);
Serial.printf("[DIAG-A] FP_PWR:%d RX_CNT:%d\n", diag.firstPath, diag.rxPreamCount);
```

**Procedimiento**:
1. Agregar logs de diagnóstico para CADA intento de RX
2. Test @ 20m capturando First Path Power y RX Preamble Count
3. Comparar valores Canal A vs Canal B

**Análisis esperado**:
- **FP_PWR Canal A < -100 dBm** → Confirma problema RX sensitivity (LNA defectuoso)
- **FP_PWR Canal A ≈ Canal B** pero RX_CNT bajo → Problema AGC/threshold
- **FP_PWR Canal A muy bajo + Canal B normal** → Confirma hardware defectuoso

---

### **TEST-05: Migración a Data Rate 850K** [� PRIORIDAD MEDIA-ALTA]

**Estado**: ⏸️ **SOLUCIÓN RADICAL** - Considerar si TEST-02 muestra Canal A <30% @ 20m

**¿Por qué considerar esto?**:
- 📡 **+8 dB sensibilidad** vs 6M8 → Puede compensar hardware defectuoso de Canal A
- 🎯 **Rango 50-80m** (2.5-3× mejora) vs 20-25m actual
- 🔧 **Solución definitiva** sin reemplazo de hardware

**⚠️ Trade-offs**:
- ⏱️ **8× más lento**: 2 ms por frame vs 250 µs (latencia 800ms vs 100ms por tag)
- ⚡ **+50% consumo energético** en tags
- 📊 **Menor throughput**: 10 tags/seg vs 80 tags/seg (aceptable para 5-10 tags totales)

**Configuración propuesta**:
```c
static dwt_config_t dwt_cfg_850k = {
    5,          // Channel
    DWT_PLEN_128,
    DWT_PAC8,
    9,
    DWT_BR_850K,  // ← Cambio crítico de 6M8 a 850K
    // ... resto igual
};

// Timeouts ajustados (8× más largos):
#define POLL_TX_TO_RESP_RX_DLY_UUS_850K 5600  // Was 700
#define RESP_RX_TIMEOUT_UUS_850K 2400         // Was 300
#define PRE_TIMEOUT_850K 12                   // Mantener optimización
```

**Esfuerzo**: 1-2 días (cambiar config DWT, ajustar timeouts en ambos firmwares, testing).

**Criterios de decisión**:
- ✅ Si Canal A ≥70% @ 50m → **APLICAR** como solución definitiva
- ⚠️ Si latencia >1 seg/tag es inaceptable → Evaluar modo híbrido (6M8 corto, 850K largo)
- ❌ Si Canal A <30% @ 20m incluso con 850K → **Hardware reemplazo obligatorio**

---

### **TEST-06: Cambio de Canal UWB** [🔵 PRIORIDAD BAJA]

**Estado**: ⏸️ **ÚLTIMA OPCIÓN SW** - Solo si hay sospecha de interferencia en Canal 5

**Configuración propuesta**:
```c
// Cambiar de Canal 5 (6.5 GHz) a Canal 9 (8.0 GHz)
static dwt_config_t dwt_cfg = {
    9,          // Channel number (antes: 5)
    DWT_PLEN_128,
    DWT_PAC8,
    9,          // Preamble code compatible con Canal 9
    // ... resto sin cambios
};
```

**⚠️ Consideración**: Canal 9 tiene ~0.5 dB menos sensibilidad que Canal 5. Solo cambiar si interferencia confirmada con spectrum analyzer.

**Procedimiento**:
1. Capturar log RF de Canal 5 (si analizador disponible)
2. Si detecta interferencias > -90 dBm → Cambiar a Canal 9
3. Re-ejecutar TEST-02 @ 20m con Canal 9
4. Comparar tasas de éxito

**Resultado esperado**: Si interferencia era causa, mejora ≥20% en ambos canales

---

## � RESUMEN EJECUTIVO DE TESTS

### ✅ **Diagnóstico Completado (Orden Cronológico)**

| Test | Fecha | Configuración | Resultado @ Distancia |
|------|-------|---------------|----------------------|
| **TEST-00** | 25-Oct | PRE_TIMEOUT=5 (baseline) | Canal A: 0% @ 23m, Canal B: 100% @ 23m |
| **TEST-01** | 26-Oct | PRE_TIMEOUT=8 (+60%) | Canal A: 3% @ 20m, Canal B: 9.7% @ 20m |
| **TEST-02** | 28-Oct | Swap físico antenas | Problema sigue en Canal A lógico ✅ |
| **TEST-03** | 28-Oct | Estructuras OTP separadas | Hipótesis incorrecta ❌ |
| **TEST-04** | 28-Oct | PRE_TIMEOUT=12 (+140%) | Canal A: 0% @ 23m, Canal B: 100% @ 23m ❌ |

**Conclusión**: 
- ❌ **Problema hardware CONFIRMADO** en Canal A
- ❌ Optimización PRE_TIMEOUT **AGOTADA** (5→8→12 no tiene efecto)
- ✅ TEST-04 idéntico a TEST-00 demuestra que timeouts NO son la causa raíz
- 🔴 **Solución SW radical (TEST-07: 850K) o reemplazo hardware son las únicas opciones**

---

### ⏳ **Tests Pendientes (Priorizados)**

| Test | Prioridad | Estado | Objetivo | Esfuerzo |
|------|-----------|--------|----------|----------|
| **TEST-05** | � **ALTA** | ⏳ Considerar | TX Power +1dB (mejora esperada <10%) | 2h (código + test) |
| **TEST-06** | 🟢 **MEDIA** | ⏸️ Opcional | RX Diagnostics para confirmar hardware | 3h (código + análisis) |
| **TEST-07** | � **CRÍTICA** | ⏳ **RECOMENDADO** | Migrar a 850K (+8dB sensibilidad) | 1-2 días |
| **TEST-08** | 🔵 **BAJA** | ⏸️ Última opción | Cambio Canal 5→9 (interferencia) | 1h (código + test) |

---

## 🎯 PLAN DE ACCIÓN INMEDIATO

### **Opción A: TEST-05 (conservadora, baja probabilidad de éxito)**
⚠️ TX Power +1dB @ 20m  
📊 Mejora esperada: +5-10% en Canal A  
⏱️ Si Canal A sigue <30% → Ir directamente a TEST-07

### **Opción B: TEST-07 (recomendada, mayor probabilidad de éxito)**
✅ Migrar a 850K data rate  
📡 +8 dB sensibilidad → **Puede compensar hardware defectuoso**  
🎯 Rango objetivo: 50-80m (vs 20-25m actual)  
⏱️ Esfuerzo: 1-2 días de implementación

**Si TEST-07 éxito (Canal A ≥70% @ 50m)**:
- ✅ Problema RESUELTO sin reemplazo de hardware
- 📝 Documentar configuración final 850K
- 🔀 Merge a branch `dev`
- ⚠️ Aceptar trade-offs: latencia 8× mayor (800ms vs 100ms)

**Si TEST-07 falla (Canal A <30% @ 20m incluso con 850K)**:
- ❌ **Hardware reemplazo OBLIGATORIO**
- 🔧 Opciones: Cambiar chip DW3000 A, reemplazar PCB completo, o validar LNA/filtro RF
- 💰 Costo estimado: $20-50 (chip) o $200-500 (PCB completo)
  
---

---

## 📊 RESULTADO FINAL

### ✅ **Éxito Global**
- [ ] **OBJETIVO ALCANZADO**: Detección estable ≥70% a 25m
- [ ] **PARCIAL**: Funciona pero <70%
- [ ] **FALLIDO**: Sin mejora significativa

### 🔧 **Configuración Final**
```c
// Valores finales implementados (actualizar después de tests)
POLL_TX_TO_RESP_RX_DLY_UUS_6M8 = _____  // Ambos equipos
RESP_RX_TIMEOUT_UUS_6M8 = _____         // Ambos equipos
PRE_TIMEOUT_6M8 = _____                 // Ambos equipos (baseline: 12 PAC)
query_timeout = _____ ms                // Sniffer (baseline: 1000 ms)
TX_POWER = 0x_______                    // Si se modificó (baseline: 0xFDFDFDFD)
Channel = _____                         // Si se cambió (baseline: 5)
```

### 📝 **Resumen de Cambios**
- [ ] TEST-00 (Swap) ejecutado → Resultado: [ ] Hardware [ ] Config
- [ ] TEST-01 implementado (PRE_TIMEOUT)
- [ ] TEST-02 implementado (TX Power)
- [ ] TEST-03 implementado (Query timeout)
- [ ] TEST-04 implementado (Validación)
- [ ] TEST-05 implementado (Logging)
- [ ] TEST-06 implementado (MULTIPLE only)
- [ ] TEST-07 implementado (solo si necesario)
- [ ] TEST-08 implementado (solo si necesario)

---

## 📋 CHECKLIST DE EJECUCIÓN

### Antes de Tests
- [ ] Preparación hardware/software completa
- [ ] Commit baseline realizado
- [ ] Logs UART configurados

### Durante Tests
- [ ] 3 minutos por distancia en TEST-01
- [ ] 20-30 ciclos por escenario
- [ ] Logs guardados por sesión
- [ ] Fotos/videos de setup

### Después de Tests
- [ ] Commit final con cambios
- [ ] Tag `v0.4.0-fix-distance`
- [ ] CHANGELOG.md actualizado
- [ ] Merge a `dev`

---

## 📝 NOTAS RÁPIDAS

**Problemas encontrados**:
```

```

**Lecciones aprendidas**:
```

```

**Recomendaciones**:
```

```

---

**Completado el**: ___________ por: ___________
