# Plan de Corrección: Detección UWB a >20 Metros

## Fecha: 2025-10-22
## Objetivo: Solucionar la falla en detección de tags a distancias superiores a 20 metros
## Basado en: Análisis completo de código y documentación técnica

---

## Resumen Ejecutivo

### Problema Principal
El sistema dual-antenna UWB (sniffer) **detecta tags correctamente a <20m pero falla a >20m**, causando:
- ❌ Solo una antena captura lecturas (imposibilita triangulación)
- ❌ Timeouts frecuentes sin datos completos
- ❌ Guardado de datos incompletos sin validación

### Causas Identificadas (en orden de impacto)
1. **CRÍTICO**: Timeouts físicos del chip DW3000 demasiado cortos (RESP_RX_TIMEOUT_UUS_6M8=300 → ~308µs)
2. **CRÍTICO**: Falta de validación antes de guardar datos incompletos (líneas 542-546 main.cpp)
3. **ALTO**: Timeout PRE_TIMEOUT_6M8=5 insuficiente para detectar preámbulo en señales débiles
4. **MEDIO**: query_timeout=1000ms puede ser insuficiente con múltiples reintentos
5. **BAJO**: Alternancia mecánica sin considerar calidad de señal

---

## Soluciones Ordenadas por Impacto Esperado

### 🔴 PRIORIDAD 1: Aumentar Timeouts del Chip DW3000 (IMPACTO: 85%)

**Problema**: A >20m, el tiempo de vuelo (ToF) + procesamiento del tag excede las ventanas de recepción configuradas.

**Valores Actuales**:
```c
// sniffer/Core/Inc/uwb3000Fxx.h líneas 726-733
// Persona/Core/Inc/uwb3000Fxx.h líneas 725-732
#define POLL_TX_TO_RESP_RX_DLY_UUS_6M8 700   // ~718 µs
#define RESP_RX_TIMEOUT_UUS_6M8 300          // ~308 µs
#define PRE_TIMEOUT_6M8 5                     // símbolos
```

**⚠️ Estos valores están duplicados en ambos proyectos y deben mantenerse sincronizados.**

| Parámetro | Ubicación Sniffer | Ubicación Persona | Valor Actual | ¿Debe Coincidir? |
|-----------|-------------------|-------------------|--------------|------------------|
| POLL_TX_TO_RESP_RX_DLY_UUS_6M8 | uwb3000Fxx.h:726 | uwb3000Fxx.h:725 | 700 | ✅ SÍ (crítico) |
| RESP_RX_TIMEOUT_UUS_6M8 | uwb3000Fxx.h:730 | uwb3000Fxx.h:729 | 300 | ✅ SÍ (crítico) |
| PRE_TIMEOUT_6M8 | uwb3000Fxx.h:733 | uwb3000Fxx.h:732 | 5 | ✅ SÍ (crítico) |

**Cálculo del Problema**:
- Velocidad de luz: 299,792,458 m/s
- ToF a 25m (ida y vuelta 50m): 50m / 299,792,458 m/s = **166.8 ns** ≈ **0.167 µs**
- ToF a 30m (ida y vuelta 60m): **0.200 µs**
- **PERO**: El problema NO es solo ToF, sino el **retardo de procesamiento del tag**

**Retardos Reales en el Sistema**:
1. Sniffer TX poll → propagación → Tag RX
2. Tag procesa (decenas de µs)
3. Tag TX response → propagación → Sniffer RX
4. **Total estimado a 25m**: 100-200 µs de ToF + 50-150 µs procesamiento = **150-350 µs**

**La ventana actual (308 µs) es MARGINAL y no deja tolerancia.**

#### Solución 1A: Valores Conservadores (+50%)

**⚠️ CRÍTICO: Modificar en AMBOS equipos (Sniffer Y Persona/Tag)**

Los timeouts están duplicados en ambos códigos y **DEBEN coincidir** para que el protocolo funcione correctamente.

```c
// ========== ARCHIVO 1: sniffer/Core/Inc/uwb3000Fxx.h ==========
// Líneas 726, 730, 733
#define POLL_TX_TO_RESP_RX_DLY_UUS_6M8 1000  // Era 700, +43%
#define RESP_RX_TIMEOUT_UUS_6M8 500          // Era 300, +67%
#define PRE_TIMEOUT_6M8 8                    // Era 5, +60%

// ========== ARCHIVO 2: Persona/Core/Inc/uwb3000Fxx.h ==========
// Líneas 725, 729, 732 (verificar exactas con grep)
#define POLL_TX_TO_RESP_RX_DLY_UUS_6M8 1000  // Era 700, +43%
#define RESP_RX_TIMEOUT_UUS_6M8 500          // Era 300, +67%
#define PRE_TIMEOUT_6M8 8                    // Era 5, +60%
```

**Aplicación en código** (automática al compilar):
```c
// sniffer/Core/Src/uwb3000Fxx.c líneas 111-114
// Persona/Core/Src/uwb3000Fxx.c líneas 112-114
if (rate == RATE_6M8) {
    dwt_setrxaftertxdelay(POLL_TX_TO_RESP_RX_DLY_UUS_6M8);  // Usa nuevo valor
    dwt_setrxtimeout(RESP_RX_TIMEOUT_UUS_6M8);              // Usa nuevo valor
    dwt_setpreambledetecttimeout(PRE_TIMEOUT_6M8);           // Usa nuevo valor
}
```

**Impacto esperado**: 85% de mejora en detección >20m

**Archivos a modificar**:
1. `sniffer/Core/Inc/uwb3000Fxx.h` (líneas 726, 730, 733)
2. `Persona/Core/Inc/uwb3000Fxx.h` (líneas 725, 729, 732)

**⚠️ ADVERTENCIA**: Si solo modificas uno de los equipos, el protocolo fallará completamente porque:
- El Sniffer abrirá ventana RX esperando con timeout X
- El Tag responderá con timing configurado para timeout Y
- Resultado: Desincronización total

**Ventajas**:
✅ Solución más directa al problema físico
✅ Sin cambios en lógica de código
✅ Mantiene compatibilidad con protocolo actual
✅ Afecta tanto TX del Sniffer como RX del Tag (bilateral)

**Desventajas**:
⚠️ Aumenta consumo energético (ventanas RX más largas en ambos equipos)
⚠️ Reduce throughput (más tiempo por lectura)
⚠️ **REQUIERE reflashear AMBOS equipos simultáneamente** (no compatible con equipos viejos)

**Procedimiento de actualización**:
1. Hacer backup de ambos códigos
2. Modificar `sniffer/Core/Inc/uwb3000Fxx.h`
3. Modificar `Persona/Core/Inc/uwb3000Fxx.h` con **valores idénticos**
4. Compilar ambos proyectos sin warnings
5. Flashear Sniffer
6. Flashear Tag (Persona)
7. Verificar en logs que ambos usan mismos timeouts (agregar debug_printf si es necesario)

**Test requerido**: TEST-01

---

#### Solución 1B: Valores Agresivos (+100%) - Si 1A no es suficiente

**⚠️ CRÍTICO: Modificar en AMBOS equipos (Sniffer Y Persona/Tag)**

```c
// ========== ARCHIVO 1: sniffer/Core/Inc/uwb3000Fxx.h ==========
// Líneas 726, 730, 733
#define POLL_TX_TO_RESP_RX_DLY_UUS_6M8 1400  // Era 700, +100%
#define RESP_RX_TIMEOUT_UUS_6M8 700          // Era 300, +133%
#define PRE_TIMEOUT_6M8 12                   // Era 5, +140%

// ========== ARCHIVO 2: Persona/Core/Inc/uwb3000Fxx.h ==========
// Líneas 725, 729, 732 (verificar exactas)
#define POLL_TX_TO_RESP_RX_DLY_UUS_6M8 1400  // Era 700, +100%
#define RESP_RX_TIMEOUT_UUS_6M8 700          // Era 300, +133%
#define PRE_TIMEOUT_6M8 12                   // Era 5, +140%
```

**Impacto esperado**: 90% si 1A solo logró mejora parcial

**Archivos a modificar**:
1. `sniffer/Core/Inc/uwb3000Fxx.h` (líneas 726, 730, 733)
2. `Persona/Core/Inc/uwb3000Fxx.h` (líneas 725, 729, 732)

**⚠️ Solo implementar si TEST-01 con Solución 1A falla en lograr 70% a 25m**

**Test requerido**: TEST-01B (solo si TEST-01 falla)

---

### 🔴 PRIORIDAD 2: Validación Antes de Guardar Datos (IMPACTO: 75%)

**Problema Crítico**: El código actual guarda datos **SIN validar** que ambas antenas tengan lecturas.

**Código Actual (INCORRECTO)**:
```cpp
// sniffer/Core/Src/main.cpp líneas 542-546
if (HAL_GetTick() - query_ticks > query_timeout)
{
    debug_distance_new(tag, tag_status, distance_a, distance_b);
    save_two_maps_and_clear_tag(distance_a, distance_b, tag_map_od, tag, lora_send_ticks, tag_map);
    tag_status = TAG_DISCOVERY;
}
```

**Consecuencia**: Si timeout ocurre con solo 1 antena con datos, guarda triangulación inválida.

#### Solución 2A: Validación Obligatoria

```cpp
// sniffer/Core/Src/main.cpp líneas 542-550
if (HAL_GetTick() - query_ticks > query_timeout)
{
    debug_distance_new(tag, tag_status, distance_a, distance_b);
    
    // ✅ VALIDACIÓN AGREGADA
    if (distance_a->get_counter() > 0 && distance_b->get_counter() > 0) {
        // Solo guarda si AMBAS antenas tienen lecturas
        save_two_maps_and_clear_tag(distance_a, distance_b, tag_map_od, tag, lora_send_ticks, tag_map);
    } else {
        // Descarta tag sin datos completos
        debug_printf("Tag %08X descartado: A=%d lecturas, B=%d lecturas\n", 
                     tag.id, distance_a->get_counter(), distance_b->get_counter());
        distance_a->clear_memory();
        distance_b->clear_memory();
    }
    
    tag_status = TAG_DISCOVERY;
}
```

**Impacto esperado**: 75% - evita datos corruptos y falsos positivos

**Archivos a modificar**:
- `sniffer/Core/Src/main.cpp` (líneas 542-546)

**Ventajas**:
✅ Evita triangulaciones inválidas
✅ Mejora calidad de datos
✅ Fácil de implementar

**Desventajas**:
⚠️ Puede descartar tags que estén en el límite de alcance
⚠️ Requiere logging para debugging

**Test requerido**: TEST-02

---

### 🟠 PRIORIDAD 3: Aumentar query_timeout (IMPACTO: 60%)

**Problema**: Con timeouts físicos aumentados y posibles reintentos, 1000ms puede no ser suficiente.

**Código Actual**:
```cpp
// sniffer/Core/Src/main.cpp línea 347
const uint32_t query_timeout = 1000;
```

#### Solución 3A: Timeout Adaptativo

```cpp
// sniffer/Core/Src/main.cpp líneas 346-350
// Timeout base + margen según lecturas requeridas
const uint32_t base_timeout = 1000;
const uint32_t timeout_per_reading = 150; // ms extra por lectura
const uint32_t query_timeout = base_timeout + 
    (distance_a->get_total_readings_for_two_transcievers() * timeout_per_reading);
```

**Ejemplo**: 
- 10 lecturas totales → 1000 + (10 × 150) = 2500ms

**Impacto esperado**: 60% - permite completar más ciclos de lectura

**Archivos a modificar**:
- `sniffer/Core/Src/main.cpp` (líneas 346-350)

**Ventajas**:
✅ Se adapta a configuración de lecturas
✅ Mejora robustez sin desperdiciar tiempo

**Desventajas**:
⚠️ Aumenta latencia total del sistema

**Test requerido**: TEST-03

---

#### Solución 3B: Timeout Fijo Aumentado (alternativa simple)

```cpp
// sniffer/Core/Src/main.cpp línea 347
const uint32_t query_timeout = 2000; // Era 1000, +100%
```

**Impacto esperado**: 55% si no quieres lógica adaptativa

**Test requerido**: TEST-03B

---

### 🟠 PRIORIDAD 4: Mejorar Alternancia de Antenas (IMPACTO: 40%)

**Problema**: `switch_hw_timestamp_query()` alterna mecánicamente sin considerar si la antena actual tiene buena señal.

**Código Actual**:
```cpp
// sniffer/Core/Src/sniffer_tag.cpp líneas 1042-1050
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

#### Solución 4A: Priorizar Antena con Mejor Señal

```cpp
// sniffer/Core/Src/sniffer_tag.cpp
void switch_hw_timestamp_query_improved(TAG_t *tag, DistanceHandler *&dist_ptr,
        Uwb_HW_t *&hw, DistanceHandler *dist_a, DistanceHandler *dist_b,
        uint8_t last_rssi_a, uint8_t last_rssi_b) {
    
    uint8_t counter_a = dist_a->get_counter();
    uint8_t counter_b = dist_b->get_counter();
    uint8_t half_target = dist_a->get_total_readings_for_two_transcievers() / 2;
    
    // Si ambas necesitan lecturas, priorizar la de mejor RSSI
    if (counter_a < half_target && counter_b < half_target) {
        if (last_rssi_a > last_rssi_b) {
            hw = &uwb_hw_a;
            dist_ptr = dist_a;
        } else {
            hw = &uwb_hw_b;
            dist_ptr = dist_b;
        }
    }
    // Lógica original para balance
    else if ((hw == &uwb_hw_a) && (counter_b < half_target)) {
        hw = &uwb_hw_b;
        dist_ptr = dist_b;
    } else if (counter_a < half_target){
        hw = &uwb_hw_a;
        dist_ptr = dist_a;
    }
}
```

**Impacto esperado**: 40% - mejora eficiencia pero no resuelve problema base

**Archivos a modificar**:
- `sniffer/Core/Src/sniffer_tag.cpp` (líneas 1042-1050)
- `sniffer/Core/Inc/sniffer_tag.hpp` (declaración)

**Ventajas**:
✅ Aprovecha mejor antena con señal más fuerte
✅ Reduce intentos en antena con mala recepción

**Desventajas**:
⚠️ Requiere lectura de RSSI del chip
⚠️ Más complejidad en lógica

**Test requerido**: TEST-04

---

### 🟡 PRIORIDAD 5: Logging y Diagnóstico (IMPACTO: 20% directo, 100% para debugging)

**Problema**: Sin logs detallados es imposible saber qué falla exactamente (timeout tipo, antena afectada, RSSI, etc).

#### Solución 5A: Sistema de Logging Completo

```cpp
// sniffer/Core/Src/sniffer_tag.cpp - Agregar después de cada wait_rx_data()

void log_rx_result(uint8_t result, TAG_t *tag, Uwb_HW_t *hw, 
                   DistanceHandler *dist_ptr, uint32_t elapsed_ms) {
    const char* hw_name = (hw == &uwb_hw_a) ? "A" : "B";
    
    switch(result) {
        case TAG_RX_CRC_VALID:
            debug_printf("[%s] Tag %08X: RX OK, lecturas=%d, tiempo=%dms\n", 
                        hw_name, tag->id, dist_ptr->get_counter(), elapsed_ms);
            break;
        case TAG_RX_PREAMBLE_DETECTION_TIMEOUT:
            debug_printf("[%s] Tag %08X: PREAMBULO TIMEOUT (señal débil?)\n", 
                        hw_name, tag->id);
            break;
        case TAG_RX_FRAME_TIMEOUT:
            debug_printf("[%s] Tag %08X: FRAME TIMEOUT (ventana cerró)\n", 
                        hw_name, tag->id);
            break;
        case TAG_NO_RXCG_DETECTED:
            debug_printf("[%s] Tag %08X: NO_RXCG (100ms sin evento)\n", 
                        hw_name, tag->id);
            break;
        case TAG_RX_ERROR:
            debug_printf("[%s] Tag %08X: CRC ERROR\n", 
                        hw_name, tag->id);
            break;
    }
}
```

**Impacto esperado**: 20% mejora directa, pero **esencial para validar otras correcciones**

**Archivos a modificar**:
- `sniffer/Core/Src/sniffer_tag.cpp` (múltiples ubicaciones)
- `sniffer/Core/Src/main.cpp` (agregar logs en transiciones)

**Ventajas**:
✅ Identifica exactamente qué tipo de timeout ocurre
✅ Permite ver diferencias entre antenas A y B
✅ Facilita debugging de otras correcciones

**Desventajas**:
⚠️ Overhead de UART (puede enlentecer sistema)
⚠️ Requiere buffer de debug adecuado

**Test requerido**: TEST-05 (ejecutar con cada otro test)

---

### 🟡 PRIORIDAD 6: Modo TAG_ONE_DETECTION (IMPACTO: 15%)

**Problema**: Este modo guarda tag sin hacer queries adicionales, perdiendo oportunidad de triangulación.

**Código Actual**:
```cpp
// sniffer/Core/Src/main.cpp líneas 550-560
else if (tag_status == TAG_ONE_DETECTION)
{
    tag_status = send_sleep_mode(&tag, uwb_tx_one.get_hw(), &distance_a);
    if (tag_status == TAG_RX_CRC_VALID)
    {
        // Solo guarda lectura de primera antena detectora
        save_one_map_and_clear_tag(distance_a, tag_map_od, &tag, lora_send_ticks);
        tag_status = TAG_DISCOVERY;
    }
}
```

#### Solución 6A: Forzar Modo MULTIPLE_DETECTION

```cpp
// sniffer/Core/Src/main.cpp - Cambiar línea ~410
// Opción 1: Eliminar modo ONE_DETECTION completamente
if (tag_status == TAG_RX_CRC_VALID) {
    // Siempre usar modo MULTIPLE para triangulación
    tag_status = TAG_SEND_TIMESTAMP_QUERY;
    query_ticks = HAL_GetTick();
}

// Comentar/eliminar el else if (tag_status == TAG_ONE_DETECTION) completo
```

**Impacto esperado**: 15% - solo afecta tags en modo ONE

**Archivos a modificar**:
- `sniffer/Core/Src/main.cpp` (líneas ~410, 550-560)

**Ventajas**:
✅ Todos los tags pasan por proceso completo de triangulación
✅ Simplifica lógica del código

**Desventajas**:
⚠️ Aumenta tiempo de procesamiento por tag
⚠️ Puede haber razón histórica para modo ONE (revisar requirements)

**Test requerido**: TEST-06

---

## Estrategia de Implementación Incremental

### Fase 1: Correcciones Críticas (Semana 1)
1. **Día 1**: Modificar timeouts en ambos archivos .h (Solución 1A)
   - `sniffer/Core/Inc/uwb3000Fxx.h`
   - `Persona/Core/Inc/uwb3000Fxx.h`
2. **Día 2**: Compilar y flashear ambos equipos
   - Verificar que ambos usan mismos valores (logs/debug)
3. **Día 3**: Implementar Solución 2A (validación en sniffer)
4. **Día 4-5**: Test completo Fase 1 (TEST-01 + TEST-02)

**Objetivo Fase 1**: Lograr detección estable a 25m

**⚠️ IMPORTANTE**: No hacer tests parciales con solo un equipo modificado

---

### Fase 2: Optimizaciones (Semana 2)
1. **Día 6-7**: Implementar Solución 5A (logging completo)
2. **Día 8-9**: Implementar Solución 3A o 3B (query_timeout)
3. **Día 10**: Test completo Fase 2 (TEST-03 + TEST-05)

**Objetivo Fase 2**: Lograr detección estable a 30m con datos de diagnóstico

---

### Fase 3: Refinamiento (Semana 3)
1. **Día 11-13**: Implementar Solución 4A (alternancia inteligente) - **Opcional**
2. **Día 14**: Implementar Solución 6A (eliminar ONE_DETECTION) - **Opcional**
3. **Día 15**: Test completo Fase 3

**Objetivo Fase 3**: Optimizar eficiencia y robustez

---

### Fase 4: Valores Agresivos (Solo si fases previas no alcanzan 30m)
1. Implementar Solución 1B (timeouts +100%) en **ambos equipos**
2. Flashear **ambos equipos**
3. Test completo (TEST-01B)

---

## Verificación de Sincronización de Timeouts

### ⚠️ CRÍTICO: Antes de TEST-01

Después de flashear ambos equipos con Solución 1A o 1B, **VERIFICAR** que los timeouts coinciden:

#### Método 1: Inspección de código compilado
```bash
# En directorio sniffer
grep -n "POLL_TX_TO_RESP_RX_DLY_UUS_6M8" Core/Inc/uwb3000Fxx.h
grep -n "RESP_RX_TIMEOUT_UUS_6M8" Core/Inc/uwb3000Fxx.h
grep -n "PRE_TIMEOUT_6M8" Core/Inc/uwb3000Fxx.h

# En directorio Persona
grep -n "POLL_TX_TO_RESP_RX_DLY_UUS_6M8" Core/Inc/uwb3000Fxx.h
grep -n "RESP_RX_TIMEOUT_UUS_6M8" Core/Inc/uwb3000Fxx.h
grep -n "PRE_TIMEOUT_6M8" Core/Inc/uwb3000Fxx.h
```

**Valores esperados Solución 1A**:
- Sniffer: 1000, 500, 8
- Persona: 1000, 500, 8 ✅ DEBEN COINCIDIR

**Valores esperados Solución 1B**:
- Sniffer: 1400, 700, 12
- Persona: 1400, 700, 12 ✅ DEBEN COINCIDIR

#### Método 2: Agregar debug al inicio (opcional pero recomendado)

```c
// Agregar en sniffer/Core/Src/main.cpp después de uwb_init()
debug_printf("=== TIMEOUTS SNIFFER ===\n");
debug_printf("POLL_TX_TO_RESP_RX_DLY: %d\n", POLL_TX_TO_RESP_RX_DLY_UUS_6M8);
debug_printf("RESP_RX_TIMEOUT: %d\n", RESP_RX_TIMEOUT_UUS_6M8);
debug_printf("PRE_TIMEOUT: %d\n", PRE_TIMEOUT_6M8);
debug_printf("=======================\n");

// Agregar en Persona/Core/Src/main.cpp después de uwb_init()
debug_printf("=== TIMEOUTS TAG ===\n");
debug_printf("POLL_TX_TO_RESP_RX_DLY: %d\n", POLL_TX_TO_RESP_RX_DLY_UUS_6M8);
debug_printf("RESP_RX_TIMEOUT: %d\n", RESP_RX_TIMEOUT_UUS_6M8);
debug_printf("PRE_TIMEOUT: %d\n", PRE_TIMEOUT_6M8);
debug_printf("====================\n");
```

Al encender equipos, los logs deberían mostrar **valores idénticos**.

#### Método 3: Test de compatibilidad rápido (1 minuto)

1. Colocar tag a 10m (distancia conocida como funcional)
2. Ejecutar 10 ciclos de ranging
3. **Si falla al 100%** → Timeouts desincronizados, revisar valores
4. **Si funciona correctamente** → Sincronización OK, proceder con TEST-01

---

## Protocolo de Pruebas Físicas

### TEST-01: Validación de Timeouts Aumentados

**Objetivo**: Verificar que timeouts aumentados permiten detección a >20m

**Requisitos**:
- 1 Sniffer programado con Solución 1A
- 1 Tag (Persona) programado con Solución 1A **con valores idénticos**
- ⚠️ **AMBOS deben tener los mismos timeouts o el protocolo fallará completamente**
- Cinta métrica o sistema de medición de distancia
- Cable USB para logs (UART)
- Batería completa en ambos equipos

**Procedimiento**:
1. **Configuración inicial**:
   - Montar sniffer en trípode a 3m altura
   - Orientar antenas A (izquierda) y B (derecha) con separación 2m
   - Encender tag y colocarlo a 10m (baseline)

2. **Test progresivo de distancia**:
   ```
   Distancia | Lecturas esperadas | Criterio éxito
   ----------|-------------------|----------------
   10m       | 100% ambas antenas | ✅ Baseline
   15m       | 100% ambas antenas | ✅ Control
   20m       | 90%+ ambas antenas | ✅ Límite actual
   22m       | 80%+ ambas antenas | ✅ Mejora esperada
   25m       | 70%+ ambas antenas | ✅ Target Fase 1
   27m       | 60%+ ambas antenas | ✅ Objetivo extendido
   30m       | 50%+ ambas antenas | ✅ Target Fase 2
   ```

3. **Captura de datos** (por cada distancia, 3 minutos):
   - Conectar UART del sniffer
   - Capturar logs completos
   - Contar lecturas exitosas vs fallidas
   - Identificar tipo de falla (preamble/frame/no_rxcg)

4. **Criterios de éxito**:
   - ✅ EXITOSO: A 25m, ≥70% de intentos tienen lecturas en AMBAS antenas
   - ⚠️ PARCIAL: A 25m, 50-70% exitoso → proceder con Solución 1B
   - ❌ FALLIDO: <50% a 25m → revisar hardware/configuración RF

5. **Datos a registrar**:
   ```
   Distancia: ___m
   Temperatura: ___°C
   Humedad: ___%
   Lecturas antena A: ___/___
   Lecturas antena B: ___/___
   Timeouts preámbulo: ___
   Timeouts frame: ___
   Timeouts no_rxcg: ___
   Errores CRC: ___
   Tiempo promedio por lectura: ___ms
   ```

6. **Repeticiones**: 
   - Mínimo 3 sesiones en diferentes momentos del día
   - Probar en interiores y exteriores

**Documentación**: Llenar plantilla TEST-01-RESULTADOS.csv

---

### TEST-01B: Timeouts Agresivos (Solo si TEST-01 falla)

**Mismo procedimiento que TEST-01**, pero con Solución 1B implementada.

**Criterio de activación**: Si TEST-01 logra <70% a 25m

---

### TEST-02: Validación de Guardado Correcto

**Objetivo**: Confirmar que solo se guardan tags con datos completos de ambas antenas

**Requisitos**:
- Sniffer con Solución 1A + Solución 2A implementadas
- Tag estándar
- Setup de TEST-01

**Procedimiento**:
1. **Escenario 1: Distancia óptima (15m)**:
   - Ejecutar 20 ciclos de ranging
   - Verificar que 100% de tags guardados tienen counter_a > 0 AND counter_b > 0
   - Revisar logs: NO debe haber mensajes "Tag descartado"

2. **Escenario 2: Distancia límite (28m)**:
   - Ejecutar 30 ciclos de ranging
   - Contar tags guardados vs descartados
   - Verificar que tags descartados realmente tenían datos incompletos
   - Revisar logs: mensajes "Tag descartado" deben indicar counters reales

3. **Escenario 3: Obstrucción intencional**:
   - Colocar tag a 20m
   - Bloquear antena B con metal (reflector)
   - Ejecutar 10 ciclos
   - Verificar que tags NO se guardan (solo antena A tiene datos)
   - Confirmar log: "A=X lecturas, B=0 lecturas"

**Criterios de éxito**:
- ✅ Escenario 1: 100% de guardados tienen ambas antenas
- ✅ Escenario 2: 0% de guardados tienen solo una antena
- ✅ Escenario 3: 100% descartados correctamente

**Documentación**: Llenar plantilla TEST-02-RESULTADOS.csv

---

### TEST-03: Validación de query_timeout Adaptativo

**Objetivo**: Confirmar que timeout adaptativo permite completar más lecturas sin desperdiciar tiempo

**Requisitos**:
- Sniffer con Soluciones 1A + 2A + 3A
- Tag estándar
- Configurar lecturas totales = 10

**Procedimiento**:
1. **Configuración A: timeout fijo 1000ms (baseline)**:
   - Colocar tag a 25m
   - Ejecutar 20 ciclos
   - Medir: tiempo promedio por ciclo, % de ciclos completos

2. **Configuración B: timeout adaptativo (10 lecturas × 150ms = 2500ms)**:
   - Mismas condiciones
   - Ejecutar 20 ciclos
   - Medir: tiempo promedio por ciclo, % de ciclos completos

**Criterios de éxito**:
- ✅ Config B debe tener ≥20% más ciclos completos que Config A
- ✅ Config B no debe tomar >3000ms promedio por ciclo (validar eficiencia)

**Datos a comparar**:
```
Configuración | Tiempo/ciclo | Ciclos completos | Lecturas totales
--------------|--------------|------------------|------------------
Fijo 1000ms   | ___ms        | ___/20           | ___
Adaptativo    | ___ms        | ___/20           | ___
Mejora        | ___%         | ___%             | ___%
```

**Documentación**: Llenar plantilla TEST-03-RESULTADOS.csv

---

### TEST-04: Alternancia Inteligente vs Mecánica

**Objetivo**: Comparar eficiencia de alternancia basada en RSSI vs mecánica

**Requisitos**:
- Sniffer con Soluciones 1A + 2A + 4A
- Tag estándar
- Setup asimétrico: tag NO en línea central entre antenas (ej: 20m de A, 25m de B)

**Procedimiento**:
1. **Configuración baseline: alternancia mecánica**:
   - Colocar tag a 20m de antena A, 25m de antena B
   - Ejecutar 30 ciclos
   - Registrar: lecturas exitosas por antena, tiempo total

2. **Configuración mejorada: alternancia por RSSI**:
   - Misma posición
   - Ejecutar 30 ciclos
   - Registrar: lecturas exitosas por antena, tiempo total

**Criterios de éxito**:
- ✅ Config mejorada debe priorizar antena A (más cercana)
- ✅ Config mejorada debe lograr ≥15% más lecturas exitosas en menos intentos

**Datos a comparar**:
```
Configuración     | Intentos A | Exitosos A | Intentos B | Exitosos B | Tiempo
------------------|------------|------------|------------|------------|--------
Mecánica          | ___        | ___        | ___        | ___        | ___ms
RSSI inteligente  | ___        | ___        | ___        | ___        | ___ms
```

**Documentación**: Llenar plantilla TEST-04-RESULTADOS.csv

---

### TEST-05: Logging Completo (ejecutar con TODOS los tests)

**Objetivo**: Validar que logs proveen información útil para debugging

**Requisitos**:
- Sniffer con Solución 5A implementada
- Cable USB conectado a UART
- Software de captura serial (PuTTY, CoolTerm, etc)

**Procedimiento**:
1. Ejecutar cualquier otro test (TEST-01, TEST-02, etc)
2. Capturar logs completos durante todo el test
3. Al finalizar, analizar logs para:
   - Contar tipos de timeout por antena
   - Identificar patrones (ej: antena B siempre falla primero)
   - Verificar que logs incluyen: tag_id, antena, tipo error, tiempo

**Criterios de éxito**:
- ✅ Logs permiten reconstruir secuencia de eventos
- ✅ Logs muestran diferencia entre antenas A y B
- ✅ Logs identifican cuello de botella (preamble vs frame timeout)

**Ejemplo log esperado**:
```
[A] Tag 12345678: RX OK, lecturas=1, tiempo=45ms
[B] Tag 12345678: PREAMBULO TIMEOUT (señal débil?)
[A] Tag 12345678: RX OK, lecturas=2, tiempo=47ms
[B] Tag 12345678: FRAME TIMEOUT (ventana cerró)
[A] Tag 12345678: RX OK, lecturas=3, tiempo=46ms
[B] Tag 12345678: RX OK, lecturas=1, tiempo=89ms
Tag 12345678 descartado: A=3 lecturas, B=1 lecturas
```

**Documentación**: Anexar logs completos a cada test

---

### TEST-06: Modo MULTIPLE_DETECTION Forzado

**Objetivo**: Verificar que eliminar TAG_ONE_DETECTION mejora triangulación

**Requisitos**:
- Sniffer con Solución 6A (modo ONE deshabilitado)
- Tag estándar

**Procedimiento**:
1. **Baseline: código original con modo ONE activo**:
   - Configurar `MASTER_ONE_DETECTION`
   - Colocar tag a 15m
   - Ejecutar 20 ciclos
   - Registrar: tags guardados, datos de triangulación disponibles

2. **Mejorado: solo modo MULTIPLE**:
   - Mismo setup
   - Ejecutar 20 ciclos
   - Registrar: tags guardados, datos de triangulación disponibles

**Criterios de éxito**:
- ✅ Modo MULTIPLE debe generar 100% de tags con datos de ambas antenas
- ✅ Modo ONE baseline debe tener tags con solo 1 antena

**Datos a comparar**:
```
Configuración | Tags guardados | Tags con ambas antenas | Tags con 1 antena
--------------|----------------|------------------------|-------------------
ONE activo    | ___            | ___                    | ___
Solo MULTIPLE | ___            | ___                    | 0 (esperado)
```

**Documentación**: Llenar plantilla TEST-06-RESULTADOS.csv

---

## Plan de Rollback

### Si las correcciones empeoran el sistema:

1. **Rollback Solución 1A/1B** (timeouts):
   - Restaurar valores originales en **ambos** archivos:
     * `sniffer/Core/Inc/uwb3000Fxx.h` → 700, 300, 5
     * `Persona/Core/Inc/uwb3000Fxx.h` → 700, 300, 5
   - Recompilar **ambos** proyectos
   - Flashear **ambos** equipos
   - ⚠️ **No dejar equipos con versiones mixtas en producción**

2. **Rollback Solución 2A** (validación):
   - Comentar bloque if de validación en `sniffer/Core/Src/main.cpp`
   - Restaurar guardado directo
   - Solo requiere reflashear Sniffer

3. **Rollback Solución 3A** (query_timeout):
   - Restaurar `const uint32_t query_timeout = 1000;`
   - Solo requiere reflashear Sniffer

4. **Control de versiones**:
   - Hacer commit ANTES de cada cambio
   - Tag commits: `test-01-pre`, `test-01-post`, etc
   - **Etiquetar versiones de firmware**: `sniffer-v0.4.0`, `persona-v0.4.0`
   - Mantener registro de qué equipos físicos tienen qué versión

---

## Métricas de Éxito del Proyecto

### Objetivos Mínimos (Fase 1)
- ✅ Detección estable a 25m con ambas antenas: ≥70% éxito
- ✅ 0% de tags guardados con datos incompletos
- ✅ Logs detallados de todos los fallos

### Objetivos Deseables (Fase 2)
- ✅ Detección estable a 30m: ≥50% éxito
- ✅ Timeout adaptativo funcional
- ✅ Reducción de 30% en intentos fallidos

### Objetivos Extendidos (Fase 3)
- ✅ Alternancia inteligente por RSSI
- ✅ Sistema robusto hasta 35m (opcional)

---

## Checklist de Implementación

### Antes de empezar:
- [ ] Backup completo del código actual
- [ ] Crear branch `fix/detection-over-20m`
- [ ] Configurar entorno de logging (UART funcional)
- [ ] Calibrar cinta métrica o sistema de medición
- [ ] Cargar completamente baterías de todos los equipos

### Por cada solución:
- [ ] Leer código actual completo
- [ ] Implementar cambio en archivo correcto
- [ ] **Si es Solución 1A/1B: Modificar AMBOS equipos (sniffer + Persona)**
- [ ] Compilar sin warnings (ambos proyectos si aplica)
- [ ] Flashear sniffer si aplica
- [ ] Flashear tag (Persona) si aplica
- [ ] **Verificar sincronización de timeouts en logs de ambos equipos**
- [ ] Ejecutar test específico
- [ ] Documentar resultados
- [ ] Commit con mensaje descriptivo
- [ ] Tag git antes de siguiente cambio

### Al finalizar:
- [ ] Merge a branch dev si todo es exitoso
- [ ] Actualizar CHANGELOG.md
- [ ] Documentar valores finales en README
- [ ] Crear release tag `v0.4.0-fix-distance`

---

## Notas Finales

### Consideraciones de Hardware
- **Antenas**: Verificar que no tengan daño físico o desconexión
- **Alimentación**: Voltaje bajo puede reducir potencia de transmisión
- **Temperatura**: Extremos (muy frío/caliente) afectan rendimiento del chip

### Consideraciones Ambientales
- **Interiores**: Multipath puede ayudar o perjudicar según geometría
- **Exteriores**: Menos interferencia pero mayor atenuación por distancia
- **Obstáculos**: Personas, muebles, paredes metálicas bloquean UWB

### Próximos Pasos Después del Fix
1. Implementar power control adaptativo (transmit power según distancia)
2. Agregar sistema de calibración automática de timeouts
3. Implementar TDMA si hay múltiples tags simultáneos
4. Optimizar consumo energético con sleep más agresivo

---

**Documento creado**: 2025-10-22  
**Versión**: 1.0  
**Autor**: Análisis basado en documentación técnica completa  
**Próxima revisión**: Después de completar Fase 1
