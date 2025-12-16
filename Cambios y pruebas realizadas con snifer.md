			# Bitácora de Pruebas - SmartLocate UWB System

Registro cronológico de cambios, pruebas y análisis del sistema de localización SmartTag.

---

## Octubre 2025

### 22 de Octubre, 2025

#### Estado Base del Sistema
- **Detección:** Estable hasta ±20 metros de distancia
- **Problema identificado:** No había "permanencia" de tags detectados en software
- **Solución aplicada:** Con Arturo se analizaron los tiempos de detección de tags y envío al servidor cuando no hay ningún tag. Se modificó este tiempo, resolviendo el problema.

#### Análisis del Sistema
- Se realizó un análisis completo del funcionamiento del sistema
- **Resultados:**
  - Varios documentos de análisis de comunicación entre sniffer y tag
  - Documento detallado del flujo de máquina de estados de cada equipo

---

### 23 de Octubre, 2025

#### Implementación de Logging
- **Conclusión del análisis:** Implementar logging mediante UART del estado de fallas de detección
- **Proceso:**
  1. Implementación del sistema de logging
  2. Pruebas y análisis de comunicación tag-sniffer
  3. **Conclusión:** Probar cambios en timeouts de respuesta de transmisores

#### Observación Importante
- Se tienen mediciones de **antena B**, pero no de **antena A** cuando está sobre 20 metros (intermitencia)

#### Primera Modificación de Timeouts
- **Cambio aplicado:** PRE_TIMEOUT_6M8 de 5 → 8 (en ambos equipos: sniffer y tag)
- **Resultados:** Similares a anteriores, con algunas mediciones de antena A
- **Nota:** Pruebas realizadas con camión presente (posible interferencia)

---

### 24 de Octubre, 2025

#### Intento de Mejora de Detección
- **Cambio planificado:** Aumentar PRE_TIMEOUT_6M8 a 12
- **Estado:** No se lograron realizar pruebas por pruebas simultáneas en el head end

---

### 27 de Octubre, 2025

#### Cambio de Hardware
- **Modificación:** Cambio de antenas del AP a antenas anteriores (orden de Daniel)

#### Pruebas con PRE_TIMEOUT_6M8 = 12
- **Configuración de prueba:**
  - Ubicación: Bajada del camión (22.3 mts medición antena B)
  - Tag instalado: Sobre taburete con batería externa
  - Orientación: Tag "mirando" (luz apuntando) hacia el sniffer

- **Resultados:**
  - En una posición: Detecta con 1 antena
  - Girado en otra posición: No detecta ninguna antena
  - **Conclusión:** Orientación del tag afecta significativamente la detección

---

### 28 de Octubre, 2025

#### Análisis Continuo de Timeouts
- Continuación del análisis de timeouts y problemas de comunicación sniffer-tag

#### Modificación en Firmware del Tag Persona
- **Cambio:** Eliminación de `hal_delay(1)` en comunicación MULTIPLE_DETECTION (caso de falla)
- **Resultados:** Similares a pruebas anteriores
  - Posición: Mismo taburete, 21.7 metros apuntando al sniffer
  - Detección: **Solo antena B**

#### Descarte de Problema de Hardware
- **Prueba:** Cruzamiento físico de antenas (swap A ↔ B)
- **Resultado:** Mismo comportamiento
  - Solo detecta antena B
  - Antena A sigue con error RX_PREAMBLE_DETECTION
- **Conclusión:** El problema NO es de hardware

#### Análisis de Calibración de Transmisores
- **Hallazgo:** Cada transmisor utiliza método de calibración propio
- **Problema identificado:** Se estaba usando la misma estructura para ambos transmisores en el sniffer
- **Hipótesis:** Si la calibración afecta la detección, esto es incorrecto

- **Cambio aplicado:** Cada transmisor utiliza su propia estructura para calibración
- **Resultado:** Sin mejora, resultados iguales
- **Verificación:** Log en inicialización confirmó que cada transmisor usa su estructura asignada correctamente

---

### 29 de Octubre, 2025

#### Estado
- Análisis en progreso...

---

## Resumen de Hallazgos Clave

### Problemas Identificados
1. ✅ Falta de permanencia de tags en software → **Resuelto**
2. ⚠️ Detección intermitente de antena A a >20m → **En investigación**
3. ⚠️ Orientación del tag afecta detección → **Documentado**

### Modificaciones Implementadas
- Timeouts de detección y envío al servidor
- Sistema de logging UART para diagnóstico
- PRE_TIMEOUT_6M8: 5 → 8 → 12
- Estructuras de calibración separadas por transmisor
- Eliminación de hal_delay(1) en MULTIPLE_DETECTION

### Configuraciones de Prueba
- **Rango de prueba:** 20-22.3 metros
- **Hardware:** Antenas AP → Antenas anteriores
- **Timeouts probados:** PRE_TIMEOUT_6M8 = 5, 8, 12

### Próximos Pasos
- [ ] Continuar análisis de problema de antena A
- [ ] Investigar impacto de orientación del tag
- [ ] Probar configuraciones adicionales de timeouts
- [ ] Validar soluciones con múltiples tags simultáneos

---

*Última actualización: 29 de Octubre, 2025* 