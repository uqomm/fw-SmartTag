# 🧪 Checklist Tests Físicos - Corrección Detección >20m

**Fecha**: ___________ | **Responsable**: ___________ | **Versión FW**: ___________

---

## 📜 HISTORIAL DE INVESTIGACIÓN (Orden Cronológico)

### **ESTADO BASE DEL PROYECTO** [📅 22-Oct-2025]

**Situación del Sistema**:
- ✅ **Detección estable**: Sistema funcional hasta **±20 metros** de distancia
- ❌ **Problema de software**: NO había "permanencia" de los tags detectados en el sistema
- 🔧 **Causa raíz identificada**: Tiempos de detección de tags y de envío al servidor cuando no hay ningún tag

**Acciones Realizadas**:

1. **Análisis de Tiempos de Detección y Envío**:
   - Se identificó que el timeout de envío al servidor (cuando no hay tags) estaba mal configurado
   - Modificación de timeouts resolvió el problema de permanencia de tags
   - Sistema comenzó a mantener correctamente la información de tags detectados

2. **Análisis Completo del Sistema** (22-Oct-2025):
   - 📄 **Documentación de Comunicación**: Análisis detallado de la comunicación Sniffer ↔ Tag
   - 📄 **Documentación de Máquina de Estados**: Flujo completo de estados en ambos equipos (Sniffer y Persona)
   - ✅ Sistema estabilizado y documentado para futuras mejoras

**Conclusión**: Sistema operacional y estable @ ≤20m, con documentación técnica completa. A partir de este punto, se inicia investigación de fallas de detección >20m.

---

### **TEST-00: PRE_TIMEOUT=5 (Baseline)** [✅ COMPLETADO - 23-Oct-2025]

**Configuración**: `PRE_TIMEOUT_6M8 = 5 PAC` (40 símbolos, baseline de fábrica)

**Resultado @ 23m**:
```
Canal A: 0% éxito (152 timeouts RX_PREAMBLE_DETECTION_TIMEOUT)
Canal B: 100% éxito (DistB: 22.88-23.42m, 0 errores)
```

**❌ Conclusión**: Configuración baseline **INSUFICIENTE** para detección >20m en Canal A. Canal B funciona perfectamente.

---

### **TEST-01: PRE_TIMEOUT=8 (Solución 1A)** [✅ COMPLETADO - 23-Oct-2025]

**Configuración**: `PRE_TIMEOUT_6M8 = 8 PAC` (64 símbolos, +60% vs baseline)

**Resultado @ 20m**:
```
Canal A: 3% éxito (96 timeouts, 3 lecturas: 19.77-19.89m)
Canal B: 9.7% éxito (28 timeouts, 3 lecturas: 19.93-20.18m)
```

**⚠️ Análisis**:
- ✅ Canal A: 0% → 3% mejora (progreso marginal desde baseline)
- ⚠️ Canal B: 100% @ 23m → 9.7% @ 20m (degradación inesperada, posible cambio de entorno)
- ⚠️ Ambos canales muestran baja tasa de éxito comparado con TEST-00 @ 23m

**⚠️ Conclusión**: PRE_TIMEOUT=8 **NO es suficiente**. Se requiere +50% adicional (PRE_TIMEOUT=12).

---

### **TEST-01B: Cambio de Antenas AP + PRE_TIMEOUT=12** [✅ COMPLETADO - 27-Oct-2025]

**Contexto**: Por indicación de Daniel, se cambiaron las antenas del AP por antenas anteriores para evaluar si el tipo de antena afectaba la detección.

**Configuración**: 
- `PRE_TIMEOUT_6M8 = 12 PAC` (96 símbolos, +140% vs baseline)
- **Antenas**: Cambiadas de antenas AP a antenas anteriores
- **Setup**: Tag montado en taburete con batería externa, en bajada del camión

**Resultado @ 22.3m (Antena B)**:
```
Comportamiento dependiente de orientación del tag:
- Posición 1 (luz LED apuntando al Sniffer): 1 antena detecta
- Posición 2 (tag girado): Ninguna antena detecta
```

**⚠️ Observaciones Críticas**:
- 🔄 **Polarización de antena afecta detección**: Orientación del tag es crítica
- 📡 **Tag "mirando" al Sniffer**: LED apuntando hacia Sniffer necesario para detección
- ⚠️ **Solo 1 antena detecta** (probablemente Antena B): Confirma problema asimétrico entre canales
- 🔴 **Cambio de antenas AP NO resolvió problema**: Antenas anteriores muestran mismo comportamiento

**❌ Conclusión**: 
- Cambio de antenas AP → antenas anteriores **NO mejora detección** de Canal A
- Problema de orientación/polarización puede ser factor adicional, pero NO explica falla total de Canal A
- Se confirma que problema NO es tipo de antena externa, sino **componentes internos (LNA, chip DW3000)**

---

### **TEST-02: Swap Físico de Antenas** [✅ COMPLETADO - 28-Oct-2025]

**Objetivo**: Determinar si problema es hardware físico (antena/cable) o configuración lógica (Canal A del firmware).

**Evidencia**:
- **Configuración Original**: Canal A → Antena Física 1, Canal B → Antena Física 2
  - Canal A @ 21.7m: 0% éxito (100% timeout)
  - Canal B @ 21.7m: 100% éxito (51/51 detecciones)
  
- **Configuración Swap**: Canal A → Antena Física 2, Canal B → Antena Física 1
  - Canal A @ 21.7m: 0% éxito (100% timeout) ← **Problema sigue en Canal A**
  - Canal B @ 21.7m: 100% éxito ← **Funciona con cualquier antena**

**✅ Conclusión**: Problema es **SOFTWARE/CONFIGURACIÓN del Canal A lógico**, NO hardware de antenas.

---

### **TEST-03: Separar Estructuras `pdw3000local`** [✅ COMPLETADO - 28-Oct-2025]

**Hipótesis**: Ambos chips DW3000 (Canal A y B) comparten una sola estructura `dwt_local_data_t`, causando que Canal B sobrescriba calibración OTP de Canal A.

**Problema identificado**:
```c
// main.cpp línea 65 y 1117
dwt_local_data_t *pdw3000local;  // ← UNA SOLA estructura para AMBOS chips
```

**Solución implementada**:
- Creadas estructuras separadas: `pdw3000local_a`, `pdw3000local_b`, `pdw3000local` (puntero activo)
- Modificado `init_uwb_device()` para recibir `dwt_local_data_t *local_data`
- Actualizado `switch_hw()` y `switch_hw_timestamp_query()` para cambiar puntero
- Archivos modificados: `main.cpp`, `sniffer_tag.cpp`, `sniffer_tag.hpp`

**Resultado TEST @ 21m**:
```
Canal A: 0% éxito (50/50 RX_PREAMBLE_DETECTION_TIMEOUT)
Canal B: 100% éxito (DistB: 21.16-21.81m, 0 errores)
```

**Verificación OTP**:
```
CANAL A: PartID:0x624831E5  Bias:0x13  DGC_MODE:OTP ✅
CANAL B: PartID:0x6E483065  Bias:0x12  DGC_MODE:OTP ✅
```

**❌ Conclusión**: Hipótesis **INCORRECTA**. Estructuras separadas funcionan correctamente (PartID diferentes, calibraciones únicas), pero Canal A sigue fallando. Problema NO era calibración OTP compartida.

---

### **TEST-04: Eliminación HAL_Delay(1) en Persona** [✅ COMPLETADO - 28-Oct-2025]

**Contexto**: Análisis de código identificó un `HAL_Delay(1)` en `main.cpp` del tag Persona que podría causar descoordinación temporal en comunicación `MULTIPLE_DETECTION` en caso de falla.

**Modificación realizada**:
- **Archivo**: `Persona/Core/Src/main.cpp`
- **Cambio**: Eliminado `HAL_Delay(1)` en caso de falla de comunicación MULTIPLE_DETECTION
- **Objetivo**: Reducir jitter temporal (0-2ms) que podría causar timeouts en comunicación >20m

**Configuración de prueba**:
- `PRE_TIMEOUT_6M8 = 12 PAC` (mantenido desde TEST-01B)
- **Setup**: Mismo taburete, misma posición que TEST-02 (21.7m)
- **Orientación**: Tag apuntando al Sniffer (LED hacia Sniffer)

**Resultado @ 21.7m**:
```
Canal A: 0% éxito (sin detección)
Canal B: 100% éxito (solo Antena B detecta)
```

**❌ Análisis**:
- ❌ **Resultados similares a pruebas anteriores**: Sin mejora a pesar de eliminar HAL_Delay(1)
- ⚠️ **Solo Antena B detecta**: Mismo comportamiento que TEST-02 y TEST-01B
- 🔴 **Optimización de timing NO resuelve problema de Canal A**: Problema más profundo que delays en código

**❌ Conclusión**: 
- Eliminación de `HAL_Delay(1)` **NO mejora detección** de Canal A
- Confirma que problema NO es descoordinación temporal por delays en código
- Se confirma **problema HARDWARE en componentes del Canal A** (LNA degradado, filtro RF, o chip DW3000 defectuoso)
- **Requiere solución más radical**: TEST-07 (850K data rate con +8dB sensibilidad) o reemplazo de hardware

---

### **TEST-05: Comparación Configuraciones Múltiples Tags** [✅ COMPLETADO - 29-Oct-2025]

**Contexto**: Se cambian antenas nuevamente a las del AP para prueba comparativa con 3 tags Persona con diferentes configuraciones de PRE_TIMEOUT y HAL_Delay.

**Configuración Sniffer**:
- `PRE_TIMEOUT_6M8 = 12 PAC` (96 símbolos)
- **Antenas**: Antenas del AP (vueltas a configuración original)
- **Setup**: Línea de vista, 3 tags simultáneos

**Configuración Tags (3 variantes)**:
| Tag ID | PRE_TIMEOUT | HAL_Delay(1) | Config |
|--------|-------------|--------------|--------|
| **0x2B45** | 8 PAC | ✅ CON | Baseline con delay |
| **0x2B5E** | 8 PAC | ❌ SIN | Sin delay optimizado |
| **0x2783** | 12 PAC | ❌ SIN | PRE_TIMEOUT aumentado + sin delay |

**Resultados @ ~18-21m (línea de vista)**:

**Tag 0x2B45** (PRE_TIMEOUT=8 + HAL_Delay):
```
Detección: 100% éxito
DistA: 18.22-19.88m
DistB: 16.87-18.57m
Errores: Mínimos (ocasionales timeouts en Ant-B)
```

**Tag 0x2B5E** (PRE_TIMEOUT=8, sin HAL_Delay):
```
Detección: 100% éxito
DistA: 18.74-21.43m
DistB: 16.79-19.32m
Errores: Frecuentes timeouts en Ant-B (1-50 errores por ciclo)
Patrón: Errores incrementan con distancia (18m: pocos, 21m: muchos)
```

**Tag 0x2783** (PRE_TIMEOUT=12, sin HAL_Delay):
```
Detección: Variable (60-100%)
DistA: 18.84-21.47m (frecuentes fallos >20m)
DistB: 16.23-19.06m
Errores: MUY frecuentes timeouts en Ant-A (hasta 50 errores consecutivos)
Patrón crítico: 
  - @ 18-19m: Ambas antenas funcionan
  - @ >20m: Ant-A falla sistemáticamente (0% éxito con 50 timeouts)
  - @ >20m: Solo Ant-B detecta (modo degradado R:3 lecturas)
```

**📊 Análisis Comparativo**:

| Aspecto | Tag 0x2B45 (8+Delay) | Tag 0x2B5E (8) | Tag 0x2783 (12) |
|---------|---------------------|----------------|-----------------|
| **Tasa éxito @ 18m** | 100% ✅ | 100% ✅ | 100% ✅ |
| **Tasa éxito @ >20m** | 100% ✅ | ~90% ⚠️ | ~60% ❌ |
| **Errores Ant-A** | Casi nulos | Bajos | MUY ALTOS (50+) |
| **Errores Ant-B** | Bajos | Medios | Bajos |
| **Comportamiento** | Estable | Degradación gradual | Fallo crítico >20m |

**🔍 Observaciones Críticas**:

1. **⚠️ HAL_Delay(1) parece BENEFICIAR la comunicación**:
   - Tag 0x2B45 (CON delay) tiene MENOS errores que 0x2B5E (SIN delay)
   - Contradice hipótesis inicial de que HAL_Delay causaba problemas
   - Posible explicación: Delay da tiempo al Sniffer para procesar entre frames

2. **🔴 PRE_TIMEOUT=12 EMPEORA detección >20m**:
   - Tag 0x2783 (PRE_TIMEOUT=12) tiene PEOR desempeño que 0x2B5E (PRE_TIMEOUT=8)
   - @ >20m: 50 timeouts consecutivos en Ant-A con PRE_TIMEOUT=12
   - Contradice expectativa de que mayor PRE_TIMEOUT mejora detección

3. **📡 Patrón de fallo consistente en Ant-A @ >20m**:
   - Todos los tags muestran degradación en Ant-A al alejarse
   - Tag 0x2783 muestra fallo más severo: modo degradado R:3 (solo Ant-B)
   - Confirma problema hardware de Ant-A/Canal A

4. **⏱️ Timing de errores constante**: 
   - Todos los timeouts duran 6ms (consistente con PRE_TIMEOUT configurado)
   - No hay variación significativa en duración de timeouts

**❌ Conclusión**:
- ❌ **PRE_TIMEOUT=12 NO mejora detección >20m**: Al contrario, EMPEORA el rendimiento
- ⚠️ **HAL_Delay(1) NO es perjudicial**: Tag con delay tiene MEJOR desempeño que sin delay
- 🔴 **Problema hardware Ant-A/Canal A CONFIRMADO**: Fallo sistemático >20m en todos los tags
- ✅ **Configuración óptima actual**: PRE_TIMEOUT=8 + HAL_Delay(1) (Tag 0x2B45)
- 🔧 **Solución requerida**: TEST-07 (850K data rate) o reemplazo hardware de Canal A

**⚠️ Nota importante**: Resultados contradicen hipótesis de optimización. Mayor PRE_TIMEOUT y eliminación de HAL_Delay NO mejoran detección, sugiriendo que problema es puramente hardware, no timing/configuración software.

---

### **TEST-06: Validación Tag con PRE_TIMEOUT=5 + HAL_Delay** [✅ COMPLETADO - 29-Oct-2025]

**Contexto**: Se agregó un cuarto tag Persona (ID: 0x29EC) con configuración conservadora (PRE_TIMEOUT=5 baseline + HAL_Delay activado) para validar si la configuración más básica tiene mejor desempeño que las optimizadas.

**Configuración Sniffer**:
- `PRE_TIMEOUT_6M8 = 12 PAC` (96 símbolos)
- **Antenas**: Antenas del AP (configuración estándar)
- **Setup**: Línea de vista, test simultáneo con tags de TEST-05

**Configuración Tag 0x29EC**:
| Parámetro | Valor | Descripción |
|-----------|-------|-------------|
| **PRE_TIMEOUT** | 5 PAC | ⬇️ Baseline original (40 símbolos) |
| **HAL_Delay(1)** | ✅ ACTIVADO | Delay de 1ms entre ciclos |
| **Firmware** | Standard | Sin optimizaciones |

**Resultados @ 13-19m (línea de vista)**:

**Tag 0x29EC** (PRE_TIMEOUT=5 + HAL_Delay):
```
Detección: ~70% éxito (variable según distancia)
DistA: N/A (fallo total Ant-A)
DistB: 13.26-19.26m
Modo operación: R:3 (solo Ant-B) mayormente
                R:5 (ambas antenas) ocasionalmente @ <16m
Errores: 50 timeouts consecutivos en Ant-A (máximo buffer)
Patrón crítico: 
  - @ 13-16m: Ocasionalmente logra R:5 (ambas antenas)
  - @ >16m: Degradación total a R:3 (solo Ant-B)
  - @ >18m: Errores incrementan en Ant-B también (25-50 timeouts)
```

**📊 Análisis Detallado**:

**Distribución de Errores**:
- **Ant-A**: 100% fallo (50/50 timeouts en buffer completo)
- **Ant-B**: Funcional hasta ~18m, luego degradación
- **Tipo de error**: 100% `RX_PREAMBLE_DETECTION_TIMEOUT` (6ms cada uno)
- **Patrón temporal**: Errores constantes sin recuperación en Ant-A

**Comparativa de Distancias**:
```
@ 13-15m: R:5 ocasional (15.27-15.90m DistA exitosa pocas veces)
@ 15-17m: R:3 mayormente (DistB: 15.32-16.78m estable)
@ 17-19m: R:3 exclusivo (DistB: 18.02-19.26m con errores en Ant-B)
@ >19m: Detección intermitente (25 timeouts Ant-A + 25 Ant-B)
```

**🔍 Observaciones Críticas**:

1. **🔴 PRE_TIMEOUT=5 (baseline) tiene PEOR rendimiento que PRE_TIMEOUT=8**:
   - Tag 0x29EC (PRE_TIMEOUT=5): Fallo Ant-A desde ~15m
   - Tag 0x2B45 (PRE_TIMEOUT=8): Ambas antenas funcionan hasta >20m
   - Confirma que PRE_TIMEOUT=5 es insuficiente para distancias largas

2. **📉 Degradación más temprana con PRE_TIMEOUT=5**:
   - @ 15m: Tag 0x29EC ya está en modo R:3 (solo Ant-B)
   - @ 15m: Tags con PRE_TIMEOUT=8 aún tienen R:5 (ambas antenas)
   - Diferencia de ~5m en rango efectivo de Ant-A

3. **⚠️ HAL_Delay(1) NO compensa PRE_TIMEOUT bajo**:
   - Tag 0x29EC tiene HAL_Delay pero sigue fallando antes que 0x2B45
   - Confirma que PRE_TIMEOUT es más crítico que HAL_Delay
   - HAL_Delay beneficia SOLO cuando PRE_TIMEOUT es adecuado

4. **📊 Consistencia con patrón hardware Canal A**:
   - Fallo total de Ant-A @ distancias medias (15-19m)
   - Ant-B funciona correctamente hasta ~18m
   - Confirma problema hardware en Canal A del Sniffer

5. **🔄 Errores balanceados Ant-A/Ant-B @ límite de rango**:
   - @ >19m: 25 timeouts Ant-A + 25 Ant-B (50% cada uno)
   - Sugiere que @ límite de rango ambos canales tienen dificultad
   - Pero Ant-A falla mucho antes (15m vs 19m)

**📋 Tabla Comparativa 4 Tags**:

| Tag ID | PRE_TIMEOUT | HAL_Delay | Rango Ant-A | Rango Ant-B | Modo @ 18m |
|--------|-------------|-----------|-------------|-------------|------------|
| **0x2B45** | 8 PAC | ✅ SI | >20m ✅ | >20m ✅ | R:5 (dual) |
| **0x2B5E** | 8 PAC | ❌ NO | >20m ✅ | >20m ⚠️ | R:5 (dual) |
| **0x2783** | 12 PAC | ❌ NO | ~18m ❌ | >20m ✅ | R:3 (Ant-B) |
| **0x29EC** | 5 PAC | ✅ SI | ~15m ❌ | ~19m ⚠️ | R:3 (Ant-B) |

**❌ Conclusión**:

1. **🔴 PRE_TIMEOUT=5 es INSUFICIENTE para operación >15m**:
   - Rango efectivo Ant-A: ~15m (vs 20m con PRE_TIMEOUT=8)
   - Degradación ~33% más temprana que configuración óptima

2. **📉 Ranking de configuraciones (mejor a peor)**:
   - 🥇 **PRE_TIMEOUT=8 + HAL_Delay(1)** (Tag 0x2B45): Mejor desempeño general
   - 🥈 **PRE_TIMEOUT=8 sin HAL_Delay** (Tag 0x2B5E): Bueno pero con más errores
   - 🥉 **PRE_TIMEOUT=5 + HAL_Delay(1)** (Tag 0x29EC): Rango reducido ~25%
   - ❌ **PRE_TIMEOUT=12 sin HAL_Delay** (Tag 0x2783): Fallo crítico >20m

3. **✅ Confirmación configuración óptima**: 
   - **PRE_TIMEOUT=8 + HAL_Delay(1)** es la mejor combinación
   - Proporciona balance entre sensibilidad y estabilidad
   - Rango efectivo máximo: ~20m ambas antenas

4. **⚠️ Problema hardware Canal A persiste en TODAS las configuraciones**:
   - PRE_TIMEOUT=5: Fallo @ 15m
   - PRE_TIMEOUT=8: Fallo @ 20m
   - PRE_TIMEOUT=12: Fallo @ 20m (peor que 8)
   - **Solución requerida**: TEST-07 (850K) o reemplazo hardware

**🔧 Recomendación Final**:
- ✅ **Mantener PRE_TIMEOUT=8 + HAL_Delay(1)** como configuración estándar
- ❌ **Descartar PRE_TIMEOUT=5** (rango insuficiente)
- ❌ **Descartar PRE_TIMEOUT=12** (empeora rendimiento)
- 🔬 **Próximo paso**: Implementar TEST-07 (850K data rate) como última solución SW

---

### **TEST-07: Sniffer PRE_TIMEOUT=8 con Movimiento y Obstrucción Corporal** [✅ COMPLETADO - 29-Oct-2025]

**Contexto**: Se modificó el preámbulo del Sniffer de 12 PAC a 8 PAC para sincronizarlo con los tags. **CONDICIÓN CRÍTICA**: Test realizado con movimiento constante del operador llevando los tags, con intervención corporal intermitente de línea de vista (NLOS - Non Line Of Sight parcial).

**Configuración Sniffer**:
- `PRE_TIMEOUT_6M8 = 8 PAC` ⬇️ (reducido desde 12 PAC)
- **Antenas**: Antenas del AP
- **Setup**: **MOVIMIENTO + OBSTRUCCIÓN CORPORAL** (condición NLOS parcial)

**Configuración Tags** (mantenidas de TEST-05/06):
| Tag ID | PRE_TIMEOUT | HAL_Delay(1) | Config |
|--------|-------------|--------------|--------|
| **0x2B45** | 8 PAC | ✅ SI | Configuración óptima |
| **0x2B5E** | 8 PAC | ❌ NO | Sin delay |
| **0x2783** | 12 PAC | ❌ NO | PRE_TIMEOUT alto |
| **0x29EC** | 5 PAC | ✅ SI | PRE_TIMEOUT bajo |

**⚠️ DIFERENCIA CLAVE**: A diferencia de TEST-05/06 (línea de vista estática), este test simula **condiciones reales de uso** con movimiento y obstrucción humana intermitente.

---

**Resultados @ 2.74-18.85m (NLOS parcial con movimiento)**:

**FASE 1: Distancias cortas (2-11m) - Línea de vista mayormente libre**:
```
Tags: Todos detectados con R:5 (ambas antenas)
DistA: 4.51-11.19m
DistB: 2.74-9.02m
Errores: Mínimos (Tag 0x2B45: 25 errores mixtos Ant-A/B)
Comportamiento: ESTABLE, ambas antenas funcionan correctamente
```

**FASE 2: Distancias medias (11-16m) - Obstrucción corporal intermitente**:
```
Tags: Degradación a R:3 (solo una antena)
Tag 0x2B5E: R:3 con Ant-A fallando (50 timeouts Ant-A)
Tag 0x2B45: R:3 con Ant-A fallando (48 timeouts Ant-A, 2 Ant-B)
Tag 0x29EC: R:3 con Ant-A fallando (50 timeouts Ant-A)
Tag 0x2783: R:3 con Ant-A fallando (50 timeouts Ant-A)

Patrón: Fallo sistemático de Ant-A con obstrucción corporal
```

**FASE 3: Distancias largas (16-19m) - Cambio de orientación**:
```
🔄 INVERSIÓN DE ERRORES (fenómeno crítico):

Tag 0x2B5E @ 18.60-18.84m: 
  - R:5 (ambas antenas funcionan)
  - Errores: 18-50 timeouts en Ant-B (Ant-A sin errores)

Tag 0x29EC @ 18.02-18.83m:
  - R:3 con Ant-B fallando (50 timeouts Ant-B)
  - R:5 ocasional (ambas funcionan con errores en Ant-B)
  - DistA: 18.02-18.83m (Ant-A FUNCIONAL)
  - DistB: N/A o 16.01-16.91m

Tag 0x2783 @ 18.81-18.83m:
  - R:5 (ambas antenas funcionan)
  - Errores: 28 Ant-A + 22 Ant-B (balanceados)
  - También detectado con Ant-B fallando (50 timeouts Ant-B, Ant-A=0)

Tag 0x2B45 @ 18.66-18.85m:
  - R:5 (ambas antenas funcionan)
  - Errores variables según orientación
```

**FASE 4: Retorno a distancias cortas (12-16m) - Movimiento continuo**:
```
Tags: Comportamiento errático dependiente de orientación
Tag 0x2783 @ 14.82m: Ant-B fallando (50 timeouts), Ant-A funcional
Tag 0x29EC @ 14.79m: Ant-B fallando (50 timeouts), Ant-A funcional
Tag 0x2B45 @ 15.48-15.94m: 
  - R:5 con errores mixtos (7 Ant-A + 43 Ant-B)
  - R:3 con Ant-B fallando completamente

Patrón: INVERSIÓN COMPLETA vs FASE 2 (ahora Ant-B falla, Ant-A funciona)
```

---

**📊 Análisis Crítico del Fenómeno de Inversión**:

**🔴 Descubrimiento MÁS IMPORTANTE del testing completo**:

1. **Ant-A NO tiene problema hardware permanente**:
   - @ FASE 3: Ant-A funciona perfectamente (0 errores) mientras Ant-B falla (50 timeouts)
   - Tag 0x29EC @ 18.79m: DistA=18.79m EXITOSA, DistB=N/A (Ant-B 50 timeouts)
   - Tag 0x2783 @ 18.83m: DistA=18.83m EXITOSA, DistB=N/A (Ant-B 50 timeouts)

2. **El problema es de ORIENTACIÓN/POLARIZACIÓN de antenas**:
   - En línea de vista estática (TEST-05/06): Ant-A falla sistemáticamente
   - Con movimiento/rotación (TEST-07): Errores se invierten entre Ant-A y Ant-B
   - Patrón: La antena que está en SOMBRA respecto al cuerpo del operador falla

3. **Sniffer PRE_TIMEOUT=8 MEJORA detección con movimiento**:
   - Tags con PRE_TIMEOUT=8 (0x2B45, 0x2B5E, 0x29EC) funcionan mejor
   - Tag 0x2783 (PRE_TIMEOUT=12) sigue mostrando problemas

4. **Obstrucción corporal afecta más que distancia**:
   - @ 11-14m con cuerpo obstruyendo: R:3 con 50 timeouts
   - @ 18-19m con mejor orientación: R:5 con ambas antenas funcionando

---

**📋 Tabla Comparativa de Configuraciones bajo NLOS**:

| Tag ID | PRE_TIMEOUT | Rango Ant-A | Rango Ant-B | Comportamiento NLOS |
|--------|-------------|-------------|-------------|---------------------|
| **0x2B45** | 8 PAC + Delay | 2-19m ✅ | 2-16m ⚠️ | Mejor resiliencia a obstrucción |
| **0x2B5E** | 8 PAC | 2-19m ✅ | 2-19m ✅ | Funciona bien con movimiento |
| **0x29EC** | 5 PAC + Delay | 2-19m ✅* | 2-16m ⚠️ | Rango corto en estático, OK con movimiento |
| **0x2783** | 12 PAC | 2-19m ⚠️ | 2-19m ⚠️ | Más errores con cualquier orientación |

*✅ = Funcional, ⚠️ = Intermitente según orientación, ❌ = Fallo total

---

**🔍 Observaciones Técnicas**:

1. **⚡ Fenómeno de polarización cruzada**:
   - Antenas Sniffer tienen orientación fija (A=izquierda, B=derecha)
   - Tags en movimiento cambian orientación de antena respecto a Sniffer
   - Cuando antena Tag queda perpendicular a antena Sniffer → timeouts

2. **📡 Diversidad de antenas funciona PARCIALMENTE**:
   - Con LOS (TEST-05/06): Ant-B compensa falla de Ant-A → R:3 funcional
   - Con NLOS (TEST-07): Ambas antenas se afectan según orientación → R:2 o pérdida

3. **🎯 Mejor configuración para NLOS confirmada**:
   - **PRE_TIMEOUT=8 en AMBOS** (Sniffer y Tag)
   - **HAL_Delay(1) opcional** (no crítico con movimiento)
   - **Antenas AP** (mantener)

4. **🔄 Patrón de alternancia de errores**:
   ```
   Posición 1 (cuerpo bloquea Ant-A Sniffer): 
     → Tag detectado con Ant-B (50 timeouts Ant-A)
   
   Rotación 90° (cuerpo bloquea Ant-B Sniffer):
     → Tag detectado con Ant-A (50 timeouts Ant-B)
   
   Posición frontal (LOS limpio):
     → Tag detectado con ambas antenas (errores mínimos)
   ```

---

**❌ Conclusión TEST-07**:

1. **🎉 DESCARTE DE HIPÓTESIS HARDWARE DEFECTUOSO**:
   - Ant-A funciona perfectamente cuando orientación es favorable
   - Ant-B también falla cuando orientación es desfavorable
   - Problema NO es hardware defectuoso, es **ORIENTACIÓN DE ANTENAS**

2. **📐 Causa raíz identificada: POLARIZACIÓN Y GEOMETRÍA**:
   - Antenas tienen patrón de radiación direccional
   - Obstrucción corporal crea zona de sombra RF
   - Separación de 2m entre Ant-A y Ant-B del Sniffer no es suficiente para diversidad completa

3. **✅ Configuración Sniffer PRE_TIMEOUT=8 VALIDADA**:
   - Mejor sincronización con tags
   - Menor cantidad de timeouts espurios
   - Rango efectivo similar a PRE_TIMEOUT=12 pero más estable

4. **⚠️ TEST-05/06 eran casos extremos (peor caso)**:
   - Línea de vista estática con orientación desfavorable para Ant-A
   - Con movimiento, problema se distribuye entre ambas antenas
   - Confirma que sistema es funcional en condiciones REALES

5. **🔧 Soluciones recomendadas** (orden de prioridad):
   - ✅ **MANTENER configuración actual** (PRE_TIMEOUT=8 ambos lados)
   - 🔄 **Aumentar separación Ant-A/Ant-B** a 3-4m (mejora diversidad espacial)
   - 📡 **Agregar algoritmo de selección dinámica** de antena basado en RSSI
   - 🔀 **Implementar diversidad de polarización** (1 antena vertical, 1 horizontal)
   - 🚫 **NO NECESARIO**: Cambio a 850K data rate ni reemplazo hardware

**🎯 RESULTADO FINAL**: Sistema es **FUNCIONAL** con configuración actual. Problema identificado como **geométrico/orientación**, NO hardware defectuoso. Mejoras posibles son de arquitectura RF (separación antenas, diversidad), no de firmware.

---

### **🔍 Conclusión del Diagnóstico**

Dado que:
1. ✅ **TEST-00**: PRE_TIMEOUT=5 @ 23m → Canal A: 0%, Canal B: 100%
2. ✅ **TEST-01**: PRE_TIMEOUT=8 @ 20m → Canal A: 3%, Canal B: 9.7%
3. ✅ **TEST-01B**: Cambio antenas AP + PRE_TIMEOUT=12 @ 22.3m → Solo 1 antena detecta (dependiente de orientación)
4. ✅ **TEST-02**: Swap antenas → Problema sigue al Canal A lógico (no antena física)
5. ✅ **TEST-03**: Estructuras OTP separadas → Calibraciones válidas en ambos chips, pero Canal A sigue fallando
6. ✅ **TEST-04**: Eliminación HAL_Delay(1) @ 21.7m → Sin mejora, solo Antena B detecta
7. ❌ **Incrementar PRE_TIMEOUT de 5→8→12 (+140%) NO tiene efecto en Canal A**
8. ❌ **Cambio de tipo de antenas (AP → anteriores) NO resuelve el problema**
9. ❌ **Optimización de timing (eliminar HAL_Delay) NO resuelve el problema**

**Hipótesis actual**: **Problema HARDWARE CONFIRMADO en componentes del Canal A**

**Posibles causas técnicas**:
- **LNA degradado** (más probable): Baja sensibilidad (-10 dB típico)
- **Filtro RF desafinado**: Atenúa señal en Canal 5 (6.5 GHz)
- **PCB routing defectuoso**: Impedancia incorrecta en RF de Chip A
- **Cristal/PLL desviado**: Frecuencia ligeramente fuera de spec
- **Chip DW3000 A defectuoso**: Fallo de fábrica

**Test recomendado**: Medir potencia RX con analizador de espectro @ -90 dBm, comparar Canal A vs B (esperado: Canal A ~10 dB menos sensibilidad).

**Opciones restantes**:
1. **TEST-05 (TX Power +1dB)**: Última optimización SW incremental (mejora esperada: <10%)
2. **TEST-07 (850K data rate)**: Solución radical SW (+8dB sensibilidad, puede compensar hardware defectuoso)
3. **Reemplazo de hardware**: Cambiar PCB o chip DW3000 A si TEST-07 falla

---

## 🧪 TESTS PENDIENTES (Optimizaciones Software)

### **TEST-05: Aumentar TX Power** [� PRIORIDAD ALTA]

**Estado**: ⏳ **CONSIDERAR AHORA** - Última optimización SW incremental antes de solución radical (TEST-07)

**Configuración propuesta**:
```c
// En sniffer_tag.cpp y main.cpp línea ~56
static dwt_txconfig_t dwt_tx_cfg = { 
    0x34,       /* PG delay */
    0xFEFEFEFE, /* TX power: +1 dB (Coarse gain=2, Fine gain=63) */
    0x0         /* PG count */
};
```

**¿Por qué ayuda?**: Mayor potencia transmitida (+1 dB) = señal más fuerte en recepción a largas distancias.

**⚠️ ADVERTENCIA**: Aumentar power puede violar límites regulatorios (-41.3 dBm/MHz). Idealmente medir con analizador de espectro antes.

**Procedimiento**:
1. Modificar `TX_POWER` de 0xFDFDFDFD → 0xFEFEFEFE (+1 dB conservador)
2. Compilar y flashear ambos dispositivos
3. Test @ 20m con protocolo similar a TEST-02/03/04
4. Comparar tasas de éxito vs PRE_TIMEOUT=12

**Criterios de éxito**: +5-10% mejora en Canal A. Si Canal A alcanza ≥40% @ 20m → Considerar éxito parcial, pero **probablemente insuficiente** para operación confiable.

**⚠️ Evaluación realista**: Dado que TEST-04 (PRE_TIMEOUT=12) falló, +1dB TX power probablemente NO será suficiente. Considerar ir directamente a TEST-07 (850K).

---

### **TEST-06: RX Diagnostics (First Path Power)** [🟢 PRIORIDAD MEDIA]

**Estado**: ⏸️ **OPCIONAL** - Solo para confirmar diagnóstico de hardware antes de TEST-07

**Objetivo**: Determinar si problema es en TX o RX del Canal A mediante diagnósticos internos del DW3000.

**Implementación**:
```c
// Agregar en sniffer_tag.cpp después de dwt_rxenable()
dwt_rxdiag_t diag;
dwt_readdiagnostics(&diag);
Serial.printf("[DIAG-A] FP_PWR:%d RX_CNT:%d\n", diag.firstPath, diag.rxPreamCount);
```

**Procedimiento**:
1. Agregar logs de diagnóstico para CADA intento de RX
2. Test @ 20m capturando First Path Power y RX Preamble Count
3. Comparar valores Canal A vs Canal B

**Análisis esperado**:
- **FP_PWR Canal A < -100 dBm** → Confirma problema RX sensitivity (LNA defectuoso)
- **FP_PWR Canal A ≈ Canal B** pero RX_CNT bajo → Problema AGC/threshold
- **FP_PWR Canal A muy bajo + Canal B normal** → Confirma hardware defectuoso

---

### **TEST-07: Migración a Data Rate 850K** [� PRIORIDAD CRÍTICA]

**Estado**: ⏳ **SOLUCIÓN RECOMENDADA** - Después de fallar TEST-02, TEST-03, TEST-04, esta es la **única opción SW viable** sin reemplazo de hardware

**¿Por qué considerar esto?**:
- 📡 **+8 dB sensibilidad** vs 6M8 → Puede compensar hardware defectuoso de Canal A
- 🎯 **Rango 50-80m** (2.5-3× mejora) vs 20-25m actual
- 🔧 **Solución definitiva** sin reemplazo de hardware

**⚠️ Trade-offs**:
- ⏱️ **8× más lento**: 2 ms por frame vs 250 µs (latencia 800ms vs 100ms por tag)
- ⚡ **+50% consumo energético** en tags
- 📊 **Menor throughput**: 10 tags/seg vs 80 tags/seg (aceptable para 5-10 tags totales)

**Configuración propuesta**:
```c
static dwt_config_t dwt_cfg_850k = {
    5,          // Channel
    DWT_PLEN_128,
    DWT_PAC8,
    9,
    DWT_BR_850K,  // ← Cambio crítico de 6M8 a 850K
    // ... resto igual
};

// Timeouts ajustados (8× más largos):
#define POLL_TX_TO_RESP_RX_DLY_UUS_850K 5600  // Was 700
#define RESP_RX_TIMEOUT_UUS_850K 2400         // Was 300
#define PRE_TIMEOUT_850K 12                   // Mantener optimización
```

**Esfuerzo**: 1-2 días (cambiar config DWT, ajustar timeouts en ambos firmwares, testing).

**Criterios de decisión**:
- ✅ Si Canal A ≥70% @ 50m → **APLICAR** como solución definitiva
- ⚠️ Si latencia >1 seg/tag es inaceptable → Evaluar modo híbrido (6M8 corto, 850K largo)
- ❌ Si Canal A <30% @ 20m incluso con 850K → **Hardware reemplazo obligatorio**

---

### **TEST-08: Cambio de Canal UWB** [🔵 PRIORIDAD BAJA]

---

### **TEST-01: Separar Estructuras `pdw3000local`** [✅ COMPLETADO - 28-Oct-2025]

**Hipótesis**: Ambos chips DW3000 (Canal A y B) comparten una sola estructura `dwt_local_data_t`, causando que Canal B sobrescriba calibración OTP de Canal A.

**Problema identificado**:
```c
// main.cpp línea 65 y 1117
dwt_local_data_t *pdw3000local;  // ← UNA SOLA estructura para AMBOS chips
```

**Solución implementada**:
- Creadas estructuras separadas: `pdw3000local_a`, `pdw3000local_b`, `pdw3000local` (puntero activo)
- Modificado `init_uwb_device()` para recibir `dwt_local_data_t *local_data`
- Actualizado `switch_hw()` y `switch_hw_timestamp_query()` para cambiar puntero
- Archivos modificados: `main.cpp`, `sniffer_tag.cpp`, `sniffer_tag.hpp`

**Resultado TEST @ 21m**:
```
Canal A: 0% éxito (50/50 RX_PREAMBLE_DETECTION_TIMEOUT)
Canal B: 100% éxito (DistB: 21.16-21.81m, 0 errores)
```

**Verificación OTP**:
```
CANAL A: PartID:0x624831E5  Bias:0x13  DGC_MODE:OTP ✅
CANAL B: PartID:0x6E483065  Bias:0x12  DGC_MODE:OTP ✅
```

**❌ Conclusión**: Hipótesis **INCORRECTA**. Estructuras separadas funcionan correctamente (PartID diferentes, calibraciones únicas), pero Canal A sigue fallando. Problema NO era calibración OTP compartida.

---

### **🔍 Conclusión del Diagnóstico**

Dado que:
1. ✅ TEST-00: Problema sigue al Canal A lógico (no antena física)
2. ✅ TEST-01: Estructuras OTP separadas y calibraciones válidas en ambos chips
3. ❌ Canal A sigue fallando a >20m a pesar de configuración correcta
4. ✅ Canal B funciona perfectamente con cualquier antena

**Hipótesis actual**: **Problema HARDWARE en componentes del Canal A**

**Posibles causas técnicas**:
- **LNA degradado**: Baja sensibilidad (-10 dB típico)
- **Filtro RF desafinado**: Atenúa señal en Canal 5 (6.5 GHz)
- **PCB routing defectuoso**: Impedancia incorrecta en RF de Chip A
- **Cristal/PLL desviado**: Frecuencia ligeramente fuera de spec
- **Chip DW3000 A defectuoso**: Fallo de fábrica

**Test recomendado**: Medir potencia RX con analizador de espectro @ -90 dBm, comparar Canal A vs B (esperado: Canal A ~10 dB menos sensibilidad).

---

## 🧪 TESTS PENDIENTES (Optimizaciones Software)

### **TEST-02: PRE_TIMEOUT=12 @ 20-25m** [🔴 PRIORIDAD CRÍTICA]

**Estado**: ⏳ **CÓDIGO YA IMPLEMENTADO** - Pendiente validación en campo

**Configuración actual**:
```c
// uwb3000Fxx.h (Sniffer y Persona)
PRE_TIMEOUT_6M8 = 12 PAC  // +140% desde baseline (5 PAC)
```

**¿Por qué ayuda?**: Espera más tiempo (96 símbolos) para detectar inicio de preámbulo en señales débiles >20m.

**Resultados históricos**:
- PRE_TIMEOUT=5 @ 23m → Canal A: 0%, Canal B: 100%
- PRE_TIMEOUT=8 @ 20m → Canal A: 3%, Canal B: 9.7%
- PRE_TIMEOUT=12 @ ??? → **PENDIENTE VALIDAR**

**Procedimiento**:
1. Firmware actual ya tiene PRE_TIMEOUT=12 (commit actual)
2. Test @ 20m, 23m, 25m con ≥100 lecturas por distancia
3. Capturar tasas de éxito y logs de errores

**Criterios de éxito**:
- ✅ **Objetivo mínimo**: Canal A ≥30%, Canal B ≥50% @ 20m
- ✅ **Objetivo aceptable**: Ambos ≥50% @ 20m
- ✅ **Ideal**: Ambos ≥70% @ 25m

**Siguiente paso si falla**: Si Canal A <30% @ 20m → Considerar TEST-05 (850K data rate) como única solución viable.

---

### **TEST-03: Aumentar TX Power** [🟡 PRIORIDAD ALTA]

**Estado**: ⏸️ **BLOQUEADO** - Solo ejecutar si TEST-02 muestra Canal A <50% @ 20m

**Configuración propuesta**:
```c
// En sniffer_tag.cpp y main.cpp línea ~56
static dwt_txconfig_t dwt_tx_cfg = { 
    0x34,       /* PG delay */
    0xFEFEFEFE, /* TX power: +1 dB (Coarse gain=2, Fine gain=63) */
    0x0         /* PG count */
};
```

**¿Por qué ayuda?**: Mayor potencia transmitida (+1 dB) = señal más fuerte en recepción a largas distancias.

**⚠️ ADVERTENCIA**: Aumentar power puede violar límites regulatorios (-41.3 dBm/MHz). Idealmente medir con analizador de espectro antes.

**Procedimiento**:
1. Modificar `TX_POWER` de 0xFDFDFDFD → 0xFEFEFEFE (+1 dB conservador)
2. Compilar y flashear ambos dispositivos
3. Re-ejecutar TEST-02 @ 20m
4. Comparar tasas de éxito vs baseline

**Criterios de éxito**: +5-10% mejora en Canal A. Si Canal A alcanza ≥60% @ 20m → Aplicar cambio permanentemente.

---

### **TEST-04: RX Diagnostics (First Path Power)** [🟢 PRIORIDAD MEDIA]

**Estado**: ⏸️ **OPCIONAL** - Solo si TEST-02 y TEST-03 fallan

**Objetivo**: Determinar si problema es en TX o RX del Canal A mediante diagnósticos internos del DW3000.

**Implementación**:
```c
// Agregar en sniffer_tag.cpp después de dwt_rxenable()
dwt_rxdiag_t diag;
dwt_readdiagnostics(&diag);
Serial.printf("[DIAG-A] FP_PWR:%d RX_CNT:%d\n", diag.firstPath, diag.rxPreamCount);
```

**Procedimiento**:
1. Agregar logs de diagnóstico para CADA intento de RX
2. Test @ 20m capturando First Path Power y RX Preamble Count
3. Comparar valores Canal A vs Canal B

**Análisis esperado**:
- **FP_PWR Canal A < -100 dBm** → Confirma problema RX sensitivity (LNA defectuoso)
- **FP_PWR Canal A ≈ Canal B** pero RX_CNT bajo → Problema AGC/threshold
- **FP_PWR Canal A muy bajo + Canal B normal** → Confirma hardware defectuoso

---

### **TEST-05: Migración a Data Rate 850K** [� PRIORIDAD MEDIA-ALTA]

**Estado**: ⏸️ **SOLUCIÓN RADICAL** - Considerar si TEST-02 muestra Canal A <30% @ 20m

**¿Por qué considerar esto?**:
- 📡 **+8 dB sensibilidad** vs 6M8 → Puede compensar hardware defectuoso de Canal A
- 🎯 **Rango 50-80m** (2.5-3× mejora) vs 20-25m actual
- 🔧 **Solución definitiva** sin reemplazo de hardware

**⚠️ Trade-offs**:
- ⏱️ **8× más lento**: 2 ms por frame vs 250 µs (latencia 800ms vs 100ms por tag)
- ⚡ **+50% consumo energético** en tags
- 📊 **Menor throughput**: 10 tags/seg vs 80 tags/seg (aceptable para 5-10 tags totales)

**Configuración propuesta**:
```c
static dwt_config_t dwt_cfg_850k = {
    5,          // Channel
    DWT_PLEN_128,
    DWT_PAC8,
    9,
    DWT_BR_850K,  // ← Cambio crítico de 6M8 a 850K
    // ... resto igual
};

// Timeouts ajustados (8× más largos):
#define POLL_TX_TO_RESP_RX_DLY_UUS_850K 5600  // Was 700
#define RESP_RX_TIMEOUT_UUS_850K 2400         // Was 300
#define PRE_TIMEOUT_850K 12                   // Mantener optimización
```

**Esfuerzo**: 1-2 días (cambiar config DWT, ajustar timeouts en ambos firmwares, testing).

**Criterios de decisión**:
- ✅ Si Canal A ≥70% @ 50m → **APLICAR** como solución definitiva
- ⚠️ Si latencia >1 seg/tag es inaceptable → Evaluar modo híbrido (6M8 corto, 850K largo)
- ❌ Si Canal A <30% @ 20m incluso con 850K → **Hardware reemplazo obligatorio**

---

### **TEST-06: Cambio de Canal UWB** [🔵 PRIORIDAD BAJA]

**Estado**: ⏸️ **ÚLTIMA OPCIÓN SW** - Solo si hay sospecha de interferencia en Canal 5

**Configuración propuesta**:
```c
// Cambiar de Canal 5 (6.5 GHz) a Canal 9 (8.0 GHz)
static dwt_config_t dwt_cfg = {
    9,          // Channel number (antes: 5)
    DWT_PLEN_128,
    DWT_PAC8,
    9,          // Preamble code compatible con Canal 9
    // ... resto sin cambios
};
```

**⚠️ Consideración**: Canal 9 tiene ~0.5 dB menos sensibilidad que Canal 5. Solo cambiar si interferencia confirmada con spectrum analyzer.

**Procedimiento**:
1. Capturar log RF de Canal 5 (si analizador disponible)
2. Si detecta interferencias > -90 dBm → Cambiar a Canal 9
3. Re-ejecutar TEST-02 @ 20m con Canal 9
4. Comparar tasas de éxito

**Resultado esperado**: Si interferencia era causa, mejora ≥20% en ambos canales

---

## � RESUMEN EJECUTIVO DE TESTS

### ✅ **Diagnóstico Completado (Orden Cronológico)**

| Test | Fecha | Configuración | Resultado @ Distancia |
|------|-------|---------------|----------------------|
| **TEST-00** | 23-Oct | PRE_TIMEOUT=5 (baseline) | Canal A: 0% @ 23m, Canal B: 100% @ 23m |
| **TEST-01** | 23-Oct | PRE_TIMEOUT=8 (+60%) | Canal A: 3% @ 20m, Canal B: 9.7% @ 20m |
| **TEST-01B** | 27-Oct | Cambio antenas AP + PRE_TIMEOUT=12 | Solo 1 antena detecta @ 22.3m (dependiente orientación) |
| **TEST-02** | 28-Oct | Swap físico antenas | Problema sigue en Canal A lógico ✅ |
| **TEST-03** | 28-Oct | Estructuras OTP separadas | Hipótesis incorrecta ❌ |
| **TEST-04** | 28-Oct | Eliminación HAL_Delay(1) en Persona | Sin mejora, solo Antena B detecta @ 21.7m ❌ |

**Conclusión**: 
- ❌ **Problema hardware CONFIRMADO** en Canal A
- ❌ Optimización PRE_TIMEOUT **AGOTADA** (5→8→12 no tiene efecto)
- ❌ Cambio de tipo de antenas **NO resuelve el problema**
- ❌ Eliminación de delays en código **NO resuelve el problema**
- ✅ Todas las optimizaciones SW probadas demuestran que el problema es HARDWARE
- 🔴 **Solución SW radical (TEST-07: 850K) o reemplazo hardware son las únicas opciones**

---

### ⏳ **Tests Pendientes (Priorizados)**

| Test | Prioridad | Estado | Objetivo | Esfuerzo |
|------|-----------|--------|----------|----------|
| **TEST-05** | � **ALTA** | ⏳ Considerar | TX Power +1dB (mejora esperada <10%) | 2h (código + test) |
| **TEST-06** | 🟢 **MEDIA** | ⏸️ Opcional | RX Diagnostics para confirmar hardware | 3h (código + análisis) |
| **TEST-07** | � **CRÍTICA** | ⏳ **RECOMENDADO** | Migrar a 850K (+8dB sensibilidad) | 1-2 días |
| **TEST-08** | 🔵 **BAJA** | ⏸️ Última opción | Cambio Canal 5→9 (interferencia) | 1h (código + test) |

---

## 🎯 PLAN DE ACCIÓN INMEDIATO

### **Opción A: TEST-05 (conservadora, baja probabilidad de éxito)**
⚠️ TX Power +1dB @ 20m  
📊 Mejora esperada: +5-10% en Canal A  
⏱️ Si Canal A sigue <30% → Ir directamente a TEST-07

### **Opción B: TEST-07 (recomendada, mayor probabilidad de éxito)**
✅ Migrar a 850K data rate  
📡 +8 dB sensibilidad → **Puede compensar hardware defectuoso**  
🎯 Rango objetivo: 50-80m (vs 20-25m actual)  
⏱️ Esfuerzo: 1-2 días de implementación

**Si TEST-07 éxito (Canal A ≥70% @ 50m)**:
- ✅ Problema RESUELTO sin reemplazo de hardware
- 📝 Documentar configuración final 850K
- 🔀 Merge a branch `dev`
- ⚠️ Aceptar trade-offs: latencia 8× mayor (800ms vs 100ms)

**Si TEST-07 falla (Canal A <30% @ 20m incluso con 850K)**:
- ❌ **Hardware reemplazo OBLIGATORIO**
- 🔧 Opciones: Cambiar chip DW3000 A, reemplazar PCB completo, o validar LNA/filtro RF
- 💰 Costo estimado: $20-50 (chip) o $200-500 (PCB completo)
  
---

---

## 📊 RESULTADO FINAL

### ✅ **Éxito Global**
- [ ] **OBJETIVO ALCANZADO**: Detección estable ≥70% a 25m
- [ ] **PARCIAL**: Funciona pero <70%
- [ ] **FALLIDO**: Sin mejora significativa

### 🔧 **Configuración Final**
```c
// Valores finales implementados (actualizar después de tests)
POLL_TX_TO_RESP_RX_DLY_UUS_6M8 = _____  // Ambos equipos
RESP_RX_TIMEOUT_UUS_6M8 = _____         // Ambos equipos
PRE_TIMEOUT_6M8 = _____                 // Ambos equipos (baseline: 12 PAC)
query_timeout = _____ ms                // Sniffer (baseline: 1000 ms)
TX_POWER = 0x_______                    // Si se modificó (baseline: 0xFDFDFDFD)
Channel = _____                         // Si se cambió (baseline: 5)
```

### 📝 **Resumen de Cambios**
- [ ] TEST-00 (Swap) ejecutado → Resultado: [ ] Hardware [ ] Config
- [ ] TEST-01 implementado (PRE_TIMEOUT)
- [ ] TEST-02 implementado (TX Power)
- [ ] TEST-03 implementado (Query timeout)
- [ ] TEST-04 implementado (Validación)
- [ ] TEST-05 implementado (Logging)
- [ ] TEST-06 implementado (MULTIPLE only)
- [ ] TEST-07 implementado (solo si necesario)
- [ ] TEST-08 implementado (solo si necesario)

---

## 📋 CHECKLIST DE EJECUCIÓN

### Antes de Tests
- [ ] Preparación hardware/software completa
- [ ] Commit baseline realizado
- [ ] Logs UART configurados

### Durante Tests
- [ ] 3 minutos por distancia en TEST-01
- [ ] 20-30 ciclos por escenario
- [ ] Logs guardados por sesión
- [ ] Fotos/videos de setup

### Después de Tests
- [ ] Commit final con cambios
- [ ] Tag `v0.4.0-fix-distance`
- [ ] CHANGELOG.md actualizado
- [ ] Merge a `dev`

---

## 📝 NOTAS RÁPIDAS

**Problemas encontrados**:
```

```

**Lecciones aprendidas**:
```

```

**Recomendaciones**:
```

```

---

**Completado el**: ___________ por: ___________
