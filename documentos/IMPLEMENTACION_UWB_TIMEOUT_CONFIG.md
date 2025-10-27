# Implementación de Configuración de UWB Timeouts

## Resumen
Este documento detalla la implementación de comandos para configurar dinámicamente los timeouts de recepción UWB desde la GUI.

## Comandos Definidos

### 1. SET_RESP_RX_TIMEOUT (0x30)
Configura el timeout para recibir mensajes RESPONSE en el protocolo DS-TWR.

**Formato:**
- Command ID: `0x30`
- Data: 4 bytes (uint32_t little-endian)
- Valor: Timeout en microsegundos (μs)
- Rango válido: 100 - 10000 μs
- Valor por defecto: 600 μs

### 2. SET_FINAL_RX_TIMEOUT (0x31)
Configura el timeout para recibir mensajes FINAL en el protocolo DS-TWR.

**Formato:**
- Command ID: `0x31`
- Data: 4 bytes (uint32_t little-endian)
- Valor: Timeout en microsegundos (μs)
- Rango válido: 100 - 10000 μs
- Valor por defecto: 220 μs

## Implementación en Firmware

### Paso 1: Agregar constantes en CommandMessage.hpp

```cpp
// Comandos de configuración UWB
constexpr uint8_t SET_RESP_RX_TIMEOUT = 0x30;
constexpr uint8_t SET_FINAL_RX_TIMEOUT = 0x31;
```

### Paso 2: Implementar handlers en main.cpp

```cpp
// Variables globales para timeouts configurables
static uint32_t resp_rx_timeout_uus = RESP_RX_TIMEOUT_UUS_6M8;  // Default: 600
static uint32_t final_rx_timeout_uus = FINAL_RX_TIMEOUT_UUS_6M8; // Default: 220

// En el handler de comandos:
case SET_RESP_RX_TIMEOUT:
{
    uint32_t timeout = command_message.getDataAsUint32();
    if (timeout >= 100 && timeout <= 10000) {
        resp_rx_timeout_uus = timeout;
        // Actualizar timeout en DW3000
        dwt_setrxtimeout(resp_rx_timeout_uus);
        printf("[INFO] RESP RX Timeout set to %lu μs\n", timeout);
    } else {
        printf("[ERROR] Invalid RESP timeout: %lu (valid: 100-10000)\n", timeout);
    }
    break;
}

case SET_FINAL_RX_TIMEOUT:
{
    uint32_t timeout = command_message.getDataAsUint32();
    if (timeout >= 100 && timeout <= 10000) {
        final_rx_timeout_uus = timeout;
        printf("[INFO] FINAL RX Timeout set to %lu μs\n", timeout);
    } else {
        printf("[ERROR] Invalid FINAL timeout: %lu (valid: 100-10000)\n", timeout);
    }
    break;
}
```

### Paso 3: Usar variables configurables en ranging

En las funciones de ranging, reemplazar las constantes por las variables:

```cpp
// En lugar de:
dwt_setrxtimeout(RESP_RX_TIMEOUT_UUS_6M8);

// Usar:
dwt_setrxtimeout(resp_rx_timeout_uus);

// En lugar de:
dwt_setrxtimeout(FINAL_RX_TIMEOUT_UUS_6M8);

// Usar:
dwt_setrxtimeout(final_rx_timeout_uus);
```

## Uso desde la GUI

La GUI (`sniffer_tag_gui.py`) ya incluye la interfaz para configurar estos timeouts:

1. **Campos de entrada:**
   - RESP RX Timeout: Campo numérico (default: 600 μs)
   - FINAL RX Timeout: Campo numérico (default: 220 μs)

2. **Botón "Apply Timeouts":**
   - Valida rangos (100-10000 μs)
   - Envía comandos 0x30 y 0x31 con valores configurados
   - Muestra confirmación con advertencia sobre soporte del firmware

3. **Mensaje de advertencia:**
   ```
   ⚠ Increasing timeouts improves long-range detection 
   but slows response time
   ```

## Consideraciones de Diseño

### Valores de Timeout

**RESP RX Timeout (600 μs):**
- Original: 300 μs
- Actual: 600 μs (duplicado para mejorar detección >20m)
- Impacto: Mayor tolerancia a delays en propagación de señal
- Trade-off: +300 μs por ciclo de ranging

**FINAL RX Timeout (220 μs):**
- Valor conservador para mensaje pequeño
- Suficiente para PRF de 6.8 MHz
- Menor impacto en tiempo total

### Límites Recomendados

| Aplicación | RESP (μs) | FINAL (μs) | Descripción |
|------------|-----------|------------|-------------|
| Corto alcance (0-10m) | 300-400 | 150-200 | Respuesta rápida |
| Medio alcance (10-20m) | 500-700 | 200-250 | Balanceado (actual) |
| Largo alcance (>20m) | 800-1200 | 250-300 | Máxima tolerancia |
| Debug/Test | 1500-3000 | 500-1000 | Diagnóstico de problemas |

### Impacto en Performance

```
Tiempo total por tag = T_poll_tx + T_resp_rx + T_final_tx + T_final_rx + T_process

Ejemplo con valores actuales (600 + 220 μs):
- 10 tags × 820 μs = 8.2 ms
- + overhead de processing (~2 ms)
- Total: ~10 ms por ciclo completo

Con timeouts largos (1200 + 300 μs):
- 10 tags × 1500 μs = 15 ms
- + overhead (~2 ms)
- Total: ~17 ms por ciclo
```

## Testing

### Verificación de Comandos

1. Conectar sniffer-tag via USB
2. Abrir GUI (`python sniffer_tag_gui.py`)
3. Configurar puerto serial
4. Ingresar valores de timeout
5. Presionar "Apply Timeouts"
6. Verificar en logs del firmware:
   ```
   [INFO] RESP RX Timeout set to 800 μs
   [INFO] FINAL RX Timeout set to 250 μs
   ```

### Pruebas de Rango

**Test 1: Corto alcance (5m)**
- RESP: 300 μs, FINAL: 150 μs
- Resultado esperado: Detección exitosa, logs limpios

**Test 2: Medio alcance (15m)**
- RESP: 600 μs, FINAL: 220 μs
- Resultado esperado: Detección consistente

**Test 3: Largo alcance (25m)**
- RESP: 1000 μs, FINAL: 300 μs
- Resultado esperado: Reduce timeouts en Ant-A

### Análisis de Logs

Antes (timeouts fijos, muchos errores):
```
╔══════════════════════════════════════════════════════════════════╗
║ TAG: 0x664821AB | Battery: 85% | Total: 50 errors                ║
╠══════════════════════════════════════════════════════════════════╣
║ ⏱ [Ant-A] RX_PREAMBLE_DETECTION_TIMEOUT (R:0, 6ms)              ║
║ ⏱ [Ant-A] RX_PREAMBLE_DETECTION_TIMEOUT (R:0, 6ms)              ║
[... 46 more similar errors ...]
```

Después (timeouts optimizados):
```
╔══════════════════════════════════════════════════════════════════╗
║ TAG: 0x664821AB | Battery: 85% | Total: 5 errors                 ║
╠══════════════════════════════════════════════════════════════════╣
║ ⏱ [Ant-A] RX_PREAMBLE_DETECTION_TIMEOUT (R:0, 9ms)              ║
║ ⏱ [Ant-A] RX_PREAMBLE_DETECTION_TIMEOUT (R:0, 9ms)              ║
║ ⏱ [Ant-A] RX_PREAMBLE_DETECTION_TIMEOUT (R:0, 9ms)              ║
[... only 2 more ...]
```

## Integración con Sistema de Logs

El nuevo formato de logs conciso trabaja en conjunto con los timeouts configurables:

1. **Timeouts grandes** → Menos errores espurios → Logs más limpios
2. **Formato conciso** → Los errores reales son más visibles
3. **Estadísticas** → Fácil ver si cambios de timeout mejoran detección

## Referencias

- `uwb3000Fxx.h`: Definiciones de timeouts por defecto
- `sniffer_tag.cpp`: Sistema de logging de errores
- `sniffer_tag_gui.py`: Interfaz de configuración
- `FORMATO_MEJORADO_LOG_ERRORES.md`: Documentación del formato de logs

## Changelog

- **v1.0** (2024): Implementación inicial de comandos SET_RESP_RX_TIMEOUT y SET_FINAL_RX_TIMEOUT
- Integración con GUI para configuración dinámica
- Documentación de límites y consideraciones de diseño
