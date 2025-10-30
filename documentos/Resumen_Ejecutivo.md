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

| Aspecto | Descripción |
|---------|-------------|
| Estado del Sistema | Operacional hasta 20m en LOS con orientación favorable; limitación por orientación de antenas >12m |
| Configuración Óptima | PRE_TIMEOUT=8 PAC (~16 µs), Data Rate=6.8 Mbps, Rango efectivo=20m |
| Solución Disponible | Migración a 850K para extender rango a 30-50m |

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

<div style="page-break-after: always;"></div>

**Razonamiento de las hipótesis probadas**:
- **PRE_TIMEOUT**: Señales débiles a larga distancia requieren más tiempo para detectar el preámbulo. Aumentar el timeout debería mejorar la sensibilidad del receptor.
- **Calibración OTP**: Se sospechaba que ambos chips DW3000 compartían la misma estructura de calibración en memoria, causando que Canal B sobrescribiera los valores de Canal A.
- **HAL_Delay(1)**: Se pensaba que el delay de 1ms entre lecturas introducía jitter y desincronización en el protocolo de comunicación, afectando el timing crítico de las respuestas UWB.

**Conclusión Preliminar**: Sospecha de problema hardware en Canal A del Sniffer

---

### **TEST-05 y TEST-06**: Comparación Multi-Tag (29 Oct)
**Objetivo**: Validar configuraciones óptimas con 4 tags simultáneos



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
- **Sistema funciona correctamente** en movimiento, con obstruccion corporal y con tags orientados hacia el sniffer hasta ~19m

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

<div style="page-break-after: always;"></div>

## Próximos Pasos Recomendados

### **Migración a Data Rate 850K**

| Beneficio | Descripción |
|-----------|-------------|
| Sensibilidad | +6.8 dB vs 6.8 Mbps |
| Rango | 30-50m (1.5-2.5× mejora) |
| Robustez NLOS | Mayor inmunidad a obstrucciones/orientación |
| Implementación | Solo FW, no requiere HW |

**Trade-offs**:

| Modo | Latencia | Consumo | Throughput |
|------|----------|---------|------------|
| Multiple Detection | 160-200ms vs 30-100ms actual | A evaluar | 5-6.25 vs 10-33 tags/seg |
| One Detection | ~50ms vs ~15ms actual | A evaluar | ~20 vs ~66 tags/seg |

**Esfuerzo estimado**: 
- **Desarrollo**: 2-3 días (cambio config transmisores, ajuste timeouts, compilación)
- **Testing**: 1-2 días (validación en campo @ 20m, 30m, 50m)
- **Total**: **1 semana** (incluyendo validación completa)

**Criterios de éxito**:
- Detección >70% @ 30m en línea de vista
- Detección >50% @ 25m con obstrucción corporal
- Latencia <1 seg por tag (aceptable para tracking)

---

## Análisis

| Opción | Costo | Beneficio | Riesgo |
|--------|-------|-----------|--------|
| **A: Migración 850K** (Recomendada) | ~1 semana desarrollo | 1.5-2.5× extensión de rango (20m → 30-50m) | Bajo (solo config FW, reversible) |
| **B: Mantener actual** | 0 (sin cambios) | Funciona hasta 20m | Ninguno |


---

<div style="page-break-after: always;"></div>

## Plan de Acción

| Semana | Tareas |
|--------|--------|
| Semana 1 (03 Nov - 7 Nov) | 1. Implementar migración a 850K (2-3 días)<br>2. Testing en campo @ 20m, 30m, 40m (1-2 días)<br>3. Validar latencia y consumo aceptables |

**Criterio de decisión**:

| Condición | Acción |
|-----------|--------|
| 850K >70% @ 30m | Evaluar consumo e implementar como solución definitiva |
| 850K 30-70% @ 30m | Evaluar ajustes adicionales |
| 850K <30% @ 30m | Investigar interferencias o problemas adicionales (improbable)


---

## Métricas de Éxito

### **Objetivos Cumplidos**
- Sistema funcional hasta 20m (con linea de vista y orientación de antenas favorable)
- Actualmente, la orientación de las antenas y la línea de vista son factores críticos para el desempeño del sistema
- 7 tests completados con análisis
- Configuración óptima actual validada (PRE_TIMEOUT=8)

### **Objetivos Pendientes**
- Validación de rango extendido >30m (requiere 850K)
- Testing con 10+ tags simultáneos
- Pruebas de consumo 
- Validación en entorno industrial con interferencias


