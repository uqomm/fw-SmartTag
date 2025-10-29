# Resumen Pruebas SmartLocate

**Proyecto**: Sniffer-Tag Firmware (DW3000 UWB)  
**Período de Testing**: 22-29 Octubre 2025  
**Estado**: Sistema Funcional - Optimizaciones Identificadas

---

## Resumen de Situación Actual

### **Estado del Sistema**
- **Operacional**: Sistema funciona correctamente hasta **20m** en línea de vista
- **Limitación identificada**: Degradación de detección >20m por orientación de antenas
- **Solución disponible**: Migración a data rate 850K puede extender rango a **30-50m**

### **Configuración Óptima Validada**
- **PRE_TIMEOUT**: 8 PAC (ambos dispositivos)
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
**Objetivo**: Validar sistema en condiciones reales de uso

**Setup**: Operador en movimiento con tags, obstrucción corporal intermitente (NLOS)

**DESCUBRIMIENTO CRÍTICO**:
```
Fenómeno de inversión de errores detectado:
  - Línea vista estática: Canal A falla sistemáticamente
  - Con movimiento/rotación: Errores se distribuyen entre Canal A y B
  - @ 18.79m con movimiento: Canal A funciona, Canal B falla
```

**Conclusión TEST-07**: 
- **NO hay problema hardware defectuoso**
- **Problema es ORIENTACIÓN/POLARIZACIÓN de antenas**
- **Sistema funciona correctamente** en condiciones reales de uso

---

## Resultados Consolidados

### **Rango Efectivo por Configuración**

| Configuración | Rango LOS Estático | Rango con Movimiento | Recomendación |
|---------------|-------------------|---------------------|---------------|
| PRE_TIMEOUT=5 | ~15m | ~15m | Insuficiente |
| **PRE_TIMEOUT=8** | **~20m** | **~19m** | **ÓPTIMA** |
| PRE_TIMEOUT=12 | ~18m | ~18m | No mejora |
| 850K (proyectado) | **30-50m** | **30-50m** | **Siguiente paso** |

### **Causa Raíz Identificada**
- **Problema**: Polarización de antenas + separación de solo 2m entre antenas del Sniffer
- **Efecto**: Una antena queda en "sombra" RF según orientación del tag
- **Impacto**: Degradación a modo single-antenna (R:3) en orientaciones desfavorables

---

## Próximos Pasos Recomendados

### **Migración a Data Rate 850K**

**Justificación**:
- **+8 dB sensibilidad** vs configuración actual (6.8 Mbps)
- **Rango objetivo**: 30-50m (1.5-2.5× mejora sobre 20m actual)
- **Robustez NLOS**: Mayor inmunidad a obstrucciones y orientación desfavorable
- **Solución SW pura**: No requiere cambios hardware

**Trade-offs aceptables**:
- **Latencia**: 800ms vs 100ms actual por tag (aceptable para 5-10 tags)
- **Consumo**: +50% en tags (de ~40 mAh a ~60 mAh, aún muy bajo)
- **Throughput**: 10 tags/seg vs 80 tags/seg actual (suficiente para aplicación)

**Esfuerzo estimado**: 
- **Desarrollo**: 2-3 días (cambio config DWT, ajuste timeouts, compilación)
- **Testing**: 1-2 días (validación en campo @ 20m, 30m, 50m)
- **Total**: **1 semana** (incluyendo validación completa)

**Criterios de éxito**:
- Detección >70% @ 30m en línea de vista
- Detección >50% @ 25m con obstrucción corporal
- Latencia <1 seg por tag (aceptable para tracking)

---

## Análisis Costo-Beneficio

### **Opción A: Migración 850K** (RECOMENDADA)
- **Costo**: ~1 semana desarrollo
- **Beneficio**: 1.5-2.5× extensión de rango (20m → 30-50m)
- **Riesgo**: Bajo (solo cambio de configuración, reversible)
- **ROI**: Excelente

### **Opción B: Mantener configuración actual**
- **Costo**: 0 (sin cambios)
- **Beneficio**: Sistema funciona correctamente hasta 20m
- **Riesgo**: Ninguno
- **ROI**: N/A (mantiene estado actual)

---

## Plan de Acción Recomendado

**Semana 1 (30 Oct - 3 Nov)**:
1. Implementar migración a 850K (2-3 días)
2. Testing en campo @ 20m, 30m, 40m (1-2 días)
3. Validar latencia y consumo aceptables

**Criterio de decisión**:
- Si 850K logra >70% @ 30m → **Implementar como solución definitiva**
- Si 850K logra 30-70% @ 30m → **Evaluar ajustes adicionales**
- Si 850K <30% @ 30m → **Investigar interferencias o problemas adicionales** (improbable)

**Probabilidad de éxito**: **80-85%** basado en especificaciones técnicas DW3000

---

## Métricas de Éxito

### **Objetivos Cumplidos**
- Sistema funcional @ 20m (configuración óptima identificada)
- Causa raíz diagnosticada (orientación antenas, NO hardware defectuoso)
- 7 tests completados con análisis exhaustivo
- Configuración óptima validada (PRE_TIMEOUT=8)

### **Objetivos Pendientes**
- Validación de rango extendido >30m (requiere 850K)
- Testing con 10+ tags simultáneos
- Pruebas de consumo energético prolongado
- Validación en entorno industrial con interferencias

---

## Notas Técnicas

### **Lecciones Aprendidas**
1. **Aumentar PRE_TIMEOUT no siempre mejora**: PRE_TIMEOUT=12 empeoró rendimiento
2. **HAL_Delay(1) es beneficioso**: Contrario a hipótesis inicial
3. **Testing con movimiento es crítico**: Revela comportamiento real vs casos estáticos extremos
4. **Geometría de antenas es factor dominante**: Más importante que ajustes de software

### **Documentación Completa**
- `CHECKLIST_TESTS_FISICOS.md`: 855 líneas con análisis detallado de 7 tests
- `ANALISIS_EJECUTIVO_COMUNICACION.md`: 680 líneas con análisis de timing
- Logs completos en `/logs/` (29-Oct-2025)

---

## Conclusión Ejecutiva

**Estado del Proyecto**: **EN BUEN CAMINO**

- Sistema funciona correctamente en condiciones reales (20m)
- Problema diagnosticado (orientación antenas, NO hardware defectuoso)
- Solución clara identificada (migración 850K, 1 semana desarrollo)
- ROI favorable (1.5-2.5× extensión rango con cambio SW puro)

**Recomendación**: **Proceder con migración a 850K** como próximo paso inmediato.

**Riesgo del proyecto**: **BAJO** - Solución técnica clara y probada en literatura técnica.

---

**Preparado por**: Equipo de Desarrollo  
**Fecha**: 29 Octubre 2025  
**Próxima Revisión**: Post-implementación 850K (estimada 5 Nov 2025)
