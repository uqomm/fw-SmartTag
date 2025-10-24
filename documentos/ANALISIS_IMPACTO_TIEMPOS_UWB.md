# Análisis de Impacto de Tiempos UWB en Medición y Cálculo (>20m)

## 1. Parámetros Críticos de Tiempo

- **POLL_TX_TO_RESP_RX_DLY_UUS_6M8 = 700**
  - Retardo entre el fin del poll y el inicio de la ventana de recepción.
- **PRE_TIMEOUT_6M8 = 5**
  - Timeout para detectar el preámbulo.
- **RESP_RX_TIMEOUT_UUS_6M8 = 300**
  - Timeout para recibir la respuesta del tag.

---

## 2. ¿Dónde se usan en el código?

En `uwb3000Fxx.c` (sniffer y tag):
```c
if (rate == RATE_6M8) {
    dwt_setrxaftertxdelay(POLL_TX_TO_RESP_RX_DLY_UUS_6M8);
    dwt_setrxtimeout(RESP_RX_TIMEOUT_UUS_6M8);
    dwt_setpreambledetecttimeout(PRE_TIMEOUT_6M8);
}
```

---

## 3. Análisis de Flujo y Posibles Problemas

### 3.1. Retardo entre TX y RX (`POLL_TX_TO_RESP_RX_DLY_UUS_6M8`)
- Si el retardo es demasiado corto, la ventana de recepción puede abrirse antes de que la señal del tag llegue, especialmente a >20m (mayor ToF).
- Si es demasiado largo, se pierde eficiencia y puede haber interferencia con otros eventos.
- **Problema típico:** A distancias largas, el pulso tarda más en llegar, y la ventana de RX puede cerrarse antes de recibir la respuesta.

### 3.2. Timeout de preámbulo (`PRE_TIMEOUT_6M8`)
- Si el preámbulo no se detecta en el tiempo configurado, la recepción se aborta.
- A mayor distancia, la señal puede ser más débil y requerir más tiempo para ser detectada.
- **Problema típico:** Lecturas fallidas por no detectar el preámbulo a tiempo.

### 3.3. Timeout de respuesta (`RESP_RX_TIMEOUT_UUS_6M8`)
- Define cuánto tiempo espera el sniffer/tag por la respuesta.
- Si la respuesta tarda más (por distancia o retardo en el tag), la ventana se cierra y se descarta la medición.
- **Problema típico:** A >20m, el tiempo de vuelo + procesamiento puede exceder el timeout, causando lecturas nulas.

---

## 4. Ejemplo de Flujo Problemático (>20m)

1. Sniffer envía poll.
2. Tag recibe y responde, pero la señal tarda más en llegar.
3. Sniffer abre ventana de RX según `POLL_TX_TO_RESP_RX_DLY_UUS_6M8`.
4. Si la respuesta llega tarde, el sniffer ya cerró la ventana (`RESP_RX_TIMEOUT_UUS_6M8`).
5. Si la señal es débil, el preámbulo no se detecta a tiempo (`PRE_TIMEOUT_6M8`).
6. Resultado: Lectura fallida, distancia no calculada.

---

## 5. Recomendaciones de Ajuste

- **Aumentar `POLL_TX_TO_RESP_RX_DLY_UUS_6M8`** para distancias largas (ej. 900-1200).
- **Aumentar `RESP_RX_TIMEOUT_UUS_6M8`** para permitir mayor tiempo de espera (ej. 500-800).
- **Aumentar `PRE_TIMEOUT_6M8`** si la señal es débil a larga distancia (ej. 8-12).
- **Implementar lógica adaptativa:** Ajustar estos parámetros según la distancia estimada o calidad de señal.
- **Registrar errores de timeout:** Para identificar si el problema es por ventana de RX, preámbulo o respuesta tardía.

---

## 6. Conclusión

Los parámetros de tiempo UWB son críticos para la robustez del sistema, especialmente a >20m. Si las ventanas de RX y los timeouts son demasiado cortos, se perderán lecturas válidas. Ajustar estos valores y analizar los logs de error permitirá mejorar la fiabilidad y precisión de las mediciones.
