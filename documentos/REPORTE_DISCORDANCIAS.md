# Reporte de Discordancias y Correcciones en Documentación

## Fecha: 2025-10-22
## Estado: Análisis Completo

---

## 1. Discordancias Encontradas

### 1.1 Valores de Timeout - CORRECTOS ✓

Los documentos reportan correctamente:
- `POLL_TX_TO_RESP_RX_DLY_UUS_6M8 = 700` ✓
- `PRE_TIMEOUT_6M8 = 5` ✓
- `RESP_RX_TIMEOUT_UUS_6M8 = 300` ✓
- `query_timeout = 1000 ms` ✓ (tanto en sniffer como en tag/persona)

**Verificación en código:**
- `sniffer/Core/Inc/uwb3000Fxx.h` líneas 726, 730, 733
- `sniffer/Core/Src/main.cpp` línea 347
- `Persona/Core/Src/main.cpp` línea 318

---

### 1.2 Offsets de Timestamps - DISCORDANCIA DETECTADA ⚠️

**Problema:** Los documentos mencionan que hay "offsets distintos" pero no los especifican claramente.

**Realidad del código:**

#### Discovery (primera detección):
```cpp
// sniffer/Core/Src/main.cpp líneas 430-431
const int poll_rx_offset = 5;
const int resp_tx_offset = 9;
```

#### Timestamp Query (consultas subsecuentes):
```cpp
// sniffer/Core/Src/main.cpp líneas 492-493
const int poll_rx_offset = 1;
const int resp_tx_offset = 1 + 4;  // = 5
```

**Estructura de frames inferida:**

**Frame Discovery Response (desde tag):**
```
[0] = command (TAG_ID_QUERY confirmación)
[1-4] = tag.id (4 bytes)
[5-8] = poll_rx_timestamp (4 bytes) ← offset 5
[9-12] = resp_tx_timestamp (4 bytes) ← offset 9
[13-14] = Battery_Voltage (2 bytes)
... más datos
```

**Frame Timestamp Query Response (desde tag):**
```
[0] = command (TAG_TIMESTAMP_QUERY o TAG_SET_SLEEP_MODE)
[1-4] = poll_rx_timestamp (4 bytes) ← offset 1
[5-8] = resp_tx_timestamp (4 bytes) ← offset 5
```

**CORRECCIÓN NECESARIA:** Los documentos deben especificar explícitamente estos offsets diferentes y la razón (estructura de frame distinta entre discovery y queries).

---

### 1.3 Timeout "Típicamente 1000-2000ms" - IMPRECISO ⚠️

**En documento:** `ANALISIS_DISCORDANCIA_LECTURAS_UWB.md`
> "Timestamp Query: típicamente 1000-2000ms (puede ser insuficiente a >20m)"

**Realidad:**
- El valor es **exactamente 1000 ms** en ambos equipos.
- El rango "1000-2000ms" sugiere variabilidad que NO existe en el código actual.

**CORRECCIÓN NECESARIA:** Cambiar a "query_timeout = 1000 ms (puede ser insuficiente a >20m, se recomienda aumentar a 1500-2000 ms)".

---

### 1.4 RX_TIMEOUT_MS No Mencionado en Documentos Principales ⚠️

**Problema:** Los primeros documentos no mencionan `RX_TIMEOUT_MS = 100` definido en `sniffer_tag.cpp:532`.

**Realidad:**
```c
// sniffer/Core/Src/sniffer_tag.cpp línea 532
#define RX_TIMEOUT_MS 100
```

Este timeout actúa como capa adicional sobre los timeouts del chip DW3000.

**CORRECCIÓN NECESARIA:** Solo `ANALISIS_RX_TIMEOUT_MS.md` lo menciona correctamente. Los otros documentos deberían al menos referenciarlo como timeout adicional de aplicación.

---

### 1.5 Máquina de Estados - INCOMPLETA ⚠️

**En documentos:** Se mencionan estados pero no todos los casos especiales.

**Estados reales del sniffer:**
- `TAG_DISCOVERY`
- `TAG_SEND_TIMESTAMP_QUERY`
- `TAG_END_READINGS`
- `TAG_ONE_DETECTION` (modo especial con un solo tag)
- `TAG_RX_CRC_VALID`
- `TAG_RX_NO_COMMAND`
- `TAG_RX_ERROR`
- `TAG_RX_FRAME_TIMEOUT`
- `TAG_RX_PREAMBLE_DETECTION_TIMEOUT`
- `TAG_NO_RXCG_DETECTED`
- `TAG_TX_ERROR`

**Estados reales del tag (Persona):**
- `TAG_WAIT_FOR_FIRST_DETECTION`
- `TAG_WAIT_FOR_TIMESTAMPT_QUERY`
- `TAG_SLEEP`
- `TAG_SLEEP_RECIVED`
- `TAG_SHIP_MODE_SET`

**CORRECCIÓN NECESARIA:** Agregar diagrama completo de estados en `ESQUEMA_COMUNICACION_UWB.md`.

---

### 1.6 Flujo de Guardado - CRÍTICO ⚠️

**Problema detectado en código pero NO suficientemente enfatizado en docs:**

El código guarda tags **SIN validar** que ambas antenas tengan lecturas:

```cpp
// sniffer/Core/Src/main.cpp línea 542-546
if (HAL_GetTick() - query_ticks > query_timeout)
{
    debug_distance_new(tag, tag_status, distance_a, distance_b);
    save_two_maps_and_clear_tag(distance_a, distance_b, tag_map_od, tag, lora_send_ticks, tag_map);
    tag_status = TAG_DISCOVERY;
}
```

**No hay verificación de:**
- `distance_a.get_counter() > 0`
- `distance_b.get_counter() > 0`

**CORRECCIÓN NECESARIA:** `ANALISIS_LOGICA_SIN_RX_TIMEOUT.md` menciona esto pero debería estar en **TODOS** los documentos como problema crítico #1.

---

### 1.7 Alternancia de Antenas - LÓGICA CORRECTA PERO MAL EXPLICADA ⚠️

**En código:**
```cpp
// sniffer/Core/Src/sniffer_tag.cpp líneas 1044-1050
void switch_hw_timestamp_query(TAG_t *tag, DistanceHandler *&dist_ptr,
        Uwb_HW_t *&hw, DistanceHandler *dist_a, DistanceHandler *dist_b) {
    if ((hw == &uwb_hw_a) && (dist_b->get_counter() < dist_b->get_total_readings_for_two_transcievers() / 2)) {
        hw = &uwb_hw_b;
        dist_ptr = dist_b;
    } else if (dist_a->get_counter() < dist_a->get_total_readings_for_two_transcievers() / 2){
        hw = &uwb_hw_a;
        dist_ptr = dist_a;
    }
}
```

**Lógica real:**
1. Si está en antena A Y la antena B tiene menos de la mitad de lecturas → cambiar a B
2. Si no, si antena A tiene menos de la mitad de lecturas → cambiar a A
3. Si no, mantener antena actual

**CORRECCIÓN NECESARIA:** Los documentos dicen "alterna basándose en contadores" pero no explican la lógica condicional completa.

---

### 1.8 Cálculo de Distancia - FÓRMULA NO DOCUMENTADA ⚠️

**En código:**
```cpp
// Cálculo RTD (Round Trip Delay)
uint64_t rtd_init = get_rx_timestamp_u64() - get_tx_timestamp_u64();
uint64_t rtd_resp = resp_tx_timestamp - poll_rx_timestamp;
float clockOffsetRatio = dwt_readclockoffset() / (float)(1 << 26);

// En DistanceHandler se calcula:
double tof = ((rtd_init - rtd_resp * (1 - clockOffsetRatio)) / 2.0) * DWT_TIME_UNITS;
double distance = tof * SPEED_OF_LIGHT;
```

**CORRECCIÓN NECESARIA:** Ningún documento explica la fórmula de Two-Way Ranging con compensación de clock offset.

---

### 1.9 sleep_time - VALORES ESPECÍFICOS NO DOCUMENTADOS ⚠️

**En tag (Persona):**
```cpp
// Persona/Core/Src/main.cpp líneas 310-312
uint32_t sleep_time_not_recived = 500;   // ms
uint32_t sleep_time_recived = 15000;      // ms
uint32_t sleep_time = 1000;               // ms
```

**CORRECCIÓN NECESARIA:** Los documentos mencionan "tiempos de sleep" pero no los valores concretos ni cuándo se usa cada uno.

---

### 1.10 Comandos del Protocolo - INCOMPLETO ⚠️

**Comandos inferidos del código:**
- `TAG_ID_QUERY` (discovery)
- `TAG_TIMESTAMP_QUERY` (ranging)
- `TAG_SET_SLEEP_MODE` (finalización)

**Otros posibles:**
- `TAG_SHIP_MODE_SET` (mencionado en tag)

**CORRECCIÓN NECESARIA:** No hay documento con tabla completa de comandos y sus payloads.

---

## 2. Documento con Errores Críticos

### `ANALISIS_LOGICA_SIN_RX_TIMEOUT.md`
- ✓ Identifica correctamente problema de guardado sin validación
- ✓ Identifica problema de alternancia simple
- ⚠️ Menciona offsets pero no los especifica

### `ANALISIS_IMPACTO_TIEMPOS_UWB.md`
- ✓ Valores de timeouts correctos
- ⚠️ No menciona RX_TIMEOUT_MS

### `ANALISIS_DISCORDANCIA_LECTURAS_UWB.md`
- ⚠️ "típicamente 1000-2000ms" es impreciso
- ⚠️ No detalla offsets diferentes

### `RELACION_TIMEOUTS_UWB.md`
- ✓ Buena explicación de interacción
- ⚠️ Falta RX_TIMEOUT_MS

### `ANALISIS_RX_TIMEOUT_MS.md`
- ✓ Correcto y detallado
- ✓ Única referencia al timeout de aplicación

### `ESQUEMA_COMUNICACION_UWB.md`
- ⚠️ Falta diagrama completo de estados
- ⚠️ Falta estructura de frames
- ⚠️ Falta tabla de comandos

### `ANALISIS_INTEGRADO_TIMEOUTS_Y_LOGICA.md`
- ✓ Buena síntesis
- ⚠️ Podría incluir más detalles de código real

---

## 3. Resumen de Acciones Correctivas

| Documento | Acción Requerida | Prioridad |
|-----------|------------------|-----------|
| `ANALISIS_DISCORDANCIA_LECTURAS_UWB.md` | Corregir "1000-2000ms" → "1000ms fijo" | Media |
| `ANALISIS_IMPACTO_TIEMPOS_UWB.md` | Agregar mención a RX_TIMEOUT_MS | Baja |
| `RELACION_TIMEOUTS_UWB.md` | Agregar RX_TIMEOUT_MS en jerarquía | Baja |
| `ESQUEMA_COMUNICACION_UWB.md` | Agregar offsets, comandos, estados completos | **Alta** |
| `ANALISIS_LOGICA_SIN_RX_TIMEOUT.md` | Especificar offsets 5/9 vs 1/5 | Alta |
| **NUEVO DOC REQUERIDO** | Protocolo completo con frames y comandos | **Crítica** |
| **NUEVO DOC REQUERIDO** | Fórmula de Two-Way Ranging | Media |
| `ANALISIS_INTEGRADO_TIMEOUTS_Y_LOGICA.md` | Actualizar con valores sleep específicos | Baja |

---

## 4. Conclusión

La documentación es **mayormente correcta** en valores de timeouts principales pero:
- **Falta especificidad** en offsets y estructura de frames
- **Falta documentación** de protocolo completo
- **Falta énfasis** en problemas críticos (guardado sin validación)
- **Inconsistencia** en mencionar RX_TIMEOUT_MS

**Recomendación:** Crear documento maestro `PROTOCOLO_UWB_COMPLETO.md` con:
1. Tabla de comandos
2. Estructura de cada tipo de frame
3. Diagramas de estado completos
4. Fórmulas de cálculo
5. Valores de configuración consolidados
