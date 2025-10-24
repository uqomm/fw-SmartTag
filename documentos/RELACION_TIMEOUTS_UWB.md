# Relación e Impacto de query_timeout y RX_TIMEOUT en UWB

## 1. Parámetros involucrados

- **query_timeout**
  - Tiempo máximo que el sniffer/tag espera para completar el proceso de ranging (consulta de timestamps).
  - Si se excede, se descartan las lecturas y se reinicia el ciclo.

- **RESP_RX_TIMEOUT_UUS_6M8 (RX_TIMEOUT)**
  - Tiempo máximo que el receptor espera la llegada de la respuesta UWB tras abrir la ventana de recepción.
  - Si la respuesta no llega en ese tiempo, la medición se descarta.

- **POLL_TX_TO_RESP_RX_DLY_UUS_6M8**
  - Retardo entre el final del poll y el inicio de la ventana de recepción.

- **PRE_TIMEOUT_6M8**
  - Tiempo máximo para detectar el preámbulo de la señal.

---

## 2. Cómo interactúan en el flujo

### a) query_timeout
- Es el "timeout global" para todo el proceso de ranging con un tag.
- Incluye todos los intentos de consulta, reintentos y esperas.
- Si el proceso de ranging (varios queries) no se completa antes de que expire, se descartan los datos y se reinicia el ciclo.

### b) RX_TIMEOUT (RESP_RX_TIMEOUT_UUS_6M8)
- Es el "timeout local" para cada intento de recepción de respuesta UWB.
- Si la respuesta de un query individual no llega en ese tiempo, ese intento se descarta y puede reintentarse.

### c) Otros timeouts
- **POLL_TX_TO_RESP_RX_DLY_UUS_6M8:** Si es muy corto/largo, la ventana de RX puede abrirse en el momento incorrecto, afectando la probabilidad de éxito.
- **PRE_TIMEOUT_6M8:** Si es muy corto, señales débiles a larga distancia pueden no ser detectadas; si es muy largo, se pierde eficiencia.

---

## 3. Ejemplo de interacción y problemas

1. El sniffer inicia ranging con un tag y arranca el `query_timeout`.
2. Envía un poll, espera la respuesta:
   - Abre la ventana de RX tras `POLL_TX_TO_RESP_RX_DLY_UUS_6M8`.
   - Espera hasta `RESP_RX_TIMEOUT_UUS_6M8` por la respuesta.
   - Si no llega, reintenta (si hay tiempo dentro de `query_timeout`).
3. Si la suma de reintentos y esperas excede `query_timeout`, se descartan los datos.
4. Si el preámbulo no se detecta dentro de `PRE_TIMEOUT_6M8`, la recepción falla.

---

## 4. Impacto en mediciones y cálculos

- **A distancias largas (>20m):**
  - El tiempo de vuelo de la señal aumenta.
  - Si `POLL_TX_TO_RESP_RX_DLY_UUS_6M8` y `RESP_RX_TIMEOUT_UUS_6M8` son muy cortos, la respuesta puede llegar fuera de la ventana de RX.
  - Si `query_timeout` es muy corto, no hay suficientes reintentos para obtener lecturas válidas.
  - Si `PRE_TIMEOUT_6M8` es muy corto, la señal débil puede no ser detectada.

- **En conjunto:**
  - Si cualquiera de estos timeouts es demasiado restrictivo, se pierden lecturas y la triangulación falla.
  - Si son demasiado largos, se pierde eficiencia y aumenta el consumo energético.

---

## 5. Recomendaciones

- Ajustar `POLL_TX_TO_RESP_RX_DLY_UUS_6M8` y `RESP_RX_TIMEOUT_UUS_6M8` para cubrir el tiempo de vuelo esperado a la máxima distancia.
- Aumentar `query_timeout` si se requieren más reintentos a larga distancia.
- Balancear `PRE_TIMEOUT_6M8` para no perder señales débiles pero sin afectar el rendimiento.
- Monitorear logs de error para identificar cuál de los timeouts está causando la mayoría de los fallos.

---

> La correcta configuración y relación entre estos timeouts es clave para la robustez y precisión del sistema UWB, especialmente en escenarios de largo alcance.
