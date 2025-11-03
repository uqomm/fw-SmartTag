# Notas Técnicas - Sistema DW3000 UWB

## Nota 1: Calibración de Antenas

La suma de los valores es el **retardo de antena de TX a RX**, determinado experimentalmente por un proceso de calibración. Aquí usamos un valor típico codificado fijo, pero en una aplicación real, cada dispositivo debería tener su propio retardo de antena calibrado adecuadamente para obtener la mejor precisión posible al realizar mediciones de distancia.

---

## Nota 2: Formato de Mensajes UWB

Los mensajes cumplen con el estándar de codificación de **trama de datos MAC IEEE 802.15.4** y siguen el estándar **ISO/IEC:24730-62:2013**.

### Tipos de Mensajes

**Poll (Sondeo)**
- Enviado por el iniciador para disparar el intercambio de ranging

**Response (Respuesta)**
- Enviado por el respondedor para permitir que el iniciador continúe

**Final (Final)**
- Enviado por el iniciador para completar el intercambio

### Estructura de Trama (Primeros 10 bytes comunes)

| Bytes | Campo | Descripción |
|-------|-------|-------------|
| 0-1 | Frame Control | `0x8841` (trama de datos, direccionamiento 16-bit) |
| 2 | Sequence Number | Incrementado para cada nueva trama |
| 3-4 | PAN ID | `0xDECA` |
| 5-6 | Destination Address | Dirección de destino (16-bit) |
| 7-8 | Source Address | Dirección de origen (16-bit) |
| 9 | Function Code | Identifica el tipo de mensaje en el ranging |

### Datos Específicos por Mensaje

**Poll Message:**
- Sin datos adicionales

**Response Message:**
- Byte 10: Activity Code (`0x02` = continuar ranging)
- Bytes 11-12: Activity Parameter (no usado para 0x02)

**Final Message:**
- Bytes 10-13: Poll TX Timestamp
- Bytes 14-17: Response RX Timestamp
- Bytes 18-21: Final TX Timestamp

**Nota:** Todos los mensajes terminan con un **checksum de 2 bytes** establecido automáticamente por el DW IC.

---

## Nota 3: Direccionamiento de Dispositivos

Las direcciones de origen y destino son constantes codificadas en este ejemplo para mantenerlo simple. En un producto real:
- Cada dispositivo debe tener un **ID único**
- Se usa direccionamiento de **16 bits** para mantener los mensajes cortos
- Las direcciones cortas deben definirse mediante un intercambio de mensajes específicos previo

---

## Nota 4: Delays y Sincronización

Los retardos entre tramas se han elegido para asegurar:
- ✅ Sincronización adecuada de transmisión y recepción
- ✅ Precisión correcta de la distancia calculada

### Diagrama de Tiempos

```
Initiator:   |Poll TX| ..... |Resp RX| ........ |Final TX|
Responder:   |Poll RX| ..... |Resp TX| ........ |Final RX|
                ^|P|                                    - Poll TX/RX time
                        ^|R|                            - Resp TX/RX time
                                           ^|F|         - Final TX/RX time
```

### Parámetros de Retardo

| Parámetro | Símbolo | Descripción |
|-----------|---------|-------------|
| `POLL_TX_TO_RESP_RX_DLY_UUS` | `<--TDLY-->` | Delay TX Poll a RX Response |
| `RESP_RX_TIMEOUT_UUS` | `<-RLEN->` | Timeout para recibir Response |
| `POLL_RX_TO_RESP_TX_DLY_UUS` | `<----RDLY----->` | Delay respuesta del respondedor |
| `RESP_TX_TO_FINAL_RX_DLY_UUS` | `<--T2DLY-->` | Delay TX Response a RX Final |
| `FINAL_RX_TIMEOUT_UUS` | `<-FLEN--->` | Timeout para recibir Final |
| `RESP_RX_TO_FINAL_TX_DLY_UUS` | `<----RDLY----->` | Delay respuesta del iniciador |

### Ejemplos de Configuración

**Ejemplo 1: SPI @ 18 MHz (Por defecto)**
- Frame length: ~190 µs
- `POLL_RX_TO_RESP_TX_DLY_UUS`: 400 µs
- `RESP_RX_TO_FINAL_TX_DLY_UUS`: 400 µs
- TXtoRX delays: 210 µs

> **Nota:** Se puede reducir usando interrupciones o optimizaciones de código/SPI

**Ejemplo 2: SPI @ 4.5 MHz**
- Frame length: ~190 µs
- `POLL_RX_TO_RESP_TX_DLY_UUS`: 550 µs
- `RESP_RX_TO_FINAL_TX_DLY_UUS`: 600 µs
- TXtoRX delays: 360 y 410 µs respectivamente

---

## Nota 5: Timeouts de Recepción

El **timeout de recepción** es para la **recepción completa de una trama**, es decir, la duración debe tener en cuenta la **longitud de la trama esperada**.

A la tasa de datos de **6.81 Mbps** con frame lengths de ~190 µs, el valor típico es **alrededor de 200 µs**.

---

## Nota 6: Velocidad SPI Máxima

En este ejemplo, el DW IC se pone en estado **IDLE** después de llamar a `dwt_initialise()`.

Esto significa que se puede usar una **tasa SPI rápida de hasta 38 MHz** en adelante.

---

## Nota 7: Timeout del Preámbulo

El **timeout del preámbulo** permite al receptor **dejar de escuchar** en situaciones donde el preámbulo no está comenzando.

### Casos de Uso

- **Out of range**: El respondedor está fuera de alcance
- **No response**: El respondedor no recibió el mensaje
- **Ahorro energético**: Evita desperdicio escuchando un mensaje que no llegará

### Recomendaciones

| Caso | PAC Mínimo | Descripción |
|------|-----------|-------------|
| Corto alcance | 5 PAC | Aplicaciones de distancia corta |
| Largo alcance | 50-80% de preamble length | Entornos desafiantes, NLOS o ruidosos |

---

## Nota 8: Configuración de Potencia y Ancho de Banda

En una aplicación real, para un **rendimiento óptimo** dentro de los límites regulatorios, puede ser necesario:

- Establecer el **ancho de banda del pulso TX**
- Configurar la **potencia TX**
- Usar la API: `dwt_configuretxrf()`
- Valores: **Calibrados por dispositivo** guardados en:
  - Sistema objetivo
  - Memoria OTP del DW IC

---

## Nota 9: Escritura de Datos TX

`dwt_writetxdata()` toma el **tamaño completo del mensaje** como parámetro, pero solo copia **(tamaño - 2) bytes**.

### Detalles

- El checksum de **2 bytes** se agrega automáticamente por el DW IC
- Esto significa que la variable podría ser **2 bytes más corta** sin perder datos
- Sin embargo, `sizeof()` seguiría indicando la longitud completa

---

## Nota 10: Modo de Operación - Polling vs Interrupciones

Se usa el **modo de operación por polling** aquí para mantener el ejemplo simple.

### Alternativas

- **Polling**: Simple, secuencial
- **Interrupciones**: Todos los eventos de estado pueden generar interrupciones

### Lectura de Registro de Estado

El registro **STATUS** tiene **5 bytes de largo**, pero como los eventos usados están en los primeros bytes:
- Se puede usar la API simple: `dwt_read32bitreg()`
- En lugar de leer los 5 bytes completos

---

## Nota 11: Timestamps de Transmisión Final

Para enviar el **timestamp TX final** en el mensaje final:
- Se calcula **con anticipación** (no se lee del registro)
- Se agrega el retardo de respuesta deseado al timestamp RX de respuesta

### Resolución

- **Timestamps**: Expresados en **unidades de tiempo del dispositivo**
- **Transmisión retardada**: Resolución de **512 unidades de tiempo**
- **Operación**: Los **9 bits inferiores** deben ser cero (alineación de 512)
- **Codificación**: Se codifica en **palabras de 32 bits** desplazando los 8 bits inferiores

---

## Nota 12: Precisión de Cálculos de Timestamp

En esta operación, el **byte de orden superior** de cada timestamp de **40 bits** se **descarta**.

### Justificación

- Los timestamps **no están separados** por más de 2³² unidades de tiempo del dispositivo
- Esto equivale a aproximadamente **67 ms**
- El cálculo de retardos de ida y vuelta puede manejarse con una **resta de 32 bits**
- Suficientemente preciso para el cálculo del tiempo de vuelo

---

## Nota 13: Manejo de Errores en dwt_starttx()

Al ejecutar en la plataforma DWK3000 con el retardo `RESP_RX_TO_FINAL_TX_DLY` proporcionado, `dwt_starttx()` siempre tiene éxito.

### Caso de Error

Si el retardo es **demasiado corto** o algo interrumpe el flujo de código:
- `dwt_starttx()` puede emitirse **demasiado tarde** para el tiempo de inicio configurado
- El código quedaría **atascado esperando** un evento que nunca llegará

### Solución

```cpp
// Manejar condición de transmisión tardía
if (dwt_starttx(DWT_START_TX_DELAYED) != DWT_SUCCESS) {
    // Abandonar rangig, reintentar después de 1 segundo
    tag_status = TAG_TX_LATE;
    retry_time = HAL_GetTick() + 1000;
}
```

**Nota:** El ejemplo complementario del respondedor (ex_05b) debe agotar el tiempo de espera del "Final" y proceder a tener su receptor listo para el siguiente poll.

---

## Nota 14: Referencias Adicionales

Para más información, consulta:

- **DecaRanging ARM Application**
  - Distribuida con el producto EVK1000
  - Ejemplo práctico adicional de uso

- **DW IC API Guide**
  - Detalles sobre funciones del controlador del DW IC

- **DW IC User Manual**
  - Especificaciones completas del chip

---

## Referencias Rápidas

### Configuración por Defecto
```cpp
Channel: 5 (6.5 GHz)
Data Rate: 6.8 Mbps
PRF: 64 MHz
Preamble: 64 símbolos
PAC: 8 símbolos
```

### Timeouts Típicos (6.8 Mbps)
```cpp
POLL_TX_TO_RESP_RX_DLY_UUS: 700 µs
RESP_RX_TIMEOUT_UUS: 600 µs
POLL_RX_TO_RESP_TX_DLY_UUS: 400 µs
RESP_RX_TO_FINAL_TX_DLY_UUS: 400 µs
```

### Parámetros Críticos
```cpp
PRE_TIMEOUT: 8 PAC
SFD Timeout: (1025 + 1 + 8 - 32) = 1002
Antenna Delay: Calibrado por dispositivo
```
