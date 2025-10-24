# Protocolo UWB Completo - Sniffer ↔ Tag (Persona)

## Versión: 1.0 (2025-10-22)
## Basado en: Código fuente real verificado

---

## 1. Configuración de Timeouts

### 1.1 Timeouts Físicos (Chip DW3000)

| Parámetro | Valor | Unidad | Conversión | Propósito |
|-----------|-------|--------|------------|-----------|
| `POLL_TX_TO_RESP_RX_DLY_UUS_6M8` | 700 | UUS | ~718 µs | Retardo entre fin TX y apertura RX |
| `RESP_RX_TIMEOUT_UUS_6M8` | 300 | UUS | ~308 µs | Duración ventana espera frame |
| `PRE_TIMEOUT_6M8` | 5 | símbolos | variable | Tiempo detección preámbulo |

**Conversión:** 1 UUS = 1.0256 µs

**Configuración en código:**
```c
// uwb3000Fxx.c líneas 111-114 (sniffer y tag)
if (rate == RATE_6M8) {
    dwt_setrxaftertxdelay(POLL_TX_TO_RESP_RX_DLY_UUS_6M8);
    dwt_setrxtimeout(RESP_RX_TIMEOUT_UUS_6M8);
    dwt_setpreambledetecttimeout(PRE_TIMEOUT_6M8);
}
```

### 1.2 Timeouts de Aplicación

| Parámetro | Sniffer | Tag | Unidad | Propósito |
|-----------|---------|-----|--------|-----------|
| `query_timeout` | 1000 | 1000 | ms | Tiempo total para completar ranging de un tag |
| `RX_TIMEOUT_MS` | 100 | N/A | ms | Timeout bucle sondeo estado RX (solo sniffer) |
| `sleep_time_not_recived` | N/A | 500 | ms | Sleep si no recibe queries |
| `sleep_time_recived` | N/A | 15000 | ms | Sleep tras ranging exitoso |
| `sleep_time` | N/A | 1000 | ms | Sleep genérico |

**Ubicación en código:**
- Sniffer: `main.cpp` línea 347
- Tag: `main.cpp` líneas 310-312, 318
- RX_TIMEOUT_MS: `sniffer_tag.cpp` línea 532

---

## 2. Estados de Máquina

### 2.1 Estados Sniffer

| Estado | Valor Enum | Transiciones Posibles | Descripción |
|--------|------------|----------------------|-------------|
| `TAG_DISCOVERY` | 11 | → TAG_SEND_TIMESTAMP_QUERY<br>→ TAG_ONE_DETECTION<br>→ TAG_DISCOVERY | Busca tags, envía poll discovery |
| `TAG_SEND_TIMESTAMP_QUERY` | 12 | → TAG_END_READINGS<br>→ TAG_SEND_TIMESTAMP_QUERY<br>→ TAG_DISCOVERY | Realiza ranging alternando antenas |
| `TAG_END_READINGS` | 10 | → TAG_DISCOVERY | Finaliza lecturas, guarda datos |
| `TAG_ONE_DETECTION` | 14 | → TAG_DISCOVERY | Modo especial un solo tag |
| `TAG_RX_CRC_VALID` | 4 | - | RX exitoso |
| `TAG_RX_NO_COMMAND` | 7 | → TAG_DISCOVERY | Comando no reconocido |
| `TAG_RX_ERROR` | 5 | → TAG_SEND_TIMESTAMP_QUERY | Error CRC u otro |
| `TAG_RX_FRAME_TIMEOUT` | 2 | → reintentar | Frame no llegó |
| `TAG_RX_PREAMBLE_DETECTION_TIMEOUT` | 3 | → reintentar | Preámbulo no detectado |
| `TAG_NO_RXCG_DETECTED` | 1 | → reintentar | Sin evento RX en RX_TIMEOUT_MS |
| `TAG_TX_ERROR` | 8 | → TAG_DISCOVERY | Error al transmitir |

### 2.2 Estados Tag (Persona)

| Estado | Transiciones | Descripción |
|--------|-------------|-------------|
| `TAG_WAIT_FOR_FIRST_DETECTION` | → TAG_WAIT_FOR_TIMESTAMPT_QUERY<br>→ TAG_SLEEP | Espera discovery del sniffer |
| `TAG_WAIT_FOR_TIMESTAMPT_QUERY` | → TAG_SLEEP<br>→ TAG_SLEEP_RECIVED | Espera queries de ranging |
| `TAG_SLEEP` | → TAG_WAIT_FOR_FIRST_DETECTION | Sleep sin lecturas completas |
| `TAG_SLEEP_RECIVED` | → TAG_WAIT_FOR_FIRST_DETECTION | Sleep con lecturas exitosas |
| `TAG_SHIP_MODE_SET` | - | Modo envío/apagado profundo |

---

## 3. Comandos del Protocolo

| Comando | Valor | Dirección | Payload | Respuesta |
|---------|-------|-----------|---------|-----------|
| `TAG_ID_QUERY` | 0x?? | Sniffer → Tag | [cmd][sniffer_state][sleep_time][dev_type] | Discovery Response |
| `TAG_TIMESTAMP_QUERY` | 0x?? | Sniffer → Tag | [cmd][tag_id(4)][dist_a(2)][dist_b(2)] | Timestamp Response |
| `TAG_SET_SLEEP_MODE` | 0x?? | Sniffer → Tag | [cmd][tag_id(4)][dist_a(2)][dist_b(2)] | Timestamp + ACK Sleep |

**Nota:** Valores hexadecimales exactos no encontrados como constantes en código, se infieren por uso.

---

## 4. Estructura de Frames

### 4.1 Discovery Request (Sniffer → Tag)

```
Offset | Bytes | Campo                | Tipo      | Descripción
-------|-------|----------------------|-----------|---------------------------
0      | 1     | command              | uint8_t   | TAG_ID_QUERY
1      | 1     | interfaz_state       | uint8_t   | MASTER_ONE/MULTIPLE_DETECTION
2      | 1     | sleep_time_recived   | uint8_t   | Tiempo sleep para tag
3      | 1     | device_type          | uint8_t   | DEV_UWB3000F27
-------|-------|----------------------|-----------|---------------------------
Total: 4 bytes + FCS
```

**Código:** `main.cpp` líneas 405-409

### 4.2 Discovery Response (Tag → Sniffer)

```
Offset | Bytes | Campo                | Tipo      | Descripción
-------|-------|----------------------|-----------|---------------------------
0      | 1     | command              | uint8_t   | TAG_ID_QUERY (echo)
1      | 4     | tag_id               | uint32_t  | ID único del tag
5      | 4     | poll_rx_timestamp    | uint32_t  | Timestamp RX del poll
9      | 4     | resp_tx_timestamp    | uint32_t  | Timestamp TX de respuesta
13     | 2     | battery_voltage      | uint16_t  | Voltaje batería (raw ADC)
-------|-------|----------------------|-----------|---------------------------
Total: 15+ bytes + FCS
```

**Offsets usados en sniffer:** `poll_rx_offset = 5`, `resp_tx_offset = 9` (líneas 430-431)

### 4.3 Timestamp Query Request (Sniffer → Tag)

```
Offset | Bytes | Campo                | Tipo      | Descripción
-------|-------|----------------------|-----------|---------------------------
0      | 1     | command              | uint8_t   | TAG_TIMESTAMP_QUERY o TAG_SET_SLEEP_MODE
1      | 4     | tag_id               | uint32_t  | ID del tag objetivo
5      | 2     | distance_a           | uint16_t  | Distancia antena A * 100
7      | 2     | distance_b           | uint16_t  | Distancia antena B * 100
-------|-------|----------------------|-----------|---------------------------
Total: 9 bytes + FCS
```

**Código:** `sniffer_tag.cpp` líneas 247-251

### 4.4 Timestamp Query Response (Tag → Sniffer)

```
Offset | Bytes | Campo                | Tipo      | Descripción
-------|-------|----------------------|-----------|---------------------------
0      | 1     | command              | uint8_t   | TAG_TIMESTAMP_QUERY (echo) o TAG_SET_SLEEP_MODE
1      | 4     | poll_rx_timestamp    | uint32_t  | Timestamp RX del query
5      | 4     | resp_tx_timestamp    | uint32_t  | Timestamp TX de respuesta
-------|-------|----------------------|-----------|---------------------------
Total: 9 bytes + FCS
```

**Offsets usados en sniffer:** `poll_rx_offset = 1`, `resp_tx_offset = 5` (líneas 492-493)

---

## 5. Secuencia Completa de Comunicación

### 5.1 Flujo Normal (Multiple Detection)

```
[SNIFFER]                                    [TAG]
    |                                           |
    |--- TX: Discovery Poll ------------------>|
    |    [TAG_ID_QUERY][state][sleep][dev]    | TAG_WAIT_FOR_FIRST_DETECTION
    |                                           |
    |<-- RX: Discovery Response ---------------|
    |    [cmd][id][ts_rx][ts_tx][batt]        |
    |                                           |
    | Calcula distancia inicial (antena A)     |
    | Cambia a antena B                        |
    |                                           |
    |--- TX: Timestamp Query 1 --------------->| TAG_WAIT_FOR_TIMESTAMPT_QUERY
    |    [TAG_TIMESTAMP_QUERY][id][dA][dB]    |
    |                                           |
    |<-- RX: Timestamp Response ---------------|
    |    [cmd][ts_rx][ts_tx]                  |
    |                                           |
    | Calcula distancia antena B               |
    | Alterna antena según contadores          |
    |                                           |
    |--- TX: Timestamp Query 2-N ------------->|
    |    (repite hasta completar lecturas)     |
    |                                           |
    |--- TX: Set Sleep Mode ------------------>|
    |    [TAG_SET_SLEEP_MODE][id][dA][dB]     |
    |                                           |
    |<-- RX: Final Timestamp + ACK ------------|
    |                                           |
    | Guarda tag con ambas distancias          | TAG_SLEEP_RECIVED (15s)
    |                                           |
    |--- Busca próximo tag ------------------->|
```

### 5.2 Flujo One Detection

```
[SNIFFER]                                    [TAG]
    |                                           |
    |--- TX: Discovery Poll ------------------>|
    |    [state = MASTER_ONE_DETECTION]       |
    |                                           |
    |<-- RX: Discovery Response ---------------|
    |                                           |
    | Calcula distancia (solo antena actual)   |
    |                                           |
    |--- TX: Set Sleep Mode ------------------>|
    |    (sin queries adicionales)             |
    |                                           |
    | Guarda tag (SIN triangulación)          | TAG_SLEEP
```

**⚠️ Problema:** En modo One Detection se pierde triangulación.

---

## 6. Alternancia de Antenas

### 6.1 Lógica en Discovery

```cpp
void switch_hw(TAG_t *tag, DistanceHandler *&dist_ptr, Uwb_HW_t *&hw,
               DistanceHandler *dist_a, DistanceHandler *dist_b) {
    if (hw == &uwb_hw_a) {
        hw = &uwb_hw_b;
        dist_ptr = dist_b;
    } else {
        hw = &uwb_hw_a;
        dist_ptr = dist_a;
    }
}
```

**Comportamiento:** Alterna A ↔ B cada vez que se llama (simple toggle).

### 6.2 Lógica en Timestamp Query

```cpp
void switch_hw_timestamp_query(TAG_t *tag, DistanceHandler *&dist_ptr,
                                Uwb_HW_t *&hw, DistanceHandler *dist_a, DistanceHandler *dist_b) {
    if ((hw == &uwb_hw_a) && (dist_b->get_counter() < dist_b->get_total_readings_for_two_transcievers() / 2)) {
        hw = &uwb_hw_b;
        dist_ptr = dist_b;
    } else if (dist_a->get_counter() < dist_a->get_total_readings_for_two_transcievers() / 2) {
        hw = &uwb_hw_a;
        dist_ptr = dist_a;
    }
    // Else: mantiene antena actual
}
```

**Comportamiento:**
1. Si está en A Y B tiene menos de mitad de lecturas → cambiar a B
2. Si no, si A tiene menos de mitad de lecturas → cambiar a A
3. Si no, mantener antena actual

**Objetivo:** Balancear lecturas entre antenas para triangulación.

---

## 7. Cálculo de Distancia (Two-Way Ranging)

### 7.1 Fórmula

```cpp
// Timestamps capturados
uint64_t rtd_init = RX_timestamp - TX_timestamp;  // Round-trip sniffer
uint64_t rtd_resp = resp_tx - poll_rx;            // Round-trip tag

// Compensación de clock offset
float clockOffsetRatio = dwt_readclockoffset() / (float)(1 << 26);

// Time of Flight
double tof = ((rtd_init - rtd_resp * (1 - clockOffsetRatio)) / 2.0) * DWT_TIME_UNITS;

// Distancia
double distance = tof * SPEED_OF_LIGHT;
```

### 7.2 Constantes

```c
#define DWT_TIME_UNITS      (1.0/499.2e6/128.0)  // ~15.65 ps
#define SPEED_OF_LIGHT      299702547.0          // m/s
```

### 7.3 Interpretación

1. `rtd_init`: Tiempo total desde que sniffer envía poll hasta que recibe respuesta
2. `rtd_resp`: Tiempo que tag tarda en procesar y responder
3. `clockOffsetRatio`: Corrección por diferencia de relojes entre dispositivos
4. TOF corregido: `(rtd_init - rtd_resp_corregido) / 2` (ida y vuelta)
5. Distancia: TOF × velocidad de la luz

---

## 8. Problemas Conocidos

### 8.1 Guardado Sin Validación ⚠️

**Código actual:**
```cpp
if (HAL_GetTick() - query_ticks > query_timeout) {
    save_two_maps_and_clear_tag(...);  // ← Guarda sin verificar
    tag_status = TAG_DISCOVERY;
}
```

**Problema:** Puede guardar tags con solo una antena medida → triangulación inválida.

**Solución recomendada:**
```cpp
if (HAL_GetTick() - query_ticks > query_timeout) {
    if (distance_a.get_counter() > 0 && distance_b.get_counter() > 0) {
        save_two_maps_and_clear_tag(...);
    }
    // Limpiar de todos modos
    distance_a.clear();
    distance_b.clear();
    tag_status = TAG_DISCOVERY;
}
```

### 8.2 Ventanas RX Cortas para >20m ⚠️

**Problema:** 
- `RESP_RX_TIMEOUT_UUS_6M8 = 300` (~308 µs) puede ser insuficiente
- A 20m, ToF ≈ 67 ns (ida y vuelta ~134 ns), pero señal débil puede requerir más tiempo detección

**Solución recomendada:**
- Aumentar a 600-800 UUS
- Ajustar `POLL_TX_TO_RESP_RX_DLY_UUS_6M8` a 900-1200

### 8.3 Mode One Detection Sin Triangulación ⚠️

**Problema:** Solo usa una antena, no alterna.

**Solución recomendada:** Forzar al menos 2-3 queries con alternancia incluso en modo único.

---

## 9. Configuración Radio DW3000

```c
dwt_config_t dwt_cfg = {
    5,                 // Channel 5 (6489.6 MHz)
    DWT_PLEN_128,      // Preamble 128 símbolos
    DWT_PAC8,          // PAC size 8
    9,                 // TX preamble code
    9,                 // RX preamble code
    1,                 // SFD non-standard 8 symbol
    DWT_BR_6M8,        // Data rate 6.8 Mb/s
    DWT_PHRMODE_STD,   // PHY header mode standard
    DWT_PHRRATE_STD,   // PHY header rate standard
    (129 + 8 - 8),     // SFD timeout
    DWT_STS_MODE_OFF,  // STS disabled
    DWT_STS_LEN_64,
    DWT_PDOA_M0        // PDOA off
};

dwt_txconfig_t dwt_tx_cfg = {
    0x34,              // PG delay
    0xfdfdfdfd,        // TX power
    0x0                // PG count
};
```

**Ubicación:** `uwb3000Fxx.c` líneas 50-75 (sniffer y tag, idéntico)

---

## 10. Referencias Rápidas

| Tema | Archivo | Líneas |
|------|---------|--------|
| Definición timeouts UWB | `sniffer/Core/Inc/uwb3000Fxx.h` | 726, 730, 733 |
| query_timeout sniffer | `sniffer/Core/Src/main.cpp` | 347 |
| query_timeout tag | `Persona/Core/Src/main.cpp` | 318 |
| RX_TIMEOUT_MS | `sniffer/Core/Src/sniffer_tag.cpp` | 532 |
| Discovery offsets | `sniffer/Core/Src/main.cpp` | 430-431 |
| Query offsets | `sniffer/Core/Src/main.cpp` | 492-493 |
| switch_hw | `sniffer/Core/Src/sniffer_tag.cpp` | 1031-1040 |
| switch_hw_timestamp_query | `sniffer/Core/Src/sniffer_tag.cpp` | 1042-1050 |
| Cálculo distancia | `sniffer/Core/Src/sniffer_tag.cpp` | 413-424 |
| Sleep times | `Persona/Core/Src/main.cpp` | 310-312 |

---

**Documento creado:** 2025-10-22  
**Basado en:** Código fuente real verificado  
**Mantenedor:** Actualizar al modificar protocolo
