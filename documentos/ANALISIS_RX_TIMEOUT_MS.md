# Análisis Específico de RX_TIMEOUT_MS y su Interacción con Otros Timeouts

## 1. Ubicación y Definición
`RX_TIMEOUT_MS` está definido en `sniffer/Core/Src/sniffer_tag.cpp`:
```c
#define RX_TIMEOUT_MS 100
```
Se usa dentro de la función `wait_rx_data()` como límite de sondeo activo del estado de recepción:
```c
while ((HAL_GetTick() - start_time) < RX_TIMEOUT_MS) { ... }
```
Si expira este bucle sin detectar ningún evento válido, retorna `TAG_NO_RXCG_DETECTED`.

## 2. Flujo Interno de `wait_rx_data()`
Orden de comprobación por cada iteración:
1. `SYS_STATUS_ALL_RX_ERR` → `TAG_RX_ERROR`
2. `SYS_STATUS_RXFTO_BIT_MASK` → `TAG_RX_FRAME_TIMEOUT`
3. `SYS_STATUS_RXPTO_BIT_MASK` → `TAG_RX_PREAMBLE_DETECTION_TIMEOUT`
4. `SYS_STATUS_RXFCG_BIT_MASK` → `TAG_RX_CRC_VALID`
5. Expira bucle → `TAG_NO_RXCG_DETECTED`

Esto añade una capa adicional de timeout (a nivel aplicación) sobre los timeouts configurados directamente en el transceptor:
- `dwt_setrxtimeout(RESP_RX_TIMEOUT_UUS_6M8)`
- `dwt_setpreambledetecttimeout(PRE_TIMEOUT_6M8)`

## 3. Jerarquía de Timeouts (Capas)
| Capa | Parámetro | Dominio | Propósito |
|------|-----------|---------|-----------|
| Física (Chip) | `POLL_TX_TO_RESP_RX_DLY_UUS_6M8` | UUS/registro | Apertura ventana RX tras TX |
| Física (Chip) | `RESP_RX_TIMEOUT_UUS_6M8` | UUS | Duración ventana de espera de frame |
| Física (Chip) | `PRE_TIMEOUT_6M8` | símbolo/preamb | Límite para detectar preámbulo |
| Aplicación (Loop) | `RX_TIMEOUT_MS` | ms (HAL tick) | Límite de sondeo del estado RX total |
| Ciclo Ranging | `query_timeout` | ms (HAL tick) | Tiempo total asignado al tag para completar lecturas |

## 4. Riesgos Específicos de RX_TIMEOUT_MS
1. **Demasiado corto (100 ms):**
   - Puede abortar antes de que el chip active internamente su propio timeout de preámbulo o de frame.
   - Resulta en retorno prematuro `TAG_NO_RXCG_DETECTED` aunque la señal estuviera próxima.
2. **Duplicidad de condiciones:**
   - Si el frame llega justo después de que la aplicación salga del bucle, se pierde.
3. **Inconsistencia de escalas:**
   - `RESP_RX_TIMEOUT_UUS_6M8 = 300` UUS ≈ 300 * 1.0256 µs ≈ 307.7 µs.
   - A nivel de chip la ventana es extremadamente corta comparada con 100 ms de sondeo externo.
   - Sugiere que el límite de aplicación (100 ms) no es el cuello, sino que la ventana física podría ser demasiado pequeña para >20 m.

## 5. Problema Combinado a >20 m
```
TX Poll -> (Delay 700 UUS) -> RX Window (≈ 308 µs) -> Preamble Timeout (5 unidades) ->
Aplicación espera hasta 100 ms -> No se detecta nada -> TAG_NO_RXCG_DETECTED.
```
A distancias largas, si la señal llega débil o con ligeros retardos (variaciones internas, interferencias), la ventana física (`RESP_RX_TIMEOUT_UUS_6M8`) puede cerrarse demasiado pronto mucho antes de consumir los 100 ms del bucle.

## 6. Síntomas Asociados
| Síntoma | Causa raíz probable |
|---------|---------------------|
| Muchos `TAG_NO_RXCG_DETECTED` | Ventana física demasiado corta (no RX_TIMEOUT_MS) |
| Alternancia antena sin éxito | RX_WINDOW insuficiente en antena con peor SNR |
| Lecturas solo en una antena | Segunda antena falla por ventana estrecha |
| Preamble timeouts frecuentes | `PRE_TIMEOUT_6M8` bajo + señal atenuada |

## 7. Recomendaciones sobre RX_TIMEOUT_MS
| Escenario | RX_TIMEOUT_MS | Comentario |
|-----------|---------------|------------|
| Base actual | 100 ms | No crítico si ventana física sigue siendo el cuello |
| Diagnóstico | 200–300 ms | Para confirmar si hay eventos tardíos (poco probable) |
| Optimizado | 50–80 ms | Reducir energía, si se amplían ventanas físicas |

## 8. Ajustes Más Impactantes (Mayor Prioridad que RX_TIMEOUT_MS)
1. Aumentar `RESP_RX_TIMEOUT_UUS_6M8` (300 → 600–800 UUS) antes de tocar RX_TIMEOUT_MS.
2. Aumentar `POLL_TX_TO_RESP_RX_DLY_UUS_6M8` si se sospecha que la apertura RX ocurre muy pronto.
3. Ajustar `PRE_TIMEOUT_6M8` (5 → 8–12) para soportar señales débiles.
4. Registrar marcas de tiempo para medir latencia real entre TX y detección.

## 9. Posible Mejora en `wait_rx_data()`
- Incluir salida inmediata si se detecta que el chip ya cerró la ventana interna.
- Añadir conteo de iteraciones para detectar loops demasiado largos.
- Registrar motivo exacto (`RXFTO`, `RXPTO`, `ALL_RX_ERR`).

Pseudo-código mejorado:
```c
while ((HAL_GetTick() - start) < RX_TIMEOUT_MS) {
  status = dwt_read32bitreg(SYS_STATUS_ID);
  if (status & SYS_STATUS_RXFCG_BIT_MASK) { clearRXFCG(); return TAG_RX_CRC_VALID; }
  if (status & (SYS_STATUS_RXFTO_BIT_MASK | SYS_STATUS_RXPTO_BIT_MASK)) { clearTimeouts(); return classify_timeout(status); }
  if (status & SYS_STATUS_ALL_RX_ERR) { clearErrors(); return TAG_RX_ERROR; }
  // Optional: break early if physical timeout already elapsed
}
return TAG_NO_RXCG_DETECTED; // Only if no physical event occurred
```

## 10. Métrica para Evaluar RX_TIMEOUT_MS
- % de eventos `TAG_NO_RXCG_DETECTED` vs `TAG_RX_FRAME_TIMEOUT`.
- Si `NO_RXCG` domina y `FRAME_TIMEOUT` es raro → ventana física demasiado corta.
- Si `NO_RXCG` casi no aparece, reducir RX_TIMEOUT_MS para ahorro energético.

## 11. Conclusión
`RX_TIMEOUT_MS` actúa como guardia externo y rara vez es el cuello real en tu escenario. La discordancia >20 m está más relacionada con las ventanas físicas (UUS) que con el lazo de 100 ms. Priorizar ajuste de `RESP_RX_TIMEOUT_UUS_6M8`, `POLL_TX_TO_RESP_RX_DLY_UUS_6M8`, y `PRE_TIMEOUT_6M8` antes de modificar el bucle de sondeo. Luego optimizar `RX_TIMEOUT_MS` para eficiencia.

## 12. Próximo Paso
Implementar logging de clasificación de fallos y ensayo A/B con:
- Perfil A: Aumentar ventanas físicas, mantener RX_TIMEOUT_MS = 100.
- Perfil B: Igual que A + reducir RX_TIMEOUT_MS = 70.
Comparar tasa de lecturas válidas y consumo.
