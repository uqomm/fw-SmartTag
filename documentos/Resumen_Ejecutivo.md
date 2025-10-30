# Resumen Pruebas SmartLocate
 
**Período de Testing**: 22-29 Octubre 2025  
**Estado**: Sistema Funcional - Optimizaciones Identificadas

---

## Definiciones

- **LOS** (Line Of Sight): Línea de vista directa sin obstrucciones entre emisor y receptor
- **NLOS** (Non Line Of Sight): Sin línea de vista directa, con obstrucciones (paredes, personas, objetos)
- **OTP** (One-Time Programmable): Memoria de calibración de fábrica del chip, programada una sola vez
- **PAC** (Preamble Acquisition Chunk): Unidad de tiempo que el receptor espera para detectar el inicio de una señal.

---

## Resumen de Situación Actual

### **Estado del Sistema**
- **Operacional**: Sistema funciona correctamente hasta **20m** en línea de vista
- **Limitación identificada**: Degradación de detección >20m por orientación de antenas
- **Solución disponible**: Migración a data rate 850K podría extender rango a **30-50m**

### **Configuración Óptima Validada**
- **PRE_TIMEOUT**: 8 PAC (ambos dispositivos) -  (~16 µs)
- **Data Rate actual**: 6.8 Mbps
- **Rango efectivo**: 20m (línea de vista con orientación favorable)

---

## Tests Realizados (22-29 Oct 2025)

### **TEST-00 a TEST-04**: Diagnóstico Inicial (23-28 Oct)
**Objetivo**: Identificar causa de falla de detección >20m

| Test | Fecha | Hipótesis Probada | Resultado |
|------|-------|-------------------|-----------|
| TEST-00 | 23-Oct | Baseline PRE_TIMEOUT=5 | Falla @ 23m Canal A |
| TEST-01 | 23-Oct | Aumentar PRE_TIMEOUT a 8 | Mejora parcial (3% Canal A) |
| TEST-02 | 28-Oct | Problema en antena física | Descartado |
| TEST-03 | 28-Oct | Calibración OTP compartida | Descartado |
| TEST-04 | 28-Oct | Delays en código | Descartado |

**Razonamiento de las hipótesis probadas**:
- **PRE_TIMEOUT**: Señales débiles a larga distancia requieren más tiempo para detectar el preámbulo. Aumentar el timeout debería mejorar la sensibilidad del receptor.
- **Calibración OTP**: Se sospechaba que ambos chips DW3000 compartían la misma estructura de calibración en memoria, causando que Canal B sobrescribiera los valores de Canal A.
- **HAL_Delay(1)**: Se pensaba que el delay de 1ms entre lecturas introducía jitter y desincronización en el protocolo de comunicación, afectando el timing crítico de las respuestas UWB.

**Conclusión Preliminar**: Sospecha de problema hardware en Canal A del Sniffer

---

### **TEST-05 y TEST-06**: Comparación Multi-Tag (29 Oct)
**Objetivo**: Validar configuraciones óptimas con 4 tags simultáneos

**Configuraciones probadas**:
```
Tag 0x2B45: PRE_TIMEOUT=8 + HAL_Delay(1)  → Mejor rendimiento
Tag 0x2B5E: PRE_TIMEOUT=8 sin HAL_Delay   → Bueno con errores
Tag 0x2783: PRE_TIMEOUT=12 sin HAL_Delay  → Empeora >20m
Tag 0x29EC: PRE_TIMEOUT=5 + HAL_Delay(1)  → Rango reducido 33%
```

**Hallazgos clave**:
- PRE_TIMEOUT=8 es configuración óptima
- PRE_TIMEOUT=12 NO mejora (contradice expectativa inicial)
- HAL_Delay(1) mejora estabilidad (contradice hipótesis inicial)

---

### **TEST-07**: Prueba con Movimiento y Obstrucción (29 Oct)
**Objetivo**: Validar sistema en movimiento y con obstrucción corporal

**Setup**: Operador en movimiento con tags, obstrucción corporal intermitente (NLOS)

**Conclusión TEST-07**: 
- **NO hay problema hardware defectuoso**
- **Problema es ORIENTACIÓN/POLARIZACIÓN de antenas**
- **Sistema funciona correctamente** en movimiento y con obstruccion corporal hasta ~19m

---

## Resultados Consolidados

### **Rango Efectivo por Configuración**

| Configuración | Rango LOS Estático | Rango con Movimiento | Recomendación |
|---------------|-------------------|---------------------|---------------|
| PRE_TIMEOUT=5 | ~15m | ~15m | Insuficiente |
| **PRE_TIMEOUT=8** | **~20m** | **~19m** | **ÓPTIMA** |
| PRE_TIMEOUT=12 | ~18m | ~18m | No mejora |
| 850K (proyectado) | **30-50m** | **30-50m** | **Siguiente paso** |


---

## Próximos Pasos Recomendados

### **Migración a Data Rate 850K**

**Justificación**:
- **+8 dB sensibilidad** vs configuración actual (6.8 Mbps)
- **Rango objetivo**: 30-50m (1.5-2.5× mejora sobre 20m actual)
- **Robustez NLOS**: Mayor inmunidad a obstrucciones y orientación desfavorable
- **Solución FW**: No requiere cambios hardware

**Trade-offs aceptables (en modo multiple detection)**:
- **Latencia**: 500-600ms vs 85-100ms actual por tag (aceptable para 5-10 tags)
- **Consumo**: +50% en tags (de ~40 mAh a ~60 mAh, aún muy bajo)
- **Throughput**: 15-20 tags/seg vs 80-100 tags/seg actual (suficiente para aplicación)

**Trade-offs aceptables (en modo one detection)**:
- **Latencia**: ~10-15ms vs ~2ms actual por medición (3 transmisiones: Sniffer→Tag→Sniffer)
- **Consumo**: Similar (+50% por transmisión, pero menos transmisiones totales)
- **Throughput**: ~100 mediciones/seg vs ~500 mediciones/seg actual

**Esfuerzo estimado**: 
- **Desarrollo**: 2-3 días (cambio config DWT, ajuste timeouts, compilación)
- **Testing**: 1-2 días (validación en campo @ 20m, 30m, 50m)
- **Total**: **1 semana** (incluyendo validación completa)

**Criterios de éxito**:
- Detección >70% @ 30m en línea de vista
- Detección >50% @ 25m con obstrucción corporal
- Latencia <1 seg por tag (aceptable para tracking)

---

## Análisis

### **Opción A: Migración 850K** (RECOMENDADA)
- **Costo**: ~1 semana desarrollo
- **Beneficio**: 1.5-2.5× extensión de rango (20m → 30-50m)
- **Riesgo**: Bajo (solo cambio de configuración, reversible)


### **Opción B: Mantener configuración actual**
- **Costo**: 0 (sin cambios)
- **Beneficio**: Sistema funciona correctamente hasta 20m
- **Riesgo**: Ninguno


---

## Plan de Acción

**Semana 1 (30 Oct - 3 Nov)**:
1. Implementar migración a 850K (2-3 días)
2. Testing en campo @ 20m, 30m, 40m (1-2 días)
3. Validar latencia y consumo aceptables

**Criterio de decisión**:
- Si 850K logra >70% @ 30m → **Implementar como solución definitiva**
- Si 850K logra 30-70% @ 30m → **Evaluar ajustes adicionales**
- Si 850K <30% @ 30m → **Investigar interferencias o problemas adicionales** (improbable)


---

## Métricas de Éxito

### **Objetivos Cumplidos**
- Sistema funcional @ 20m (configuración óptima identificada)
- Causa raíz diagnosticada (orientación antenas, NO hardware defectuoso)
- 7 tests completados con análisis
- Configuración óptima validada (PRE_TIMEOUT=8)

### **Objetivos Pendientes**
- Validación de rango extendido >30m (requiere 850K)
- Testing con 10+ tags simultáneos
- Pruebas de consumo energético prolongado
- Validación en entorno industrial con interferencias


