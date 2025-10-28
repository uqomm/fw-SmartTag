# 🧪 Checklist Tests Físicos - Corrección Detección >20m

**Fecha**: ___________ | **Responsable**: ___________ | **Versión FW**: ___________

---

## ✅ PREPARACIÓN RÁPIDA

- [ ] **Hardware**: Sniffer + Tag cargados, antenas separadas 2m, cable UART, cinta métrica 30m
- [ ] **Software**: STM32CubeIDE, terminal serial, branch `fix/detection-over-20m`
- [ ] **Marcadores**: 10m, 15m, 20m, 25m, 30m

---

## 🎯 HOJA DE RUTA: DIAGNÓSTICO Y SOLUCIÓN CANAL A

### 📋 **TEST-00: Swap Físico de Antenas** [✅ COMPLETADO]
**Fecha**: 28-Oct-2025  
**Estado**: [✅] EJECUTADO  
**Resultado**: ❌ **SIN MEJORA** - Problema permanece en Canal A lógico  

**Evidencia**:
- **Configuración Original**: Canal A → Antena Física 1, Canal B → Antena Física 2
  - Canal A @ 21.7m: 0% éxito (100% timeout)
  - Canal B @ 21.7m: 100% éxito (51/51 detecciones)
  
- **Configuración Swap**: Canal A → Antena Física 2, Canal B → Antena Física 1
  - Canal A @ 21.7m: 0% éxito (100% timeout) ← **Problema sigue en Canal A**
  - Canal B @ 21.7m: 100% éxito ← **Funciona con cualquier antena**

**Conclusión**: ✅ **Problema es SOFTWARE/CONFIGURACIÓN, NO hardware de antenas**

---

### 🔍 **ROOT CAUSE IDENTIFICADO: Estructura `pdw3000local` Compartida**

**Problema**: Ambos chips DW3000 (Canal A y Canal B) comparten **una sola estructura** `dwt_local_data_t *pdw3000local`

**Archivo**: `sniffer-tag/Core/Src/main.cpp` línea 65 y 1117
```c
dwt_local_data_t *pdw3000local;  // ← UNA SOLA estructura para AMBOS chips
pdw3000local = new dwt_local_data_t;  // Línea 1117
```

**Secuencia del Bug**:
1. `init_uwb_device(&uwb_hw_a, ...)` → Llama `dwt_local_data_init(pdw3000local)`
   - Lee OTP del **Chip A**: partID, lotID, dgc_otp_set, bias_tune, etc.
   - Guarda en `pdw3000local`

2. `init_uwb_device(&uwb_hw_b, ...)` → Llama `dwt_local_data_init(pdw3000local)` ← **¡MISMA estructura!**
   - Lee OTP del **Chip B**: partID, lotID, dgc_otp_set, bias_tune, etc.
   - **SOBRESCRIBE** datos del Chip A en `pdw3000local`

3. Durante operación:
   - **Chip A opera con configuración OTP del Chip B** (incorrecta)
   - **Chip B opera con su propia configuración** (correcta)

**Impacto**: 
- Si Chip B tiene mejor calibración DGC en OTP → funciona bien a >20m
- Chip A usa calibración del Chip B → falla en señales débiles >20m
- A distancias cortas (<10m), señal fuerte compensa la mala calibración

---

### ⚠️ **Prioridad 0: FIX CRÍTICO - Separar Estructuras `pdw3000local`** [⏳ EN VALIDACIÓN]
**Estado**: [⏳] CÓDIGO IMPLEMENTADO - VALIDANDO RESULTADOS  
**Fecha implementación**: 28-Oct-2025
**Archivos modificados**:
1. ✅ `sniffer-tag/Core/Src/main.cpp` (líneas 65, 1120-1137)
2. ✅ `sniffer-tag/Core/Src/sniffer_tag.cpp` (init_uwb_device + switch functions)
3. ✅ `sniffer-tag/Core/Inc/sniffer_tag.hpp` (declarations)

**✅ Cambios implementados**:
- [x] Creadas estructuras separadas: `pdw3000local_a`, `pdw3000local_b`, `pdw3000local`
- [x] Inicialización independiente para cada chip
- [x] Modificado `init_uwb_device()` para recibir `dwt_local_data_t *local_data`
- [x] Actualizado `switch_hw()` y `switch_hw_timestamp_query()` para cambiar puntero activo
- [x] Código compilado sin errores

**❌ RESULTADO TEST @ 21m (28-Oct-2025 15:00)**:
```
Tag 0x2783 @ 21m:
- Canal A: 0% éxito (50/50 RX_PREAMBLE_DETECTION_TIMEOUT)
- Canal B: 100% éxito (DistB: 21.16-21.81m, 0 errores)
- Conclusion: PROBLEMA PERSISTE SIN CAMBIOS
```

**🔍 ANÁLISIS**:
La hipótesis de "Canal B sobrescribe calibración OTP de Canal A" parece **INCORRECTA** porque:
1. Estructuras separadas → calibraciones independientes preservadas
2. Resultado idéntico → Canal A sigue fallando completamente
3. Posibles causas alternativas:
   - Ambos chips tienen `dgc_otp_set = DWT_DGC_LOAD_FROM_SW` (sin calibración OTP válida)
   - Problema hardware en Canal A (antena, LNA, filtro RF, PCB routing)
   - CS pin de Canal A realmente tiene problema de inicialización


**🔬 SIGUIENTE PASO: Verificar valores OTP leídos**

**✅ RESULTADO TEST OTP (28-Oct-2025 16:17)**:
```
CANAL A:
[OTP] PartID:0x624831E5 LotID:0x01410075
[OTP] DGC_TUNE:0x10000240 DGC_MODE:OTP
[OTP] Bias:0x13 Xtrim:0x2E

CANAL B:
[OTP] PartID:0x6E483065 LotID:0x01410075
[OTP] DGC_TUNE:0x10000240 DGC_MODE:OTP
[OTP] Bias:0x12 Xtrim:0x2E
```

**📊 ANÁLISIS DIAGNÓSTICO**:

✅ **Estructuras separadas funcionan correctamente**:
- PartID diferentes: `0x624831E5` vs `0x6E483065` ✅
- Bias diferentes: `0x13` vs `0x12` ✅
- Cada estructura almacena valores únicos de su chip

✅ **Ambos chips tienen calibración OTP válida**:
- DGC_MODE:OTP en ambos ✅
- DGC_TUNE:0x10000240 (mismo lote de fabricación)
- LotID:0x01410075 (mismo lote)

🚨 **CONCLUSIÓN CRÍTICA**:
- ✅ La implementación de estructuras separadas es CORRECTA
- ✅ Ambos chips tienen calibración DGC de fábrica
- ❌ **Canal A SIGUE FALLANDO** a pesar de calibración correcta
- ❌ **La hipótesis de "calibración OTP compartida" era INCORRECTA**

---

### 🔍 **NUEVA HIPÓTESIS: Problema HARDWARE en Canal A**

Dado que:
1. Calibración OTP es correcta y está separada
2. TEST-00 mostró que problema sigue al Canal A lógico (no antena física)
3. Canal B funciona perfectamente con cualquier antena

**Posibles causas**:
1. **LNA de Canal A degradado** → Baja sensibilidad (-10 dB típico)
2. **Filtro RF desafinado** → Atenúa señal en Canal 5 (6.5 GHz)
3. **PCB routing defectuoso** → Impedancia incorrecta en RF de Chip A
4. **Cristal/PLL problema** → Frecuencia ligeramente desviada
5. **Problema en chip DW3000 A** → Chip defectuoso de fábrica

**Test definitivo recomendado**:
- Medir potencia RX con analizador de espectro @ -90 dBm
- Comparar Canal A vs Canal B
- Esperado: Canal A muestre ~10 dB menos sensibilidad

---

**Cambios requeridos** (DOCUMENTACIÓN - YA IMPLEMENTADOS):

**1. Crear dos estructuras separadas** (main.cpp línea 65):
```c
// ANTES:
dwt_local_data_t *pdw3000local;

// DESPUÉS:
dwt_local_data_t *pdw3000local_a;  // Para Canal A
dwt_local_data_t *pdw3000local_b;  // Para Canal B
dwt_local_data_t *pdw3000local;    // Puntero activo (apunta a _a o _b)
```

**2. Inicializar ambas estructuras** (main.cpp línea 1117):
```c
// ANTES:
pdw3000local = new dwt_local_data_t;

// DESPUÉS:
pdw3000local_a = new dwt_local_data_t;
pdw3000local_b = new dwt_local_data_t;
pdw3000local = pdw3000local_a;  // Inicialmente apunta a Canal A
```

**3. Modificar `init_uwb_device()` para recibir estructura** (sniffer_tag.cpp):
```c
// ANTES:
void init_uwb_device(Uwb_HW_t *uwb_hw, SPI_HandleTypeDef *hspi, ...);

// DESPUÉS:
void init_uwb_device(Uwb_HW_t *uwb_hw, SPI_HandleTypeDef *hspi, 
                     dwt_local_data_t *local_data, ...);
```

**4. Llamar init con estructura correcta** (main.cpp línea 1126):
```c
// DESPUÉS:
init_uwb_device(&uwb_hw_a, &hspi3, pdw3000local_a, ...);
init_uwb_device(&uwb_hw_b, &hspi3, pdw3000local_b, ...);
```

**5. Cambiar puntero en `switch_hw()`** (sniffer_tag.cpp):
```c
void switch_hw(...) {
    if (hw == &uwb_hw_a) {
        hw = &uwb_hw_b;
        pdw3000local = pdw3000local_b;  // ← Cambiar estructura activa
        dist_ptr = dist_b;
    } else {
        hw = &uwb_hw_a;
        pdw3000local = pdw3000local_a;  // ← Cambiar estructura activa
        dist_ptr = dist_a;
    }
}
```

**Resultado esperado**: 
- Cada chip usa su **propia calibración OTP**
- Configuración DGC correcta para cada canal
- Ambos canales funcionan ≥70% @ 21m

**🔧 Próximos pasos**:
1. ⏳ Implementar cambios en código
2. ⏳ Compilar proyecto Sniffer
3. ⏳ Flashear firmware a Sniffer
4. ⏳ Test físico @ 21m con ambas antenas
5. ⏳ Si ambos ≥70%: **PROBLEMA RESUELTO** ✅
6. ⏳ Si persiste: Investigar forzar DGC desde SW (DWT_DGC_LOAD_FROM_SW)

---

### ⚙️ **Prioridad 1: Aumentar PRE_TIMEOUT** [✅ YA OPTIMIZADO]
**Estado actual**: `PRE_TIMEOUT_6M8 = 12 PAC` (96 símbolos de preámbulo)  
**¿Por qué ayuda?**: Espera más tiempo para detectar inicio de preámbulo en señales débiles  
**Máximo recomendado**: 16 PAC (100% del preámbulo de 128 símbolos)  
**Resultado pruebas**:
- PRE_TIMEOUT=5 @ 23m → Antena A: 0% éxito, B: 100%
- PRE_TIMEOUT=8 @ 20m → Antena A: 3% éxito, B: 10%
- PRE_TIMEOUT=12 (actual) → Esperado: 50% ambas antenas @ 25m

**✅ Acción**: Ya configurado al valor óptimo (12 PAC). No aumentar más a menos que tests muestren <50% éxito.

---

### ⚡ **Prioridad 2: Aumentar TX Power** [⚠️ PENDIENTE CALIBRACIÓN]
**Estado actual**: `TX_POWER = 0xFDFDFDFD` (Coarse gain=1, Fine gain=63)  
**¿Por qué ayuda?**: Mayor potencia transmitida = señal más fuerte en recepción a largas distancias  
**Máximo permitido**: `0xFFFFFFFF` (Coarse gain=3, Fine gain=63) - pero requiere **calibración RF**  
**⚠️ Advertencia**: 
- Aumentar power sin calibrar puede violar límites regulatorios (-41.3 dBm/MHz)
- Requiere medir espectro RF con analizador antes de incrementar

**🔧 Acción propuesta**:
```c
// En sniffer_tag.cpp y main.cpp línea ~56:
static dwt_txconfig_t dwt_tx_cfg = { 
    0x34,       /* PG delay */
    0xFEFEFEFE, /* TX power: Coarse gain=2, Fine gain=63 (moderado) */
    0x0         /* PG count */
};
```

**⚠️ CRÍTICO**: Medir con analizador de espectro antes de aprobar cambio. Si no tienes equipo, deja en 0xFDFDFDFD.

---

### 📡 **Prioridad 3: Mejorar RX Sensitivity / Calibración** [🧪 REQUIERE PRUEBA]
**Estado actual**: Sin calibración de sensibilidad documentada  
**¿Por qué ayuda?**: Mejor discriminación señal/ruido = detecta señales más débiles  
**Acciones sugeridas**:
1. **Calibración AGC (Automatic Gain Control)**:
   - Usar `dwt_rxcal()` para calibrar ganancia RX
   - Configurar umbrales de detección más agresivos
2. **Ajustar Threshold de detección**:
   - Bajar threshold en registro `DRX_CONF` para detectar señales más débiles
   - Trade-off: Mayor tasa de falsos positivos

**🔧 Test propuesto**:
- Capturar niveles RX con `dwt_readdiagnostics()` @ 25m
- Si First Path Power < -95 dBm → Señal muy débil, ajustar thresholds
- Si > -85 dBm → Problema no es RX sensitivity

---

### 📶 **Prioridad 4: Antenas de Mayor Ganancia** [💰 HARDWARE UPGRADE]
**Estado actual**: Antenas desconocidas (probablemente ≤2 dBi)  
**¿Por qué ayuda?**: Mejor ganancia = ±4-6 dB extra = duplica/triplica rango efectivo  
**Opciones**:
- **Bajo costo**: Antenas 3 dBi monopolo → +1 dB mejora (~10% más rango)
- **Medio costo**: Antenas 5 dBi direccionales → +3 dB mejora (~40% más rango)
- **Profesional**: Antenas 8 dBi patch → +6 dB mejora (2x rango)

**📋 Especificaciones requeridas**:
- Frecuencia: 6.0-6.5 GHz (Canal 5 UWB)
- Impedancia: 50Ω
- Conector: U.FL/IPEX compatible con DW3000

**💡 Tip**: Primero optimizar firmware/power, luego invertir en antenas si sigue fallando.

---

### 🔇 **Prioridad 5: Reducir Interferencias** [🧪 CAMBIO DE CANAL]
**Estado actual**: Canal 5 (6.5 GHz) hardcoded  
**¿Por qué ayuda?**: Otras redes UWB/WiFi 6E pueden interferir en 6.5 GHz  
**Acción propuesta**:
```c
// En sniffer_tag.cpp línea ~89 y ~51:
dwt_config_t dwt_cfg = { 
    9,  /* Channel number: cambiar de 5 → 9 (8.0 GHz) */
    // ... resto igual
};
```

**⚠️ Nota**: Canal 9 tiene menor rango máximo pero menos interferencia. Test en ambos canales.

**🔧 Test interferencia**:
1. Ejecutar sniffer en modo spectrum analyzer (si disponible)
2. Medir ocupación espectral en Canal 5 vs Canal 9
3. Elegir canal con menor ruido de fondo

---

## 🚀 TESTS SIMPLIFICADOS (Ordenados por Prioridad)

### **🔴 PRIORIDAD CRÍTICA - Diagnóstico Ant-A**

| # | Prueba | Valida | Cómo hacerlo | ✅ Validado |
|---|--------|--------|--------------|-------------|
| **TEST-00** | **Swap Antenas A ↔ B** | Identificar si problema es hardware o config | 1. Apagar equipos<br>2. **Sniffer**: Desconectar antenas y reconectar: Ant-A→puerto B, Ant-B→puerto A<br>3. **Tag**: Ídem swap físico<br>4. Test @ 21m: Si problema se mueve con antena física → **Hardware/cable**, Si queda en canal A → **Config DW3000** | [✅] CFG |

### **🟡 PRIORIDAD ALTA - Optimización Software**

| # | Prueba | Valida | Cómo hacerlo | ✅ Validado |
|---|--------|--------|--------------|-------------|
| **TEST-01** | **PRE_TIMEOUT aumentado** | Mejora detección débil >20m | 1. Aumentar `PRE_TIMEOUT_6M8` 12→16 en `uwb3000Fxx.h` (ambos equipos)<br>2. Compilar y flashear<br>3. Test @ 21m Ant-A: ≥50% éxito esperado | [ ] SÍ [ ] NO |
| **TEST-02** | **TX Power incrementado** | Mayor alcance por señal fuerte | 1. Cambiar `TX_POWER` 0xFDFDFDFD→0xFEFEFEFE en ambos<br>2. ⚠️ **OPCIONAL**: Medir espectro antes<br>3. Test 25-30m: mejora ≥30% | [ ] SÍ [ ] NO |
| **TEST-03** | **Query timeout aumentado** | Más tiempo para queries lentas | 1. Cambiar `query_timeout` 1000→2000 en `main.cpp`<br>2. Flashear sniffer<br>3. Comparar 25m: mejora ≥20% | [ ] SÍ [ ] NO |

### **🟢 PRIORIDAD MEDIA - Robustez y Logging**

| # | Prueba | Valida | Cómo hacerlo | ✅ Validado |
|---|--------|--------|--------------|-------------|
| **TEST-04** | **Validación de guardado** | No guarda datos incompletos | 1. Agregar validación `dist_a > 0.1 && dist_b > 0.1` en `main.cpp` líneas 542-546<br>2. Flashear sniffer<br>3. Test 3 escenarios: 15m, 28m, obstrucción | [ ] SÍ [ ] NO |
| **TEST-05** | **Sistema de logging** | Diagnóstico de fallos | 1. Agregar `log_rx_result()` en `sniffer_tag.cpp`<br>2. Flashear sniffer<br>3. Capturar logs UART durante tests | [ ] SÍ [ ] NO |
| **TEST-06** | **Modo MULTIPLE obligatorio** | Siempre ambas antenas | 1. Eliminar `TAG_ONE_DETECTION` en `main.cpp`<br>2. Flashear sniffer<br>3. Test 15m: 100% ambas antenas | [ ] SÍ [ ] NO |

### **🔵 PRIORIDAD BAJA - Último Recurso**

| # | Prueba | Valida | Cómo hacerlo | ✅ Validado |
|---|--------|--------|--------------|-------------|
| **TEST-07** | **Timeouts agresivos** | Si TEST-01 no funciona | 1. `PRE_TIMEOUT_6M8` = 16 PAC<br>2. `POLL_TX_TO_RESP_RX_DLY_UUS_6M8` = 1400<br>3. `RESP_RX_TIMEOUT_UUS_6M8` = 700<br>4. ≥70% a 25m | [ ] SÍ [ ] NO |
| **TEST-08** | **Cambio de canal** | Reducir interferencia | 1. Canal 5→9 en `dwt_cfg` (ambos)<br>2. Retest rango completo<br>3. Comparar SNR vs Canal 5 | [ ] SÍ [ ] NO |

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
