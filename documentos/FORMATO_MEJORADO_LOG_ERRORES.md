# Formato Mejorado de Log de Errores UWB

## 📋 Nuevo Formato

El sistema ahora muestra logs de errores con un formato más claro y estructurado:

```
╔═══ ERROR LOG: Tag 0x62482B5E ═══╗
║ Total Errors: 50 (Ant-A:25, Ant-B:25)
║ Types: Timeouts=48, CRC=2, Others=0
╠═════════════════════════════════════════╣
║ ⏱ [Ant-B] RX_PREAMBLE_DETECTION_TIMEOUT
║    └─ Readings:0, Time:6ms
║ ⏱ [Ant-A] RX_PREAMBLE_DETECTION_TIMEOUT
║    └─ Readings:1, Time:6ms
║ ⏱ [Ant-B] RX_PREAMBLE_DETECTION_TIMEOUT
║    └─ Readings:0, Time:6ms
║ ⏱ [Ant-A] RX_PREAMBLE_DETECTION_TIMEOUT
║    └─ Readings:1, Time:6ms
...
╚═════════════════════════════════════════╝
```

## 🔍 Componentes del Log

### Header (Resumen Ejecutivo)
```
╔═══ ERROR LOG: Tag 0xXXXXXXXX ═══╗
║ Total Errors: 50 (Ant-A:25, Ant-B:25)
║ Types: Timeouts=48, CRC=2, Others=0
╠═════════════════════════════════════════╣
```

**Información proporcionada:**
- **Tag ID**: Identificador completo del tag (8 dígitos hex)
- **Total Errors**: Número total de errores registrados
- **Ant-A / Ant-B**: Distribución de errores por antena
- **Types**: Clasificación por tipo de error
  - **Timeouts**: `RX_PREAMBLE_DETECTION_TIMEOUT` + `RX_FRAME_TIMEOUT`
  - **CRC**: `NO_RXCG_DETECTED` (errores de checksum)
  - **Others**: Otros tipos de error

### Body (Detalle de Eventos)
```
║ ⏱ [Ant-B] RX_PREAMBLE_DETECTION_TIMEOUT
║    └─ Readings:0, Time:6ms
```

**Elementos:**
- **Indicador**: 
  - ⏱ = Timeout
  - ✗ = CRC Error
  - ⚠ = Otros errores
- **[Ant-X]**: Antena donde ocurrió el error
- **Tipo de Error**: Nombre descriptivo del error
- **Readings**: Número de lecturas exitosas antes del error
- **Time**: Tiempo transcurrido en milisegundos

## 📊 Interpretación del Resumen

### Ejemplo 1: Tag con Problemas Sistemáticos
```
╔═══ ERROR LOG: Tag 0x62482B5E ═══╗
║ Total Errors: 50 (Ant-A:25, Ant-B:25)
║ Types: Timeouts=48, CRC=2, Others=0
```

**Análisis:**
- ✅ **50 errores** es mucho para un tag funcional
- ✅ **Distribución equitativa** (25/25) sugiere problema del tag, no del sniffer
- ✅ **48 timeouts** indica señal débil o distancia excesiva
- ⚠️ **2 CRC errors** es aceptable (interferencia ocasional)

**Diagnóstico:** Tag muy lejos o con transmisión débil

### Ejemplo 2: Tag con Problema en Una Antena
```
╔═══ ERROR LOG: Tag 0x6A4829EC ═══╗
║ Total Errors: 2 (Ant-A:2, Ant-B:0)
║ Types: Timeouts=2, CRC=0, Others=0
```

**Análisis:**
- ✅ **Solo 2 errores** es excelente
- ✅ **Ant-A únicamente** sugiere problema temporal
- ✅ **Tag funciona después** (no es problema grave)

**Diagnóstico:** Inicialización o interferencia momentánea

### Ejemplo 3: Tag con Problemas de CRC
```
╔═══ ERROR LOG: Tag 0xXXXXXXXX ═══╗
║ Total Errors: 20 (Ant-A:10, Ant-B:10)
║ Types: Timeouts=5, CRC=15, Others=0
```

**Análisis:**
- ⚠️ **15 CRC errors** de 20 es alto
- ✅ **Solo 5 timeouts** indica señal llega bien
- ❌ **Datos corruptos** por interferencia o ruido

**Diagnóstico:** Interferencia electromagnética o problema del tag

## 🔢 Patrones de Readings

El campo **Readings** indica cuántas mediciones exitosas se hicieron **antes** del error:

```
║ ⏱ [Ant-B] RX_PREAMBLE_DETECTION_TIMEOUT
║    └─ Readings:0, Time:6ms    ← Primera medición falló

║ ⏱ [Ant-A] RX_PREAMBLE_DETECTION_TIMEOUT
║    └─ Readings:1, Time:6ms    ← Falló después de 1 medición exitosa

║ ⏱ [Ant-B] RX_PREAMBLE_DETECTION_TIMEOUT
║    └─ Readings:0, Time:6ms    ← Primera medición de esta antena

║ ⏱ [Ant-A] RX_PREAMBLE_DETECTION_TIMEOUT
║    └─ Readings:2, Time:6ms    ← Falló después de 2 mediciones exitosas
```

### Interpretación de Patrones

#### Patrón A: Alternancia 0-1-0-1
```
Readings:0 (Ant-B) → Readings:1 (Ant-A) → Readings:0 (Ant-B) → Readings:1 (Ant-A)
```
**Significado:** Ant-A funciona parcialmente, Ant-B nunca logra sincronizar
**Causa:** Ant-B no puede "ver" el tag (obstrucción, ángulo, distancia)

#### Patrón B: Todos con Readings:0
```
Readings:0 → Readings:0 → Readings:0 → Readings:0
```
**Significado:** Intentos de comunicación inicial fallidos
**Causa:** Tag inicializándose o muy lejos

#### Patrón C: Incremento progresivo
```
Readings:0 → Readings:1 → Readings:2 → Readings:3
```
**Significado:** Tag funcionando pero con errores ocasionales
**Causa:** Señal marginal, interferencia intermitente

## 🎯 Uso del Log para Diagnóstico

### Paso 1: Revisar el Resumen
```
║ Total Errors: X (Ant-A:Y, Ant-B:Z)
```
- Si X > 10: Investigar problema
- Si Y ≈ Z: Problema del tag o distancia
- Si Y >> Z o Z >> Y: Problema de una antena específica

### Paso 2: Revisar Tipos
```
║ Types: Timeouts=A, CRC=B, Others=C
```
- Timeouts altos: Señal débil, acercar tag
- CRC altos: Interferencia, revisar entorno
- Others altos: Problema de configuración

### Paso 3: Revisar Patrones de Readings
- Readings siempre 0: Tag no sincroniza
- Readings alternados: Una antena funciona, otra no
- Readings crecientes: Tag funcionando con errores ocasionales

### Paso 4: Verificar Resultado Final
```
[HH:MM:SS] TAG [0x2B5E] - R:3, DistA:05.34, DistB:N/A, Bat:35
```
Comparar con el log de errores para confirmar diagnóstico

## 📈 Comparación: Formato Antiguo vs Nuevo

### Formato Antiguo
```
=== Log Tag 62482B5E (50 eventos de error) ===
[A] RX_PREAMBLE_DETECTION_TIMEOUT, lecturas=1, tiempo=6ms
[B] RX_PREAMBLE_DETECTION_TIMEOUT, lecturas=0, tiempo=6ms
[A] RX_PREAMBLE_DETECTION_TIMEOUT, lecturas=1, tiempo=6ms
[B] RX_PREAMBLE_DETECTION_TIMEOUT, lecturas=0, tiempo=6ms
...
=== Fin Log ===
```

**Problemas:**
- ❌ Sin resumen ejecutivo
- ❌ Difícil contar errores por antena
- ❌ No clasifica tipos de error
- ❌ Formato plano, difícil de leer

### Formato Nuevo
```
╔═══ ERROR LOG: Tag 0x62482B5E ═══╗
║ Total Errors: 50 (Ant-A:25, Ant-B:25)
║ Types: Timeouts=48, CRC=2, Others=0
╠═════════════════════════════════════════╣
║ ⏱ [Ant-A] RX_PREAMBLE_DETECTION_TIMEOUT
║    └─ Readings:1, Time:6ms
║ ⏱ [Ant-B] RX_PREAMBLE_DETECTION_TIMEOUT
║    └─ Readings:0, Time:6ms
...
╚═════════════════════════════════════════╝
```

**Ventajas:**
- ✅ **Resumen ejecutivo** con estadísticas
- ✅ **Clasificación automática** de errores
- ✅ **Visual indicators** (⏱, ✗, ⚠)
- ✅ **Formato estructurado** fácil de leer
- ✅ **Diagnóstico rápido** sin analizar todo el log

## 🛠️ Casos de Uso Reales

### Caso 1: Tag Funcionando Correctamente
```
No se muestra log de errores (count = 0)

[HH:MM:SS] TAG [0x29EC] - R:5, DistA:04.30, DistB:03.82, Bat:36
```
✅ **Perfecto**: Sin errores, mediciones completas

### Caso 2: Tag con Errores Iniciales
```
╔═══ ERROR LOG: Tag 0x6A4829EC ═══╗
║ Total Errors: 2 (Ant-A:2, Ant-B:0)
║ Types: Timeouts=2, CRC=0, Others=0
╠═════════════════════════════════════════╣
║ ⏱ [Ant-A] RX_PREAMBLE_DETECTION_TIMEOUT
║    └─ Readings:0, Time:6ms
║ ⏱ [Ant-A] RX_PREAMBLE_DETECTION_TIMEOUT
║    └─ Readings:0, Time:6ms
╚═════════════════════════════════════════╝

[HH:MM:SS] TAG [0x29EC] - R:5, DistA:04.30, DistB:03.82, Bat:36
```
✅ **Aceptable**: Errores de inicialización, luego funciona perfectamente

### Caso 3: Tag con Problemas Sistemáticos
```
╔═══ ERROR LOG: Tag 0x62482B5E ═══╗
║ Total Errors: 50 (Ant-A:25, Ant-B:25)
║ Types: Timeouts=48, CRC=2, Others=0
╠═════════════════════════════════════════╣
...50 errores...
╚═════════════════════════════════════════╝

[HH:MM:SS] TAG [0x2B5E] - R:3, DistA:05.34, DistB:N/A, Bat:35
```
❌ **Problema**: Solo 3 lecturas logradas de 5 intentadas, Ant-B no funciona

## 💡 Recomendaciones Según Log

### Si ves: `Total Errors: 50+ (Ant-A:25, Ant-B:25)`
**Acción:** Acercar el tag o remover obstáculos

### Si ves: `Total Errors: X (Ant-A:X, Ant-B:0)` o viceversa
**Acción:** Revisar orientación del tag respecto a las antenas

### Si ves: `Types: Timeouts=5, CRC=20, Others=0`
**Acción:** Buscar fuentes de interferencia electromagnética

### Si ves: Readings siempre 0
**Acción:** Verificar que el tag esté transmitiendo

### Si ves: Readings incrementales pero errores frecuentes
**Acción:** Tag en límite de rango, mejorar señal

## 📝 Notas Técnicas

- **Buffer Size**: Máximo 50 eventos de error por tag
- **Overflow**: Si hay más de 50 errores, se guardan los primeros 50
- **Timestamp**: Cada error registra su tiempo de ocurrencia
- **No Impact**: El logging se hace **después** de todas las mediciones, no afecta timing UWB
- **UART Output**: 115200 baud, suficiente para logs detallados

## 🎓 Conclusión

El nuevo formato de log proporciona:
1. ✅ **Diagnóstico rápido** con resumen ejecutivo
2. ✅ **Clasificación automática** de tipos de error
3. ✅ **Visualización clara** con indicadores
4. ✅ **Análisis profundo** con detalles de cada evento
5. ✅ **Fácil interpretación** sin necesidad de contar manualmente

Esto permite identificar problemas de RF en segundos en lugar de analizar cientos de líneas de log.
