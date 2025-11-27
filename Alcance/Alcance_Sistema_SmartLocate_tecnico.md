# Alcance del Sistema SmartLocate - Documento Técnico

**Fecha:** 27 de Noviembre de 2025  
**Versión:** 1.0  
**Clasificación:** Técnico - Para Ingeniería y Operaciones

---

## 1. Resumen Ejecutivo

El sistema SmartLocate es una solución de localización en tiempo real basada en tecnología **Ultra-Wideband (UWB)** para rastreo de personal en ambientes confinados, específicamente diseñado para túneles mineros subterráneos. El sistema utiliza el chip **DW3000** de Qorvo para medición de distancias mediante **Two-Way Ranging (TWR)** con triangulación de doble antena.

### Estado Actual
- **Tecnología validada:** UWB @ 850 Kbps (migrado desde 6.8 Mbps en Nov 2025)
- **Rango operativo validado:** 38 metros en condiciones LOS óptimas
- **Rango robusto garantizado:** 12 metros con orientación variable
- **Sistema en producción:** Firmware estable, listo para despliegue piloto

---

## 2. Arquitectura del Sistema

### 2.1. Componentes Hardware

#### **Sniffer (Unidad Fija de Detección)**
- **MCU:** STM32G474RET (Cortex-M4 @ 170 MHz)
- **Radio UWB:** DW3000 (Qorvo) - Dual channel para 2 antenas
- **Backhaul:** LoRa SX1278 (comunicación con servidor)
- **Antenas:** 2 × UWB omnidireccionales (separación: 2.4m recomendado)
- **Alimentación:** 12V DC (crítico para desempeño óptimo)
- **Interfaces:** SPI (DW3000), UART (LoRa), GPIO (control de antenas)

#### **Tag/Persona (Dispositivo Portátil)**
- **MCU:** STM32U535VET (Cortex-M33 @ 160 MHz, ultra-bajo consumo)
- **Radio UWB:** DW3000 (single channel)
- **Gestión de batería:** BQ25150 (carga Li-Ion)
- **Batería:** 1200 mAh Li-Ion (recargable)
- **Indicador:** WS2812 RGB LED (estado y batería)
- **Carga:** USB-C
- **Consumo:**
  - Activo (RX/TX): ~100 mA
  - Sleep profundo: ~1 µA (STM32U5 STOP mode)

#### **Servidor (VSDR-TG)** - Especificaciones de Brochure
- **Plataforma:** Mini-PC Linux embebido
- **Radio:** Módem spread spectrum (conexión a headend VHF)
- **Interfaces:** Ethernet RJ45, HDMI, USB 3.0
- **Montaje:** Rack 19", 2U
- **Software:** Servidor web embebido (visualización 2D)



### 2.3. Protocolo UWB - Two-Way Ranging

El sistema implementa **TWR (Two-Way Ranging)** con alternancia de antenas para triangulación 2D:

#### **Fase 1: Discovery**
```
Sniffer (Ant A/B) ──[TAG_ID_QUERY]──► Tag
                  ◄──[ID+TS+BAT]──────
                  
Tiempo: ~1 ms
Offset timestamps: 5/9 (discovery frame)
```

#### **Fase 2: Ranging** (3 lecturas por antena)
```
Sniffer (Ant A) ──[TAG_TIMESTAMP_QUERY]──► Tag
                ◄──[RX_TS, TX_TS]──────────

Sniffer (Ant B) ──[TAG_TIMESTAMP_QUERY]──► Tag
                ◄──[RX_TS, TX_TS]──────────

Tiempo: 6-18 ms (3 queries × 2 antenas)
Offset timestamps: 1/5 (query frame)
Cálculo distancia: d = c × (ToF) / 2
```

#### **Fase 3: Sleep**
```
Sniffer ──[TAG_SET_SLEEP_MODE]──► Tag
        ◄──[ACK]───────────────────

Tag → STOP Mode (15s con lecturas, 500ms sin lecturas)
```

### 2.4. Configuraciones UWB Soportadas

| Parámetro | Configuración 6.8M (Oct 2025) | Configuración 850K (Nov 2025) |
|-----------|-------------------------------|-------------------------------|
| **Data Rate** | DWT_BR_6M8 | DWT_BR_850K |
| **Preamble Length** | DWT_PLEN_128 (128 símbolos) | DWT_PLEN_1024 (1024 símbolos) |
| **PAC Size** | DWT_PAC8 (8 símbolos) | DWT_PAC32 (32 símbolos) |
| **PRE_TIMEOUT** | 12 PACs (~24 µs) | 32 PACs (~192 µs) |
| **POLL_TX_TO_RESP_RX_DLY** | 700 µs | 5600 µs (×8) |
| **RESP_RX_TIMEOUT** | 300 µs | 2400 µs (×8) |
| **POLL_RX_TO_RESP_TX_DLY** | 900 µs | 7200 µs (×8) |
| **SFD Timeout** | 129 + 8 - 12 = 125 | 1025 + 1 + 8 - 32 = 1002 |
| **Rango LOS validado** | 20m | **38m** |
| **Sensibilidad** | Baseline | **+6 dB** |


**Nota:** Todos los timeouts escalados por factor ×8 debido a frames 8× más largos (850K vs 6.8M).

---

## 3. Modos de Operación

### 3.1. MULTIPLE DETECTION (Modo Principal)

**Características:**
- Escaneo secuencial de múltiples tags (no paralelo)
- 3 lecturas por antena (6 totales por tag)
- Alternancia automática entre antena A y B
- Triangulación completa para posicionamiento 2D

**Flujo de Estados:**
```
TAG_DISCOVERY → TAG_SEND_TIMESTAMP_QUERY → TAG_END_READINGS → (repeat)
     ↓                    ↓                         ↓
  Busca tags      Alterna Ant A/B         Envía sleep command
  Timeout: N/A    Timeout: 1000ms         Guarda en mapa
                  Counter: 3×A + 3×B
```



**Casos de Error Manejados:**
- `RX_PREAMBLE_TIMEOUT`: Señal débil, incrementa error_crc, reintenta
- `RX_CRC_ERROR`: Frame corrupto, incrementa error_crc, reintenta
- `RX_FRAME_TIMEOUT`: Frame incompleto, incrementa error_crc, reintenta
- `query_timeout` (1000 ms): Guarda datos parciales si al menos 1 antena tiene 3 lecturas

### 3.2. ONE DETECTION (Modo Simplificado)

**Características:**
- Detección de un solo tag
- Sin queries adicionales de ranging
- Sin triangulación (solo distancia de antena activa)
- Latencia mínima

**Flujo de Estados:**
```
TAG_DISCOVERY → TAG_ONE_DETECTION → TAG_END_READINGS
     ↓                 ↓                    ↓
  Detecta tag    Guarda distancia    Envía sleep (15s)
                 de 1 antena solo
```

**Rendimiento (850 Kbps):**
- Latencia: ~50 ms
- Throughput: ~20 tags/seg
- **Limitación:** No hay posicionamiento 2D (sin triangulación)

**Uso recomendado:** Testing, debugging, escenarios con tag único conocido.

---

## 4. Capacidades Validadas

### 4.1. Pruebas de Campo (Octubre 2025 - 6.8 Mbps)

**Período:** 22-29 de Octubre de 2025  
**Tests ejecutados:** TEST-00 a TEST-07  
**Configuración:** PRE_TIMEOUT=8, Data Rate 6.8M

| Métrica | Resultado | Condiciones |
|---------|-----------|-------------|
| **Rango LOS (estático)** | 20m | Orientación óptima, 12V |
| **Rango LOS (movimiento)** | 19m | Operador caminando, obstrucción corporal |
| **Rango robusto** | 12m | Orientación variable aceptada |
| **Tasa detección @ 20m** | 70-80% | LOS, orientación favorable |
| **Tasa detección @ >20m** | <30% | Limitado por orientación/polarización |
| **Tags simultáneos probados** | 4 | TEST-05, TEST-06 |


**Conclusiones clave:**
-  No hay defectos hardware (Canal A OK tras TEST-07)
-  Sistema estable en movimiento y NLOS hasta 19m
-  Limitación principal: orientación de antena del tag >12m
-  PRE_TIMEOUT=8 óptimo (PRE_TIMEOUT=12 no mejora)

### 4.2. Migración a 850 Kbps (Noviembre 2025)

**Fecha implementación:** 26 de Noviembre de 2025  
**Esfuerzo:** 1 semana (2-3 días desarrollo + 1-2 días testing)

| Métrica | 6.8 Mbps | 850 Kbps | Mejora |
|---------|----------|----------|--------|
| **Rango LOS (óptimo)** | 20m | **38m** | **+90%**  |
| **Rango robusto** | 12m | 12m | 0% (baseline) |
| **Sensibilidad** | Baseline | **+6 dB** | Teórico confirmado |
| **Detección @ 0-12m** | 100% | 100% | Consistente |
| **Detección @ 12-38m** | <30% | Variable | Depende orientación |
| **Latencia (Multiple)** | 30-100 ms | 160-200 ms | +60-100% |
| **Throughput (Multiple)** | 10-33 tags/s | 5-6.25 tags/s | -50% |
| **CRC errors @ 38m** | N/A | 0% | Señal limpia  |

**Beneficios logrados:**
-  Extensión de rango: 20m → 38m (+90%)
-  +6 dB sensibilidad (frames largos, preámbulo 1024)
-  Mejor tolerancia NLOS y orientación >12m
-  Robustez mejorada con PAC32 (mejor detección en ruido)

**Trade-offs aceptados:**
-  Throughput reducido, habría que validar si afecta a nuestro sistema.

**Condiciones críticas validadas:**
-  LOS requerido para rango >12m
-  Orientación favorable de antena tag necesaria >12m
-  0% CRC errors en 38m (señal limpia, sin interferencias)

---

## 5. Especificaciones Técnicas del Sistema

### 5.1. Rangos Operativos

| Escenario | Rango Garantizado | Rango Óptimo | Condiciones |
|-----------|-------------------|--------------|-------------|
| **Personal a pie (LOS)** | 12m | 38m | 12V, orientación favorable |
| **Personal a pie (NLOS ligero)** | 10m | 20m | Obstrucción corporal |
| **Personal a pie (orientación variable)** | 12m | 12m | Sin optimización orientación |
| **Vehículos (30 km/h)** | **No validado** | **Proyectado: 12-30m** | **Pendiente TEST** |

### 5.2. Capacidad y Throughput

| Parámetro | Especificación | Validado | Notas |
|-----------|----------------|----------|-------|
| **Tags por Sniffer** | 50 (límite software) | <10 probados | Escalabilidad no validada |
| **Tags simultáneos activos** | 50 | 4 | TEST-05, TEST-06 (Oct 2025) |
| **Update rate por tag** | 1 detección / 15s |  Validado | Sleep time con lecturas exitosas |
| **Latencia detección** | <1 segundo |  Validado | 160-200 ms @ 850K |
| **Throughput teórico** | 5-6 tags/seg |  Validado | Multiple Detection mode |
| **Tiempo ciclo 50 tags** | ~10 segundos |  Proyectado | No probado con alta densidad |

### 5.3. Batería y Autonomía

| Parámetro | Especificación | Validado | Notas |
|-----------|----------------|----------|-------|
| **Capacidad batería** | 1200 mAh Li-Ion |  Hardware | BQ25150 |
| **Autonomía @ 6.8M** | ~7 días |  No probado | Estimado brochure |
| **Autonomía @ 850K** | **Pendiente validar** |  No probado | Frames más largos → mayor consumo TX |
| **Consumo activo** | ~100 mA |  Datasheet | DW3000 TX/RX |
| **Consumo sleep** | ~1 µA |  Datasheet | STM32U5 STOP mode |
| **Duty cycle típico** | 0.2-1% |  Estimado | 200ms activo / 15s sleep |
| **Carga completa** | USB-C, ~2 horas |  Hardware | BQ25150 charge controller |

**Consumo estimado @ 850K:**

Según pruebas realizadas con un Tag, se obtuvo lo siguiente:

- En caso más desfavorable 6.5 Días de batería
- En caso de múltiple detection efectivo constante, 14 Dias de batería.


## 6. Análisis de Gaps - Validado vs Pendiente

### 6.1. VALIDADO 

| Capacidad | Evidencia | Fecha Validación |
|-----------|-----------|------------------|
| Rango 38m LOS | Pruebas de campo @ 12V, orientación óptima | Nov 2025 |
| Rango 12m robusto | TEST-00 a TEST-07, orientación variable | Oct 2025 |
| Triangulación 2D | Dual-antenna (A/B), 3 lecturas c/u | Oct-Nov 2025 |
| Detección en movimiento | TEST-07, operador caminando | Oct 2025 |
| NLOS ligero (corporal) | TEST-07, obstrucción corporal hasta 19m | Oct 2025 |
| Monitoreo batería | Transmisión voltage en discovery/queries | Oct-Nov 2025 |
| Multiple detection (4 tags) | TEST-05, TEST-06 | Oct 2025 |
| Migración 850K exitosa | Implementación completa, rango +90% | Nov 2025 |
| Firmware estable | 0% CRC errors @ 38m, sistema robusto | Nov 2025 |

### 6.2. PENDIENTE DE VALIDACIÓN 

| Capacidad | Riesgo | Prioridad | Tiempo Estimado |
|-----------|--------|-----------|-----------------|
| **Vehículos @ 30 km/h con 25 tags** | **ALTO** | **CRÍTICA** | **1 día** |
| **Autonomía batería 7 días @ 850K** | MEDIO | ALTA | 2-3 días |
| **Alta densidad (25-50 tags simultáneos)** | MEDIO | ALTA | 1 día |
| **NLOS severo (múltiples paredes)** | BAJO | MEDIA | 1 día |

---

## 7. Despliegue Físico - Especificaciones de Instalación

### 7.1. Configuración del Sniffer

**Ubicación recomendada:**
- **Altura sobre piso:** 3 metros (validado en túneles Ø 3m)
- **Orientación antenas:** Vertical (omnidireccional en plano horizontal)
- **Separación antenas A-B:** 2.4 metros (1.2m a cada lado del sniffer)
- **Alimentación:** 12V DC estabilizado ( crítico para performance)
- **Backhaul:** LoRa a servidor (frecuencia y potencia por configurar)



### 7.2. Configuración del Tag

**Portación recomendada:**
- **Ubicación:** Sobre ropa exterior (chaleco, casco)
- **Evitar:** Bloqueo con objetos metálicos, mochilas pesadas
- **Indicador LED:** Verde (batería OK), Amarillo (batería media), Rojo (batería baja)

**Factores críticos de performance:**
-  **Orientación antenna tag:** Vertical óptimo (omnidireccional horizontal)
-  **Obstrucción corporal:** Tolerable hasta 19m (TEST-07 validado)
-  **Orientación desfavorable:** Limita rango a 12m
-  **Movimiento:** No afecta detección (TEST-07 @ 19m caminando)

---

## 8. Roadmap Técnico - Validaciones Pendientes

### 8.1. Fase 1: Validaciones Críticas (Semana 1-2)

**Prioridad ALTA - Requeridas antes de despliegue comercial**

#### TEST-08: Vehículos en Movimiento @ 30 km/h
**Objetivo:** Validar detección de vehículo con 25 tags pasando bajo sniffer a 30 km/h

**Setup:**
- Vehículo con 25 tags distribuidos (simulando pasajeros)
- Velocidad: 30 km/h (8.3 m/s)
- Sniffer @ 3m altura
- Túnel LOS

**Métricas de éxito:**
- ≥70% de tags detectados (≥18 de 25)
- Latencia <1s por tag detectado


**Duración:** 1 día  
**Riesgo si falla:** ALTO - Caso de uso principal del brochure



#### TEST-09: Autonomía de Batería @ 850K
**Objetivo:** Validar duración de batería en condiciones reales de operación

**Setup:**
- 3-5 tags en operación continua
- Ciclo: detección cada 15s (sleep_time_recived)
- Monitoreo voltage cada hora
- Hasta descarga completa (cutoff voltage)

**Métricas de éxito:**
- ≥6 días de operación continua


**Duración:** 7 días (prueba continua)  
**Riesgo si falla:** MEDIO - Expectativa del cliente basada en brochure (7 días)



#### TEST-10: Alta Densidad (25-50 Tags Simultáneos)
**Objetivo:** Validar performance con máxima densidad de tags especificada

**Setup:**
- 50 tags distribuidos en área 20m × 3m
- Sniffer central @ 3m altura
- Monitoreo de colisiones, timeouts, latencias

**Métricas de éxito:**
- 100% de tags detectados en cada ciclo (50/50)
- Tiempo ciclo completo <15 segundos
- 0 colisiones de frames (protocolo secuencial)

**Duración:** 1 día  
**Riesgo si falla:** MEDIO - Especificación brochure (50 tags/sniffer)


        

### 8.4. Cronograma Consolidado

```
Semana 1-2: Validaciones Críticas
├─ TEST-08: Vehículos @ 30 km/h (1 día)
├─ TEST-09: Batería @ 850K (inicio, 7 días paralelo)
└─ TEST-10: Alta densidad 50 tags (1 día)

Semana 3: Validaciones de Robustez
├─ TEST-11: NLOS severo (1 día)

```

---

## 9. Optimizaciones Futuras (Post-Validación)

### 9.1. Firmware

**Optimización 1: Ranging Adaptativo**
- Ajustar número de lecturas basado en SNR (3 lecturas si señal fuerte, 5 si débil)
- Reducir latencia promedio en condiciones óptimas
- **Beneficio:** -20% latencia en rango <20m
- **Esfuerzo:** 1 semana

**Optimización 2: Gestión Inteligente de Sleep**
- Sleep dinámico basado en actividad detectada (15s estático, 5s en movimiento)
- Wake-up por acelerómetro (opcional, requiere HW)
- **Beneficio:** +20% autonomía batería
- **Esfuerzo:** 2 semanas (+ HW acelerómetro si aplica)

**Optimización 3: Algoritmo de Triangulación Mejorado**
- Filtro Kalman para suavizar lecturas ruidosas
- Compensación de orientación basada en RSSI
- **Beneficio:** +15% precisión en NLOS
- **Esfuerzo:** 2-3 semanas

### 9.2. Hardware

**Mejora 1: Antena Optimizada**
- Diseño custom para patrón omnidireccional optimizado
- Ganancia +2 dBi (extender rango 20%)
- **Beneficio:** 38m → 45m (proyectado)
- **Esfuerzo:** 4-6 semanas + testing

**Mejora 2: Tag con Acelerómetro**
- Detección de movimiento para sleep adaptativo
- Detección de caídas (alarma de seguridad)
- **Beneficio:** +30% autonomía + feature de seguridad
- **Esfuerzo:** Rediseño PCB (8-12 semanas)

**Mejora 3: Sniffer con GPS**
- Sincronización temporal precisa entre sniffers
- Logging con timestamp absoluto
- **Beneficio:** Mejor precisión en handover entre sniffers
- **Esfuerzo:** 2 semanas FW + validación

---

## 10. Matriz de Riesgos Técnicos

| Riesgo | Probabilidad | Impacto | Mitigación | Owner |
|--------|--------------|---------|-----------|-------|
| **Vehículos @ 30 km/h fallan TEST-08** | Media (40%) | ALTO | Reducir velocidad especificación (20 km/h) o aumentar sniffers | Testing |
| **Batería <7 días @ 850K** | Baja (20%) | MEDIO | Especificar 5 días conservador, optimizar sleep | FW |
| **Alta densidad (50 tags) timeout** | Baja (15%) | MEDIO | Especificar 25 tags/sniffer, escalar con más sniffers | Testing |
| **NLOS severo degrada rango <8m** | Media (30%) | BAJO | Documentar limitación, recomendar LOS deployment | Producto |
| **EMI industrial causa 5-10% errores** | Media (35%) | MEDIO | Filtrado FW, shielding HW si necesario | FW+HW |
| **Temperatura extrema falla carga** | Baja (10%) | MEDIO | Especificar rango operación reducido (-10°C a +50°C) | Producto |
| **Certificación RF rechazada** | Baja (10%) | ALTO | Pre-compliance testing, diseño conservador | HW+Compliance |
| **IP rating insuficiente (polvo)** | Media (25%) | MEDIO | Rediseño enclosure, gaskets mejorados | Mecánica |

**Leyenda:**
- **Probabilidad:** Baja (<20%), Media (20-50%), Alta (>50%)
- **Impacto:** BAJO (ajuste documentación), MEDIO (rediseño menor), ALTO (bloqueante comercial)

---

## 11. Conclusiones Técnicas

### 11.1. Madurez del Sistema

| Componente | Madurez | Estado | Acción Requerida |
|------------|---------|--------|------------------|
| **Firmware Sniffer** | 85% |  Estable | Validaciones pendientes (TEST-08, TEST-10) |
| **Firmware Tag** | 85% |  Estable | Validación batería (TEST-09) |
| **Hardware Sniffer** | 90% |  Producible | Certificaciones IP, EMC |
| **Hardware Tag** | 90% |  Producible | Certificaciones IP, vibración |
| **Protocolo UWB** | 95% |  Validado | Optimizaciones opcionales |
| **Integración Sistema** | 70% |  Parcial | Servidor web, visualización 2D |
| **Documentación** | 60% |  Incompleta | Manuales instalación, troubleshooting |

**Evaluación global:** Sistema en **TRL 7** (System prototype demonstration in operational environment)

**Camino a TRL 8-9 (comercialización):**
1. Completar TEST-08, TEST-09, TEST-10 (Fase 1 roadmap) → TRL 8
2. Validar en ambiente industrial real (Fase 2 roadmap) → TRL 8
3. Obtener certificaciones (Fase 3 roadmap) → TRL 9
4. Despliegue piloto con cliente (3-6 meses) → TRL 9

### 11.2. Fortalezas Técnicas

 **Tecnología UWB validada:** DW3000 chip maduro, protocolo TWR robusto  
 **Rango extendido demostrado:** 38m LOS (+90% vs baseline 6.8M)  
 **Triangulación 2D funcional:** Dual-antenna alternancia automática  
 **Firmware estable:** 0% CRC errors @ 38m, manejo de errores robusto  
 **Migración exitosa:** 6.8M → 850K en 1 semana (bajo riesgo)  
 **Bajo consumo:** STM32U5 + sleep profundo (~1 µA)  
 **Escalabilidad teórica:** 50 tags/sniffer (software ready, pending test)  

### 11.3. Debilidades Técnicas

 **Validaciones incompletas:** Vehículos, batería, alta densidad sin probar  
 **Documentación desactualizada:** Brochures describen VHF vs. UWB real  
 **Integración servidor:** Visualización 2D no documentada técnicamente  
 **Certificaciones ausentes:** IP, EMC, RF compliance pendientes  
 **Orientación crítica:** Performance degrada >12m con orientación desfavorable  
 **Dependencia 12V:** Alimentación inadecuada afecta rango  

### 11.4. Recomendaciones para Ingeniería

**Prioridad CRÍTICA (Pre-despliegue):**
1. Ejecutar TEST-08 (vehículos @ 30 km/h) - **1 día, BLOQUEANTE**
2. Ejecutar TEST-09 (batería 7 días) - **7 días, BLOQUEANTE**
3. Ejecutar TEST-10 (50 tags simultáneos) - **1 día, BLOQUEANTE**
4. Actualizar brochures con especificaciones reales (UWB, 38m, TWR)

**Prioridad ALTA (Pre-producción):**
5. Validar EMI industrial (TEST-12) - **1 semana**
6. Desarrollar manual de instalación técnico detallado
7. Crear guía de troubleshooting (errores comunes, diagnóstico)
8. Implementar logging robusto para debugging en campo

**Prioridad MEDIA (Optimización):**
9. Optimizar algoritmo triangulación (Kalman filter)
10. Implementar ranging adaptativo (ajustar queries por SNR)
11. Diseño antena custom (+2 dBi ganancia)
12. Pre-compliance testing RF (antes de certificación formal)

---

## 12. Referencias Técnicas

### 12.1. Documentación Interna

- `Resumen_Ejecutivo.md` - Resultados testing Oct 2025 (6.8 Mbps)
- `Resumen_Ejecutivo_Migracion_850K.md` - Resultados migración Nov 2025
- `Migracion_Base.md` - Plan técnico migración 850K
- `ESQUEMA_COMUNICACION_UWB.md` - Protocolo UWB detallado con timeouts
- `DIAGRAMAS_ESTADO_MERMAID.md` - Máquinas de estado Sniffer/Tag
- `Brochure_servidor_tag.md` - Especificaciones comerciales servidor
- `Brochure_Sniffer_tag.md` - Especificaciones comerciales VLS-TG

### 12.2. Datasheets y Manuales

- **DW3000 User Manual** (Qorvo) - UWB transceiver, TWR protocol, timeouts
- **STM32G474RET Datasheet** (STMicroelectronics) - Sniffer MCU
- **STM32U535VET Datasheet** (STMicroelectronics) - Tag MCU ultra-low-power
- **BQ25150 Datasheet** (Texas Instruments) - Battery charger IC
- **SX1278 Datasheet** (Semtech) - LoRa transceiver





---

## Apéndice A: Glosario Técnico

| Término | Definición |
|---------|------------|
| **UWB** | Ultra-Wideband - Tecnología de radio de corto alcance, ancho de banda >500 MHz |
| **TWR** | Two-Way Ranging - Método de medición de distancia por tiempo de vuelo bidireccional |
| **TDOA** | Time Difference of Arrival - Método alternativo de posicionamiento (NO usado en SmartLocate) |
| **LOS** | Line of Sight - Línea de vista directa sin obstrucciones |
| **NLOS** | Non Line of Sight - Sin línea de vista, con obstrucciones |
| **PAC** | Preamble Acquisition Chunk - Unidad de símbolos para detección de preámbulo |
| **PRE_TIMEOUT** | Timeout de detección de preámbulo (en múltiplos de PAC) |
| **ToF** | Time of Flight - Tiempo de vuelo de señal RF |
| **SNR** | Signal-to-Noise Ratio - Relación señal/ruido |
| **RSSI** | Received Signal Strength Indicator - Indicador de potencia de señal recibida |
| **CRC** | Cyclic Redundancy Check - Verificación de integridad de datos |
| **DWT** | DecaWave Time - Unidad de tiempo interna del DW3000 (15.65 ps) |
| **RXFTO** | RX Frame Timeout - Timeout de recepción de frame completo |
| **RXPTO** | RX Preamble Timeout - Timeout de detección de preámbulo |
| **TRL** | Technology Readiness Level - Nivel de madurez tecnológica (1-9) |

## Apéndice B: Comandos UWB Frame Structures

### Discovery Frame (TAG_ID_QUERY)
```c
TX: [0x10, state, sleep_mode, dev_mode]  // 4 bytes
RX: [0x10, tag_id, poll_rx_ts(5), resp_tx_ts(9), battery]  // 15 bytes
```

### Timestamp Query Frame (TAG_TIMESTAMP_QUERY)
```c
TX: [0x11, tag_id, distance_a, distance_b]  // 9 bytes
RX: [0x11, poll_rx_ts(1), resp_tx_ts(5), battery]  // 15 bytes
```

### Sleep Command Frame (TAG_SET_SLEEP_MODE)
```c
TX: [0x12, tag_id, sleep_time_rx, sleep_time_no_rx, ship_mode]  // 8 bytes
RX: [0x12, poll_rx_ts(1), resp_tx_ts(5), battery]  // 15 bytes
```

**Nota:** Números entre paréntesis indican offset de timestamp dentro del frame.