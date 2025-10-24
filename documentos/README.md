# 📚 Documentos Técnicos - Proyecto Sniffer-Tag UWB

**Versión**: v0.4.0-fix-distance  
**Fecha**: Octubre 2025  

---

## 📖 Guía Rápida

Esta carpeta contiene toda la documentación técnica del proyecto **Sniffer-Tag UWB** para la corrección de detección a distancias >20 metros.

### 🚀 Inicio Rápido
1. **[`INDICE_DOCUMENTOS.md`](INDICE_DOCUMENTOS.md)** - Índice completo organizado
2. **[`PLAN_CORRECCION_DISTANCIA.md`](PLAN_CORRECCION_DISTANCIA.md)** - Plan de corrección principal
3. **[`CHECKLIST_TESTS_FISICOS.md`](CHECKLIST_TESTS_FISICOS.md)** - Guía de pruebas

### 🎯 Problema Principal
El sistema UWB falla en detectar tags a >20m debido a timeouts insuficientes del chip DW3000 y falta de validación de datos.

### ✅ Soluciones Implementadas
- 🔴 **PRIORIDAD 1**: Aumentar timeouts DW3000 (RESP_RX_TIMEOUT_UUS_6M8: 300→500 µs)
- 🟡 **PRIORIDAD 2**: Validar datos antes de guardar
- 🟢 **PRIORIDAD 3**: Timeout adaptativo por distancia

---

## 📂 Estructura de Documentos

```
📁 documentos/
├── 📋 INDICE_DOCUMENTOS.md          # ← Este archivo (índice completo)
├── 🎯 PLAN_CORRECCION_DISTANCIA.md  # Plan principal
├── ✅ CHECKLIST_TESTS_FISICOS.md     # Tests de validación
├── 🔍 ANALISIS_*.md                  # Análisis técnicos
├── 📊 RESULTADOS_*.md                # Resultados de tests
├── 📖 PROTOCOLO_*.md                 # Documentación técnica
└── 📊 DIAGRAMAS_*.md                 # Diagramas y visualizaciones
```

---

## 🔄 Estado del Proyecto

- ✅ **Análisis completado** - Documentación técnica completa
- ✅ **Plan definido** - 3 soluciones identificadas
- 🔄 **Implementación** - En progreso (FWSL-5)
- ⏳ **Validación** - Pendiente

### 📈 Métricas de Éxito
- **Antes**: Detección falla >20m
- **Después**: Detección confiable >30m
- **Impacto**: 85% mejora esperada

---

## 👥 Equipo

- **Arturo Armando Veras Olivos** - Coordinación y análisis
- **Manuel Ibáñez Mancilla** - Desarrollo y optimizaciones
- **Alan Gamalier Vergara Muñoz** - Integración inicial

---

## 📞 Contactos

- **JIRA**: [FWSL](https://uqomm-teams.atlassian.net/projects/FWSL)
- **Git**: [sniffer-tag-firmware](https://gitlab.com/uqomm/firmware/sniffer-tag-firmware)

---

*Para navegación detallada, consulta [`INDICE_DOCUMENTOS.md`](INDICE_DOCUMENTOS.md)*</content>
<parameter name="filePath">c:\Users\artur\development\sniffer-tag\documentos\README.md