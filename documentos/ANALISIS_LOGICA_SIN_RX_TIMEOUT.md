# Análisis de Lógica (Sin Profundizar en RX_TIMEOUT_MS)

## 1. Objetivo
Documentar problemas lógicos detectados en el flujo Sniffer ↔ Tag que no dependen directamente de los parámetros de tiempo, antes de incorporar ajustes con `RX_TIMEOUT_MS`.

## 2. Estados Principales (Sniffer)
- `TAG_DISCOVERY`: Envía poll y recibe identificación + primera distancia.
- `TAG_SEND_TIMESTAMP_QUERY`: Rango adicional; alterna antenas; acumula lecturas.
- `TAG_END_READINGS`: Marca fin de lecturas cuando recibe comando de sleep.
- `TAG_ONE_DETECTION`: Caso especial, se fuerza sleep rápido.

## 3. Problemas Lógicos Detectados

### 3.1 Guardado de Tag con Lecturas Parciales
En varios caminos de salida (timeout, errores) se llama a:
```
save_two_maps_and_clear_tag(distance_a, distance_b, ...)
```
Sin verificar si ambas antenas tienen mínimo 1 lectura válida. Se pueden persistir tags sin triangulación correcta.

### 3.2 Incremento de Lecturas sin Validación de Calidad
```
tag.readings++
```
Se incrementa tras cada respuesta válida sin evaluar SNR/potencia/ratio de error. Lecturas ruidosas contaminan la media.

### 3.3 Alternancia Basada Solo en Contadores
`switch_hw_timestamp_query()` alterna usando:
```
if (dist_b->get_counter() < total/2) ... else if (dist_a->get_counter() < total/2)
```
No considera:
- Errores consecutivos
- Última antena exitosa
- Calidad de señal almacenada

### 3.4 Reinicio Agresivo de Estado
```
if ((tag_status != TAG_SEND_TIMESTAMP_QUERY) && (tag_status != TAG_ONE_DETECTION)) tag_status = TAG_DISCOVERY;
```
Cualquier estado transitorio (ej. `TAG_RX_NO_COMMAND`) provoca reinicio inmediato sin fase de recuperación o reintento segmentado.

### 3.5 Finalización sin Mínimos por Antena
Al entrar en `TAG_END_READINGS` se guarda y limpia sin validar:
```
minLecturasA >= 1 && minLecturasB >= 1
```
No existe política de "mínimo técnico" para triangulación.

### 3.6 Contaminación de Contexto entre Tags
Variables como `distance_ptr` apuntan a estructuras reutilizadas. Si `clear()` no reinicia todos los campos (ej. error_track, último valor), podrían influir sobre nuevo tag.

### 3.7 Offsets Inconsistentes
Discovery usa offsets diferentes que Timestamp Query.
- **Discovery (primera detección):** `poll_rx_offset = 5` / `resp_tx_offset = 9`
  - Frame: `[cmd(1)][id(4)][poll_rx_ts(4)][resp_tx_ts(4)][battery(2)]...`
- **Query (subsecuente):** `poll_rx_offset = 1` / `resp_tx_offset = 5`
  - Frame: `[cmd(1)][poll_rx_ts(4)][resp_tx_ts(4)]`
  
**Razón:** Estructura de frame diferente (discovery incluye ID y batería, query solo timestamps).

Riesgo si el formato de frame no está documentado: cálculo RTD incorrecto.

**Verificado en código:**
- `sniffer/Core/Src/main.cpp` líneas 430-431 (discovery)
- `sniffer/Core/Src/main.cpp` líneas 492-493 (query)

### 3.8 Falta de Clasificación de Errores
Todos los errores en query se tratan igual:
```
distance_ptr->error_crc_increment(); tag_status = TAG_SEND_TIMESTAMP_QUERY;
```
No hay distinción entre: CRC, timeout de frame, preámbulo no detectado. Sin acciones adaptativas.

### 3.9 Modo `TAG_ONE_DETECTION` Subóptimo
Se fuerza `TAG_SET_SLEEP_MODE` rápido y se pierde oportunidad de obtener lecturas multian-tena para triangulación aunque sea un solo tag.

### 3.10 Persistencia en Timeout
En `TAG_SEND_TIMESTAMP_QUERY` si expira `query_timeout` se guarda aunque incompleto:
```
if (HAL_GetTick() - query_ticks > query_timeout) { save... }
```
Debería validar completitud mínima.

## 4. Recomendaciones Iniciales (Sin Cambiar Timeouts aún)
| Problema | Acción Correctiva |
|----------|-------------------|
| Lecturas parciales guardadas | Validar ambas antenas >0 antes de guardar |
| Calidad no evaluada | Incorporar extracción de SNR/FPP y filtrar |
| Alternancia simple | Añadir heurística: errores consecutivos + última exitosa |
| Reinicio agresivo | Introducir subestado de recuperación (RETRY) |
| Finalización sin mínimos | Definir MIN_PER_ANTENNA = 1–2 antes de cerrar ciclo |
| Contaminación contexto | Asegurar `clear()` reinicia métricas y estado |
| Offsets distintos | Documentar frame, verificar con assert tamaños |
| Error único | Mapear error → acción (cambiar antena, extender ventana) |
| Modo único detection | Agregar mini-rango (≥2 lecturas por antena) |
| Timeout guarda incompleto | Validar integridad antes de persistir |

## 5. Métricas para Verificación Posterior
- % Tags guardados con ambas antenas válidas.
- Promedio lecturas por antena vs objetivo.
- Tasa de errores por tipo (CRC/PRE/FRAME/NO_COMMAND).
- Ratio de reinicios inmediatos sin lecturas válidas.

## 6. Próximo Paso
Integrar ahora el impacto de `RX_TIMEOUT_MS` y su interacción con los otros parámetros para ajustar estrategia de reintentos sin romper la lógica corregida.
