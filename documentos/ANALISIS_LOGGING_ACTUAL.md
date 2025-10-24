# Análisis del Sistema de Logging Actual del Sniffer

## Fecha: 2025-10-22
## Estado: Sistema básico funcional pero insuficiente para debugging avanzado

---

## 1. Sistema de Logging Actual

### 1.1 Función Principal: `debug_distance_new()`

**Ubicación**: `sniffer/Core/Src/sniffer_tag.cpp` líneas 594-623

**Firma**:
```cpp
void debug_distance_new(TAG_t tag, TAG_STATUS_t status, DistanceHandler d_a, DistanceHandler d_b)
```

**Formato de salida**:
```
{message: STATUS},{ID: 0xXXXXXXXX},{readings: N},{error_track_a:X},{error_track_b:X},
{Counter_a: X},{Counter_b: X},{distance_a: X.XX},{distance_b: X.XX},
{error_crc_a:X},{error_crc_b:X},{battery_voltage_INT: X}
```

**Ejemplo real**:
```
{message: SEND_TIMESTAMP_QUERY},{ID: 0x12345678},{readings: 5},{error_track_a:2},{error_track_b:3},
{Counter_a: 3},{Counter_b: 2},{distance_a: 24.56},{distance_b: 25.12},
{error_crc_a:1},{error_crc_b:2},{battery_voltage_INT: 3300}
```

---

## 2. Mensajes de Estado Disponibles

**Definición**: `sniffer/Core/Src/sniffer_tag.cpp` líneas 10-14

```cpp
static const char *TAG_MESSAGES[] = {
    "NO_RESPONSE",                      // 0
    "NO_RXCG_DETECTED",                 // 1
    "RX_FRAME_TIMEOUT",                 // 2
    "RX_PREAMBLE_DETECTION_TIMEOUT",    // 3
    "RX_CRC_VALID",                     // 4
    "RX_ERROR",                         // 5
    "RX_DATA_ZERO",                     // 6
    "RX_NO_COMMAND",                    // 7
    "TX_ERROR",                         // 8
    "HUMAN_DISTANCE_OK",                // 9
    "END_READINGS",                     // 10
    "DISCOVERY",                        // 11
    "SEND_TIMESTAMP_QUERY",             // 12
    "TAG_SET_SLEEP_MODE",               // 13
    "TAG_ONE_DETECTION",                // 14
    "UNKNOWN"                           // 15
};
```

---

## 3. Puntos Donde se Llama `debug_distance_new()`

### En `main.cpp`:

1. **Línea 521**: Después de `TAG_END_READINGS`
   ```cpp
   debug_distance_new(tag, tag_status, distance_a, distance_b);
   save_two_maps_and_clear_tag(...);
   ```

2. **Línea 532**: Cuando comando no reconocido
   ```cpp
   debug_distance_new(tag, tag_status, distance_a, distance_b);
   save_two_maps_and_clear_tag(...);
   ```

3. **Línea 544**: En timeout de query_timeout
   ```cpp
   debug_distance_new(tag, tag_status, distance_a, distance_b);
   save_two_maps_and_clear_tag(...);
   ```

**Problema**: Solo se llama cuando se va a guardar o descartar el tag, **NO durante cada intento de RX**.

---

## 4. Análisis de `wait_rx_data()`

**Ubicación**: `sniffer/Core/Src/sniffer_tag.cpp` líneas 535-569

**Funcionamiento**:
```cpp
TAG_STATUS_t wait_rx_data() {
    uint32_t start_time = HAL_GetTick();
    
    while ((HAL_GetTick() - start_time) < RX_TIMEOUT_MS) {  // 100ms
        status_reg = dwt_read32bitreg(SYS_STATUS_ID);
        
        if (status_reg & SYS_STATUS_ALL_RX_ERR)
            return TAG_RX_ERROR;
        
        if (status_reg & SYS_STATUS_RXFTO_BIT_MASK)
            return TAG_RX_FRAME_TIMEOUT;
        
        if (status_reg & SYS_STATUS_RXPTO_BIT_MASK)
            return TAG_RX_PREAMBLE_DETECTION_TIMEOUT;
        
        if (status_reg & SYS_STATUS_RXFCG_BIT_MASK)
            return TAG_RX_CRC_VALID;
    }
    
    return TAG_NO_RXCG_DETECTED;  // Timeout 100ms sin evento
}
```

**Estado actual**: ❌ **NO hay logging dentro de `wait_rx_data()`**

---

## 5. Problemas Identificados

### 🔴 Problema 1: Sin Logging por Intento Individual
- `wait_rx_data()` retorna códigos de error pero **no los registra**
- No se sabe cuántos intentos fallidos ocurren por antena
- No se identifica qué antena (A o B) tuvo el error

### 🔴 Problema 2: Sin Identificación de Antena
- Los logs actuales **no indican** si el error fue en antena A o B
- Imposible diagnosticar asimetría entre antenas

### 🔴 Problema 3: Sin Timestamp de Intentos
- No se registra cuánto tiempo tomó cada intento
- No se puede analizar si timeouts ocurren cerca del límite (100ms)

### 🔴 Problema 4: Sin Desglose de Tipos de Timeout
- `error_track_a` y `error_track_b` son contadores genéricos
- No distinguen entre:
  - Timeout de preámbulo
  - Timeout de frame
  - No RXCG detectado
  - Error CRC

### 🔴 Problema 5: Logging Solo en Puntos Finales
- Se llama `debug_distance_new()` solo cuando:
  1. Se completan lecturas (`END_READINGS`)
  2. Hay comando no reconocido
  3. Expira `query_timeout`
- **NO se registra cada intento de query individual**

---

## 6. Lo Que se Necesita para TEST-05 (Solución 5A)

Según el **PLAN_CORRECCION_DISTANCIA.md**, se requiere:

### Función `log_rx_result()`:
```cpp
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

**Llamar después de cada `wait_rx_data()`** en el loop de queries.

---

## 7. Gaps Actuales vs Necesidades

| Característica | Estado Actual | Necesidad | Gap |
|----------------|---------------|-----------|-----|
| Identificar antena | ❌ No | ✅ Sí (A/B) | **CRÍTICO** |
| Logging por intento | ❌ No | ✅ Sí | **CRÍTICO** |
| Tipo de timeout específico | ❌ No | ✅ Sí | **ALTO** |
| Tiempo por intento | ❌ No | ✅ Sí | **MEDIO** |
| Contador de lecturas | ✅ Sí | ✅ Sí | OK |
| Tag ID | ✅ Sí | ✅ Sí | OK |
| Distancias finales | ✅ Sí | ✅ Sí | OK |
| Error CRC por antena | ✅ Sí (parcial) | ✅ Sí | OK |

---

## 8. Cambios Necesarios

### 8.1 Crear función `log_rx_result()`

**Ubicación**: `sniffer/Core/Src/sniffer_tag.cpp` (agregar después de línea 593)

```cpp
void log_rx_result(uint8_t result, TAG_t *tag, Uwb_HW_t *hw, 
                   DistanceHandler *dist_ptr, uint32_t elapsed_ms) {
    const char* hw_name = (hw == &uwb_hw_a) ? "A" : "B";
    char log_buffer[150];
    
    switch(result) {
        case TAG_RX_CRC_VALID:
            snprintf(log_buffer, sizeof(log_buffer),
                    "[%s] Tag %08X: RX OK, lecturas=%d, tiempo=%lums\r\n", 
                    hw_name, (unsigned int)tag->id, dist_ptr->get_counter(), elapsed_ms);
            break;
            
        case TAG_RX_PREAMBLE_DETECTION_TIMEOUT:
            snprintf(log_buffer, sizeof(log_buffer),
                    "[%s] Tag %08X: PREAMBULO TIMEOUT (señal débil?)\r\n", 
                    hw_name, (unsigned int)tag->id);
            break;
            
        case TAG_RX_FRAME_TIMEOUT:
            snprintf(log_buffer, sizeof(log_buffer),
                    "[%s] Tag %08X: FRAME TIMEOUT (ventana cerró)\r\n", 
                    hw_name, (unsigned int)tag->id);
            break;
            
        case TAG_NO_RXCG_DETECTED:
            snprintf(log_buffer, sizeof(log_buffer),
                    "[%s] Tag %08X: NO_RXCG (100ms sin evento)\r\n", 
                    hw_name, (unsigned int)tag->id);
            break;
            
        case TAG_RX_ERROR:
            snprintf(log_buffer, sizeof(log_buffer),
                    "[%s] Tag %08X: CRC ERROR\r\n", 
                    hw_name, (unsigned int)tag->id);
            break;
            
        default:
            snprintf(log_buffer, sizeof(log_buffer),
                    "[%s] Tag %08X: UNKNOWN ERROR %d\r\n", 
                    hw_name, (unsigned int)tag->id, result);
            break;
    }
    
    HAL_UART_Transmit(&huart1, (uint8_t*)log_buffer, strlen(log_buffer), HAL_MAX_DELAY);
}
```

### 8.2 Declarar en header

**Ubicación**: `sniffer/Core/Inc/sniffer_tag.hpp` (agregar después de línea 211)

```cpp
void log_rx_result(uint8_t result, TAG_t *tag, Uwb_HW_t *hw, 
                   DistanceHandler *dist_ptr, uint32_t elapsed_ms);
```

### 8.3 Llamar en `main.cpp` después de cada query

**Ubicación**: `sniffer/Core/Src/main.cpp` aproximadamente línea 480-490

**ANTES** (sin logging por intento):
```cpp
tag_status = send_timestamp_query(&tag, hw, &rx_buffer_size, &distance_a, &distance_b);
if (tag_status == TAG_RX_CRC_VALID)
{
    // procesar respuesta...
}
```

**DESPUÉS** (con logging):
```cpp
uint32_t query_start = HAL_GetTick();
tag_status = send_timestamp_query(&tag, hw, &rx_buffer_size, &distance_a, &distance_b);
uint32_t query_elapsed = HAL_GetTick() - query_start;

// LOG del resultado inmediatamente
log_rx_result(tag_status, &tag, hw, distance_ptr, query_elapsed);

if (tag_status == TAG_RX_CRC_VALID)
{
    // procesar respuesta...
}
```

### 8.4 Agregar log en timeout de query_timeout

**Ubicación**: `sniffer/Core/Src/main.cpp` línea 542-546

**ANTES**:
```cpp
if (HAL_GetTick() - query_ticks > query_timeout)
{
    debug_distance_new(tag, tag_status, distance_a, distance_b);
    save_two_maps_and_clear_tag(distance_a, distance_b, tag_map_od, tag, lora_send_ticks, tag_map);
    tag_status = TAG_DISCOVERY;
}
```

**DESPUÉS** (con mensaje de descarte si faltan datos):
```cpp
if (HAL_GetTick() - query_ticks > query_timeout)
{
    debug_distance_new(tag, tag_status, distance_a, distance_b);
    
    // LOG adicional para timeout
    char timeout_log[100];
    snprintf(timeout_log, sizeof(timeout_log),
            "TIMEOUT: Tag %08X - A=%d lecturas, B=%d lecturas\r\n",
            (unsigned int)tag.id, distance_a.get_counter(), distance_b.get_counter());
    HAL_UART_Transmit(&huart1, (uint8_t*)timeout_log, strlen(timeout_log), HAL_MAX_DELAY);
    
    save_two_maps_and_clear_tag(distance_a, distance_b, tag_map_od, tag, lora_send_ticks, tag_map);
    tag_status = TAG_DISCOVERY;
}
```

---

## 9. Ejemplo de Salida Mejorada

### Antes (logging actual):
```
{message: SEND_TIMESTAMP_QUERY},{ID: 0x12345678},{readings: 3},{error_track_a:5},{error_track_b:8},
{Counter_a: 3},{Counter_b: 1},{distance_a: 24.56},{distance_b: 25.12},{error_crc_a:1},{error_crc_b:2},{battery_voltage_INT: 3300}
```

### Después (con Solución 5A):
```
[A] Tag 12345678: RX OK, lecturas=1, tiempo=45ms
[B] Tag 12345678: PREAMBULO TIMEOUT (señal débil?)
[A] Tag 12345678: RX OK, lecturas=2, tiempo=47ms
[B] Tag 12345678: FRAME TIMEOUT (ventana cerró)
[A] Tag 12345678: RX OK, lecturas=3, tiempo=46ms
[B] Tag 12345678: NO_RXCG (100ms sin evento)
[A] Tag 12345678: RX OK, lecturas=4, tiempo=44ms
[B] Tag 12345678: PREAMBULO TIMEOUT (señal débil?)
TIMEOUT: Tag 12345678 - A=4 lecturas, B=0 lecturas
{message: SEND_TIMESTAMP_QUERY},{ID: 0x12345678},{readings: 4},{error_track_a:0},{error_track_b:4},
{Counter_a: 4},{Counter_b: 0},{distance_a: 24.56},{distance_b: 0.00},{error_crc_a:0},{error_crc_b:0},{battery_voltage_INT: 3300}
```

**Beneficios**:
1. ✅ Se ve claramente que antena B falla consistentemente
2. ✅ Se identifica tipo de timeout específico (preámbulo, frame, no_rxcg)
3. ✅ Se mide tiempo por intento
4. ✅ Se mantiene compatibilidad con logging existente

---

## 10. Impacto en Rendimiento

### Overhead Estimado:
- `snprintf()`: ~50-100 µs
- `HAL_UART_Transmit()` con 100 bytes @ 115200 baud: ~8.7 ms
- **Total por log**: ~9 ms

### Frecuencia:
- 1 log por query (típicamente 10-20 queries por tag)
- Máximo: 20 logs por tag × 9ms = **180 ms adicionales**

### Impacto en query_timeout:
- Con timeout actual de 1000ms, agregar 180ms = **18% overhead**
- **Recomendación**: Aumentar `query_timeout` a 1200-1500ms si se activa logging detallado
- O implementar logging condicional (solo cuando `DEBUG_VERBOSE` está definido)

---

## 11. Implementación Condicional (Recomendada)

Para no impactar producción, usar compilación condicional:

```cpp
// En sniffer/Core/Inc/main.h
#define DEBUG_VERBOSE  // Comentar para deshabilitar logging detallado

// En sniffer/Core/Src/main.cpp
#ifdef DEBUG_VERBOSE
    log_rx_result(tag_status, &tag, hw, distance_ptr, query_elapsed);
#endif
```

**Ventaja**: Se puede deshabilitar para producción sin cambiar código.

---

## 12. Resumen de Cambios Necesarios

### ✅ Cambios Obligatorios para TEST-05:

1. **Crear** `log_rx_result()` en `sniffer_tag.cpp`
2. **Declarar** en `sniffer_tag.hpp`
3. **Llamar** después de cada `send_timestamp_query()` en `main.cpp`
4. **Agregar** log de timeout cuando expira `query_timeout`
5. **Opcional**: Implementar `#ifdef DEBUG_VERBOSE` para control

### 📁 Archivos a Modificar:

1. `sniffer/Core/Src/sniffer_tag.cpp` (agregar función)
2. `sniffer/Core/Inc/sniffer_tag.hpp` (declaración)
3. `sniffer/Core/Src/main.cpp` (llamadas en loop de queries)
4. `sniffer/Core/Inc/main.h` (opcional: flag DEBUG_VERBOSE)

### ⏱️ Tiempo Estimado de Implementación:
- 30-45 minutos de código
- 15-30 minutos de testing
- **Total**: ~1 hora

---

## 13. Conclusión

### Estado Actual:
- ❌ Sistema de logging **insuficiente** para debugging detallado
- ❌ No identifica antena en errores
- ❌ No registra intentos individuales
- ❌ No distingue tipos de timeout

### Acción Requerida:
- ✅ Implementar **Solución 5A** completa según plan
- ✅ Es **esencial** para validar otras correcciones (TEST-01, TEST-02, etc)
- ✅ Cambios mínimos, impacto alto en capacidad de diagnóstico

### Prioridad:
**ALTA** - Sin esto, es imposible validar si las correcciones de timeouts están funcionando correctamente.

---

**Documento creado**: 2025-10-22  
**Versión**: 1.0  
**Próxima acción**: Implementar cambios en código del sniffer
