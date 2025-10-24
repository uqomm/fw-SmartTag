# Análisis Integrado de Timeouts y Lógica UWB (Sniffer ↔ Tags)

## 1. Resumen de Parámetros Clave

| Parámetro | Rol | Valor Real | Riesgo si es Bajo | Riesgo si es Alto |
|-----------|-----|------------|------------------|-------------------|
| `query_timeout` (ms) | Tiempo global para completar todas las queries a un tag | **1000** | Lecturas incompletas | Ciclo lento / consumo extra |
| `RESP_RX_TIMEOUT_UUS_6M8` | Ventana de espera de respuesta tras abrir RX | **300** (~308 µs) | Respuesta llega fuera de ventana | Captura ruido / falsas detecciones |
| `POLL_TX_TO_RESP_RX_DLY_UUS_6M8` | Retardo entre TX poll y apertura RX | **700** (~718 µs) | RX abre demasiado pronto | Latencia innecesaria |
| `PRE_TIMEOUT_6M8` | Tiempo para detectar preámbulo | **5** | No detecta señal débil | Ineficiente |
| `RX_TIMEOUT_MS` | Timeout bucle aplicación sondeo RX | **100** | Salida prematura (poco probable) | Consumo extra |
| Intervalo entre queries | Tiempo de reposo entre intentos | variable | Tag no listo / solapamiento | Lento |
| Sleep del Tag (sin lecturas) | Ciclo energético | **500 ms** | Tag se duerme prematuramente | Tag activo demasiado tiempo |
| Sleep del Tag (con lecturas) | Ciclo energético tras éxito | **15000 ms** | - | - |

## 2. Relaciones Funcionales

1. `POLL_TX_TO_RESP_RX_DLY_UUS_6M8` + `RESP_RX_TIMEOUT_UUS_6M8` definen la "ventana efectiva" de captura.
2. `PRE_TIMEOUT_6M8` actúa dentro de esa ventana como filtro inicial (preámbulo correcto).
3. Cada intento fallido consume parte del presupuesto temporal impuesto por `query_timeout`.
4. El tag puede abandonar (sleep) si el sniffer no completa a tiempo → pérdida de triangulación.

```
[TX Poll] -- (Delay: POLL_TX_TO_RESP_RX_DLY) --> [Open RX Window for RESP_RX_TIMEOUT]
                      |-- Dentro: Detectar preámbulo (PRE_TIMEOUT)
```

## 3. Cadena de Fallo Típica >20m

```
Señal debilitada -> Preámbulo no detectado (PRE_TIMEOUT muy corto) ->
Reintento -> Gasto de tiempo global -> query_timeout expira ->
Tag pasa a sleep sin lecturas completas -> Distancias incompletas.
```

## 4. Señales de Discordancia Observables

| Síntoma | Causa Probable | Parámetro Implicado |
|---------|----------------|---------------------|
| Solo una antena con lecturas | Expira ciclo antes de alternar | `query_timeout` bajo |
| Muchas lecturas fallidas consecutivas | Ventana RX subdimensionada | `RESP_RX_TIMEOUT_UUS_6M8` |
| Ninguna detección >20m | Delay y preámbulo insuficientes | `POLL_TX_TO_RESP_RX_DLY`, `PRE_TIMEOUT` |
| Distancias erráticas | Ruido capturado | `RESP_RX_TIMEOUT_UUS_6M8` demasiado alto |
| Tag se duerme rápido | Estado no sincronizado | Lógica de transición / `query_timeout` |

## 5. Ajustes Recomendados (Base)

| Parámetro | Actual | Propuesto Test 1 | Propuesto Test 2 (Adaptativo) |
|-----------|--------|------------------|-------------------------------|
| `POLL_TX_TO_RESP_RX_DLY_UUS_6M8` | 700 | 950 | 700–1100 según SNR |
| `RESP_RX_TIMEOUT_UUS_6M8` | 300 | 600 | 300–800 según distancia previa |
| `PRE_TIMEOUT_6M8` | 5 | 10 | 5–12 según fallo previo |
| `query_timeout` | 1000 ms | 1800 ms | 1200–2500 ms según éxito parcial |

## 6. Métrica para Evaluar Mejora

- Tasa de éxito por antena (% respuestas válidas / intentos).
- Latencia promedio de ciclo por tag.
- Lecturas mínimas garantizadas (>=1 por antena) vs completadas (objetivo 3–4).
- Ratio de expiraciones de `query_timeout`.

## 7. Recomendación de Logging

Agregar (solo modo diagnóstico):
```
LOG: poll_sent @tick
LOG: rx_open @tick (delay=..., win=...)
LOG: preamble_detect success|fail @tick
LOG: resp_received @tick (TOF=..., SNR=...)
LOG: query_cycle progress (reads_A=..., reads_B=..., elapsed=...)
LOG: timeout_reason (query|rx|preamble)
```

## 8. Próximo Paso

Tras estabilizar tiempos, revisar lógica independiente de estos (transiciones de estado, alternancia antena, validación de lecturas, manejo de error).

---

## 9. Análisis Inicial de Lógica (Previo a Profundización)

Hallazgos esperados a verificar:
- ¿Se reinicia correctamente contador de lecturas al cambiar de tag? (posible contaminación de estado previo).
- ¿Se alterna antena sin verificar calidad mínima? (puede desperdiciar ventana en antena inservible).
- ¿Se guarda tag aunque falte una de las dos distancias? (triangulación inválida).
- ¿Se manda a sleep un tag tras parcial éxito en una sola antena? (pérdida prematura de oportunidad).
- Criterio para finalizar ciclo: ¿usa total lecturas o condición de comando recibido? Riesgo de terminar antes de alternar.

Se requiere lectura de bucle principal y funciones `switch_hw_timestamp_query`, `save_two_maps_and_clear_tag`, `tag_receive_cmd`.

---

(Se continuará con una auditoría lógica detallada fuera del ámbito de los parámetros de tiempo.)
