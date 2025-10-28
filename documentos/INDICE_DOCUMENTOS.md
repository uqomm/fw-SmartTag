# 📚 Índice de Documentos Técnicos - Proyecto Sniffer-Tag UWB

**Última actualización**: Octubre 2025  
**Proyecto**: Firmware Sniffer-Tag UWB  
**Versión**: v0.4.0-fix-distance  

---

## 🎯 **DOCUMENTOS PRINCIPALES**

### 📋 Plan de Corrección
- **[`PLAN_CORRECCION_DISTANCIA.md`](PLAN_CORRECCION_DISTANCIA.md)** - Plan completo para solucionar detección >20m
- **[`CHECKLIST_TESTS_FISICOS.md`](CHECKLIST_TESTS_FISICOS.md)** - ⭐ **HOJA DE RUTA SIMPLIFICADA**: 5 enfoques prioritarios + tests validación

### 📖 Protocolos y Esquemas
- **[`PROTOCOLO_UWB_COMPLETO.md`](PROTOCOLO_UWB_COMPLETO.md)** - Protocolo completo Sniffer ↔ Tag
- **[`ESQUEMA_COMUNICACION_UWB.md`](ESQUEMA_COMUNICACION_UWB.md)** - Diagrama de comunicación UWB

---

## 🔍 **ANÁLISIS TÉCNICOS**

### ⏱️ Timeouts y Temporización
- **[`PARAMETROS_UWB_COMPLETOS.md`](PARAMETROS_UWB_COMPLETOS.md)** - ⭐ **REFERENCIA COMPLETA** de todos los parámetros UWB (6.8M + 850K)
- **[`ANALISIS_INTEGRADO_TIMEOUTS_Y_LOGICA.md`](ANALISIS_INTEGRADO_TIMEOUTS_Y_LOGICA.md)** - Análisis integrado de timeouts
- **[`ANALISIS_IMPACTO_TIEMPOS_UWB.md`](ANALISIS_IMPACTO_TIEMPOS_UWB.md)** - Impacto de tiempos en rendimiento
- **[`ANALISIS_RX_TIMEOUT_MS.md`](ANALISIS_RX_TIMEOUT_MS.md)** - Análisis específico de RX_TIMEOUT
- **[`ANALISIS_LOGICA_SIN_RX_TIMEOUT.md`](ANALISIS_LOGICA_SIN_RX_TIMEOUT.md)** - Lógica sin timeout RX
- **[`RELACION_TIMEOUTS_UWB.md`](RELACION_TIMEOUTS_UWB.md)** - Relaciones entre timeouts

### 📊 Datos y Validación
- **[`ANALISIS_DISCORDANCIA_LECTURAS_UWB.md`](ANALISIS_DISCORDANCIA_LECTURAS_UWB.md)** - Análisis de discordancias
- **[`REPORTE_DISCORDANCIAS.md`](REPORTE_DISCORDANCIAS.md)** - Reporte detallado de discordancias

### 📝 Logging y Monitoreo
- **[`ANALISIS_LOGGING_ACTUAL.md`](ANALISIS_LOGGING_ACTUAL.md)** - Análisis del sistema de logging actual
- **[`SOLUCION_LOGGING_DIFERIDO.md`](SOLUCION_LOGGING_DIFERIDO.md)** - Solución de logging diferido
- **[`RESULTADOS_LOGGING_DIFERIDO_ANALISIS.md`](RESULTADOS_LOGGING_DIFERIDO_ANALISIS.md)** - Resultados del análisis

---

## 📈 **RESULTADOS Y REPORTES**

### ✅ Resultados de Tests
- **[`1.-Resultados_timeout_cambiado.md`](1.-Resultados_timeout_cambiado.md)** - Resultados de cambio de timeout

---

## 📊 **DIAGRAMAS Y VISUALIZACIONES**

### 🔄 Diagramas de Estado
- **[`DIAGRAMAS_ESTADO_MERMAID.md`](DIAGRAMAS_ESTADO_MERMAID.md)** - Diagramas Mermaid de estados
- **[`DIAGRAMAS_ESTADO_MERMAID.html`](DIAGRAMAS_ESTADO_MERMAID.html)** - Versión HTML interactiva

---

## 🗂️ **ORGANIZACIÓN POR TEMAS**

### 🚨 **Problema Principal: Detección >20m**
1. [`PLAN_CORRECCION_DISTANCIA.md`](PLAN_CORRECCION_DISTANCIA.md) *(Plan de solución)*
2. [`ANALISIS_INTEGRADO_TIMEOUTS_Y_LOGICA.md`](ANALISIS_INTEGRADO_TIMEOUTS_Y_LOGICA.md) *(Análisis técnico)*
3. [`PROTOCOLO_UWB_COMPLETO.md`](PROTOCOLO_UWB_COMPLETO.md) *(Referencia técnica)*
4. [`CHECKLIST_TESTS_FISICOS.md`](CHECKLIST_TESTS_FISICOS.md) *(Validación)*

### ⏱️ **Sistema de Timeouts**
1. [`PARAMETROS_UWB_COMPLETOS.md`](PARAMETROS_UWB_COMPLETOS.md) ⭐ *(Referencia completa - EMPEZAR AQUÍ)*
2. [`RELACION_TIMEOUTS_UWB.md`](RELACION_TIMEOUTS_UWB.md) *(Relaciones)*
3. [`ANALISIS_IMPACTO_TIEMPOS_UWB.md`](ANALISIS_IMPACTO_TIEMPOS_UWB.md) *(Impacto)*
4. [`ANALISIS_RX_TIMEOUT_MS.md`](ANALISIS_RX_TIMEOUT_MS.md) *(RX específico)*
5. [`ANALISIS_LOGICA_SIN_RX_TIMEOUT.md`](ANALISIS_LOGICA_SIN_RX_TIMEOUT.md) *(Lógica alternativa)*

### 📊 **Validación de Datos**
1. [`ANALISIS_DISCORDANCIA_LECTURAS_UWB.md`](ANALISIS_DISCORDANCIA_LECTURAS_UWB.md) *(Discordancias)*
2. [`REPORTE_DISCORDANCIAS.md`](REPORTE_DISCORDANCIAS.md) *(Reporte completo)*

### 📝 **Sistema de Logging**
1. [`ANALISIS_LOGGING_ACTUAL.md`](ANALISIS_LOGGING_ACTUAL.md) *(Estado actual)*
2. [`SOLUCION_LOGGING_DIFERIDO.md`](SOLUCION_LOGGING_DIFERIDO.md) *(Solución propuesta)*
3. [`RESULTADOS_LOGGING_DIFERIDO_ANALISIS.md`](RESULTADOS_LOGGING_DIFERIDO_ANALISIS.md) *(Resultados)*

---

## 📋 **ESTADO ACTUAL DEL PROYECTO**

- ✅ **Análisis completado** - Todos los documentos técnicos revisados
- ✅ **Plan de corrección definido** - 3 soluciones principales identificadas
- 🔄 **Implementación en progreso** - Modificaciones a `uwb3000Fxx.h` pendientes
- ⏳ **Validación pendiente** - Tests físicos TEST-01, TEST-02, TEST-03

### 🎯 **Próximos Pasos**
1. Implementar correcciones en código fuente
2. Ejecutar tests de validación física
3. Documentar resultados
4. Liberar versión corregida

---

## 👥 **AUTORES Y RESPONSABLES**

- **Arturo Armando Veras Olivos** - Análisis técnico y coordinación
- **Manuel Ibáñez Mancilla** - Desarrollo LoRa y optimizaciones
- **Alan Gamalier Vergara Muñoz** - Integración inicial LoRa

---

## 📞 **CONTACTOS**

- **Proyecto**: fw-SmartLocate (FWSL)
- **Repositorio**: [GitLab uqomm/firmware/sniffer-tag-firmware](https://gitlab.com/uqomm/firmware/sniffer-tag-firmware)
- **JIRA**: [Proyecto FWSL](https://uqomm-teams.atlassian.net/projects/FWSL)

---

*Para navegación rápida, usa `Ctrl+F` para buscar por palabras clave como "timeout", "logging", "test", etc.*</content>
<parameter name="filePath">c:\Users\artur\development\sniffer-tag\documentos\README.md