# Sniffer-Tag GUI Tool

Interfaz gráfica para controlar y monitorear el firmware sniffer-tag STM32 via UART.

## Características

### 🎯 Control de Detección
- **One Detection Mode**: Detecta un solo tag y espera
- **Multiple Detection Mode**: Detecta múltiples tags continuamente

### ⚙️ Configuración

#### Parámetros de Operación
- Tiempo de sleep cuando se recibe tag
- Tiempo de sleep cuando NO se recibe tag
- Número de lecturas por tag (readings)

#### UWB Protocol Timeouts (NEW)
- **8 parámetros configurables** para optimización del protocolo DS-TWR:
  - **RX Timeouts**: PRE_TIMEOUT, RESP_RX_TIMEOUT, FINAL_RX_TIMEOUT
  - **TX → RX Delays**: POLL_TX_RESP_RX, RESP_TX_FINAL_RX
  - **RX → TX Delays**: POLL_RX_RESP_TX, RESP_RX_FINAL_TX
  - **Ranging Cycle**: RNG_DELAY (delay entre ciclos de ranging)
- Valores optimizados para largo alcance (>20m)
- Validación automática de rangos según tipo de parámetro (PAC/μs/ms)
- Comandos LTEL 0x30-0x37 para configuración dinámica

### 📊 Monitoreo en Tiempo Real
- **Vista de Tags Activos**: Tabla con información de cada tag detectado
  - ID del tag (formato 0xXXXX)
  - Número de lecturas (R)
  - Distancia A (metros)
  - Distancia B (metros)
  - Nivel de batería
  - Última vez visto
  
- **Log Serial**: Todos los mensajes del dispositivo con timestamps
  - Colores para diferentes tipos de mensajes (INFO, WARN, ERROR, DEBUG, TAG)
  - **Formato conciso** para errores múltiples (>10): muestra primeros 3 + últimos 3
  - Auto-scroll configurable
  - Guardado a archivo

### 📈 Estadísticas
- Total de detecciones
- Tags únicos detectados
- Tags activos actualmente
- Promedio de distancias A y B
- Promedio de batería
- Última detección

## Instalación

### Dependencias

```bash
pip install pyserial
```

### Sistema Operativo
- Windows, Linux, MacOS

## Uso

### Ejecutar la GUI

```bash
python sniffer_tag_gui.py
```

### Conexión

1. Selecciona el puerto COM del sniffer-tag
2. Selecciona baudrate (default: 115200)
3. Haz clic en "Connect"

### Comandos Disponibles

#### Modos de Detección
- **One Detection**: Modo de detección única (CMD 0x17)
- **Multiple Detection**: Modo de detección múltiple (CMD 0x18)

#### Configuración
- **Sleep Time (Received)**: Tiempo de espera cuando se detecta tag (CMD 0x19)
- **Sleep Time (Not Rcv)**: Tiempo de espera cuando NO se detecta tag (CMD 0x20)
- **Tag Readings**: Número de lecturas por tag (CMD 0x21)

#### Consultas
- **Query LTEL**: Consulta parámetros LTEL (CMD 0x11)
- **Query Sigma**: Consulta parámetros Sigma (CMD 0x12)
- **Query STR**: Consulta parámetros STR (CMD 0x15)
- **Query ADC**: Consulta valores ADC (CMD 0x16)

#### Configuración de Potencia
- **Set ATT LTEL**: Configura atenuación LTEL (CMD 0x20)
- **Set POUT Max**: Configura potencia máxima de salida (CMD 0x24)
- **Set POUT Min**: Configura potencia mínima de salida (CMD 0x23)

#### Configuración de Timeouts UWB (NEW)
- **Set UWB Timeouts**: Configura todos los timeouts del protocolo DS-TWR (CMD 0x30-0x37)
  - PRE_TIMEOUT (0x30): Timeout detección de preámbulo (PAC units)
  - RESP_RX_TIMEOUT (0x31): Timeout recepción RESPONSE (μs)
  - FINAL_RX_TIMEOUT (0x32): Timeout recepción FINAL (μs)
  - POLL_TX_RESP_RX (0x33): Delay POLL TX → RESPONSE RX (μs)
  - RESP_TX_FINAL_RX (0x34): Delay RESPONSE TX → FINAL RX (μs)
  - POLL_RX_RESP_TX (0x35): Delay POLL RX → RESPONSE TX (μs)
  - RESP_RX_FINAL_TX (0x36): Delay RESPONSE RX → FINAL TX (μs)
  - RNG_DELAY (0x37): Delay entre ciclos de ranging (ms)

### Comandos Personalizados

En el campo "Custom Command", puedes enviar comandos en formato hexadecimal:

```
Formato: CMD_ID [DATA]
Ejemplos:
  17          - One Detection (sin datos)
  19 05       - Set sleep received a 5 segundos
  21 0A       - Set tag readings a 10
  30 0C       - Set PRE_TIMEOUT a 12 PAC
  37 03E8     - Set RNG_DELAY a 1000 ms (0x03E8)
```

**Nota sobre timeouts UWB**: Los comandos 0x30-0x37 esperan datos en formato uint32_t (4 bytes, little-endian). El GUI maneja la conversión automáticamente.

## Formato del Protocolo

El GUI utiliza el protocolo LTEL definido en `CommandMessage.hpp`:

```
START(0x7E) + MODULE_FUNCTION(0x10) + MODULE_ID(0x00) + CMD + DATA_LEN_HIGH + DATA_LEN_LOW + DATA + CRC(2 bytes) + END(0x7F)
```

- **START**: 0x7E
- **MODULE_FUNCTION**: 0x10 (SNIFFER)
- **MODULE_ID**: 0x00 (configurable)
- **CMD**: Comando a ejecutar
- **DATA_LEN**: Longitud de datos (16-bit, big-endian)
- **DATA**: Datos opcionales
- **CRC**: CRC16 XMODEM (2 bytes, little-endian)
- **END**: 0x7F

## Interpretación de Logs

### Formato de Tag Detectado
```
[HH:MM:SS] TAG [0x2B45] - R:5, DistA:04.94, DistB:04.76, Bat:34
```

Donde:
- **TAG [0xXXXX]**: ID del tag (últimos 4 dígitos del ID completo)
- **R:X**: Número de lecturas realizadas
- **DistA**: Distancia medida por antena A (metros)
- **DistB**: Distancia medida por antena B (metros)
- **Bat**: Nivel de batería (0-255)

### Formato de Resumen
```
[HH:MM:SS] Tags detected: 2
[HH:MM:SS] No tags detected (scan 1)
```

## Guardado de Logs

El GUI permite guardar logs con estadísticas incluidas:

```
=== SNIFFER-TAG LOG ===
Generated: 2025-10-27 15:30:45
============================================================

[15:30:10] TAG [0x2B45] - R:5, DistA:04.94, DistB:04.76, Bat:34
[15:30:10] Tags detected: 1
...

============================================================
=== STATISTICS ===

Total Detections: 45
Unique Tags: 3
Active Tags: 2
```

## Depuración

### Modo DEBUG
El GUI muestra mensajes de depuración en gris cuando:
- Se envía un comando (muestra el frame completo en hexadecimal)
- Se reciben mensajes de debug del firmware

### Verificación de Frames
Los frames enviados se muestran en formato:
```
[HH:MM:SS] >>> SENT: CMD=0x17 (ONE_DETECTION), Frame=7e10001700000fcf7f
```

## Solución de Problemas

### No se detectan tags
1. Verifica que el modo sea "Multiple Detection"
2. Verifica que el número de readings sea apropiado (5-10)
3. Revisa los tiempos de sleep
4. **Ajusta PRE_TIMEOUT** si operas a >20m (incrementar de 5→12 PAC mejora detección)

### Distancias muestran N/A
- El contador de lecturas debe ser > 0
- Verifica que el UWB esté configurado correctamente
- **Revisa los timeouts RX**: valores muy bajos causan timeouts prematuros

### Caracteres extraños en batería
- El firmware debe usar `Real_Batt_Voltage` (uint8_t)
- No usar `Battery_Voltage` (uint16_t)

### Detección inestable a largo alcance (>20m)
- **Incrementar PRE_TIMEOUT**: De 5 PAC (default) a 12 PAC (mejora ~50% success rate)
- **Ajustar RX timeouts**: Incrementar RESP_RX_TIMEOUT y FINAL_RX_TIMEOUT
- **Incrementar RNG_DELAY**: Dar más tiempo entre ciclos para procesamiento
- Ver documentación: `documentos/ANALISIS_IMPACTO_TIEMPOS_UWB.md`

### Alto consumo de energía
- **Reducir frecuencia de ranging**: Incrementar RNG_DELAY (default 1000ms)
- Balance entre consumo y frecuencia de actualización según aplicación

## Diferencias con gui_tool.py

Este GUI está específicamente diseñado para el **sniffer-tag** (UWB tag detector), mientras que `gui_tool.py` es para el **gateway** (FSK scanner):

| Feature | sniffer_tag_gui.py | gui_tool.py |
|---------|-------------------|-------------|
| Protocol | LTEL (MODULE_FUNCTION=0x10) | FSK Scanner Protocol |
| Purpose | UWB Tag Detection | FSK Signal Scanning |
| Main Display | Active Tags Table | FSK Scanner Logs |
| Commands | Tag detection modes | FSK frequency scanning |
| Statistics | Tags, distances, battery | Signal detections, configs tested |

## Autor

Engineering Team - October 2025

## Licencia

Propiedad de Sigma Dev
