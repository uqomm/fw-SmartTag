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

### 2.2. Arquitectura de Comunicación

```
┌─────────────────────────────────────────────────────────────┐
│                    TÚNEL MINERO (Ø ~3m)                      │
│                                                               │
│  ┌──────────┐                                                │
│  │ Sniffer  │ ← Altura: 3m sobre piso                        │
│  │  ┌───┐   │                                                │
│  │  │ANT│   │ ← Antena A                                     │
│  │  │ A │   │                                                │
│  └──┴───┴───┘                                                │
│       ↕ 2.4m                                                 │
│  ┌─────────┐                                                 │
│  │  ┌───┐  │ ← Antena B                                      │
│  │  │ANT│  │                                                 │
│  │  │ B │  │                                                 │
│  └──┴───┴──┘                                                 │
│       ║                                                       │
│       ║ UWB Ranging (6.5 GHz, Channel 9)                     │
│       ║                                                       │
│   ●───╫───● ← Tags en personas (caminando)                   │
│       ║                                                       │
│   🚗──╫──● ← Tags en vehículos (hasta 30 km/h - pendiente)   │
│       ║                                                       │
│       ▼                                                       │
│   [LoRa TX] → Servidor VSDR-TG                               │
│                                                               │
└─────────────────────────────────────────────────────────────┘
```

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
| **Latencia por tag** | 30-100 ms | 160-200 ms |

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

**Rendimiento (850 Kbps):**
- Latencia por tag: 160-200 ms
- Throughput: 5-6.25 tags/seg
- Capacidad: Hasta 50 tags (limitado por software, no probado)

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
| **Latencia por tag (Multiple)** | 30-100 ms | Variable por errores CRC |
| **Latencia por tag (One)** | ~15 ms | Sin queries adicionales |

**Conclusiones clave:**
- ✅ No hay defectos hardware (Canal A OK tras TEST-07)
- ✅ Sistema estable en movimiento y NLOS hasta 19m
- ⚠️ Limitación principal: orientación de antena del tag >12m
- ✅ PRE_TIMEOUT=8 óptimo (PRE_TIMEOUT=12 no mejora)

### 4.2. Migración a 850 Kbps (Noviembre 2025)

**Fecha implementación:** 26 de Noviembre de 2025  
**Esfuerzo:** 1 semana (2-3 días desarrollo + 1-2 días testing)

| Métrica | 6.8 Mbps | 850 Kbps | Mejora |
|---------|----------|----------|--------|
| **Rango LOS (óptimo)** | 20m | **38m** | **+90%** ✅ |
| **Rango robusto** | 12m | 12m | 0% (baseline) |
| **Sensibilidad** | Baseline | **+6 dB** | Teórico confirmado |
| **Detección @ 0-12m** | 100% | 100% | Consistente |
| **Detección @ 12-38m** | <30% | Variable | Depende orientación |
| **Latencia (Multiple)** | 30-100 ms | 160-200 ms | +60-100% |
| **Throughput (Multiple)** | 10-33 tags/s | 5-6.25 tags/s | -50% |
| **CRC errors @ 38m** | N/A | 0% | Señal limpia ✅ |

**Beneficios logrados:**
- ✅ Extensión de rango: 20m → 38m (+90%)
- ✅ +6 dB sensibilidad (frames largos, preámbulo 1024)
- ✅ Mejor tolerancia NLOS y orientación >12m
- ✅ Robustez mejorada con PAC32 (mejor detección en ruido)

**Trade-offs aceptados:**
- ⚠️ Latencia aumenta ~100 ms (aceptable para tracking)
- ⚠️ Throughput reducido 50% (5-6 tags/seg suficiente para 50 tags con sleep 15s)

**Condiciones críticas validadas:**
- ✅ Alimentación 12V esencial (performance degradado con <12V)
- ✅ LOS requerido para rango >12m
- ✅ Orientación favorable de antena tag necesaria >12m
- ✅ 0% CRC errors en 38m (señal limpia, sin interferencias)

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
| **Update rate por tag** | 1 detección / 15s | ✅ Validado | Sleep time con lecturas exitosas |
| **Latencia detección** | <1 segundo | ✅ Validado | 160-200 ms @ 850K |
| **Throughput teórico** | 5-6 tags/seg | ✅ Validado | Multiple Detection mode |
| **Tiempo ciclo 50 tags** | ~10 segundos | ⚠️ Proyectado | No probado con alta densidad |

### 5.3. Batería y Autonomía

| Parámetro | Especificación | Validado | Notas |
|-----------|----------------|----------|-------|
| **Capacidad batería** | 1200 mAh Li-Ion | ✅ Hardware | BQ25150 |
| **Autonomía @ 6.8M** | ~7 días | ⚠️ No probado | Estimado brochure |
| **Autonomía @ 850K** | **Pendiente validar** | ❌ No probado | Frames más largos → mayor consumo TX |
| **Consumo activo** | ~100 mA | ✅ Datasheet | DW3000 TX/RX |
| **Consumo sleep** | ~1 µA | ✅ Datasheet | STM32U5 STOP mode |
| **Duty cycle típico** | 0.2-1% | ⚠️ Estimado | 200ms activo / 15s sleep |
| **Carga completa** | USB-C, ~2 horas | ✅ Hardware | BQ25150 charge controller |

**Consumo estimado @ 850K:**
- Activo (200 ms cada 15s): 100 mA × 0.2s = 20 mAs
- Sleep (14.8s): 1 µA × 14.8s ≈ 0.015 mAs
- **Total por ciclo:** ~20 mAs
- **Autonomía estimada:** 1200 mAh / (20 mAs × 4 ciclos/min × 60 min × 24 h) ≈ **5-6 días**

⚠️ **Validación pendiente:** Prueba de batería en campo durante 7 días (TEST crítico).

### 5.4. Temperatura y Ambiente

| Parámetro | Especificación | Fuente |
|-----------|----------------|--------|
| **Temperatura operación** | -20°C a +60°C | Brochure servidor |
| **Temperatura almacenamiento** | -40°C a +85°C | Datasheet STM32 |
| **Humedad relativa** | 0-95% (sin condensación) | Típico IP54 |
| **Protección ingreso** | No especificada | ⚠️ Pendiente certificación |
| **Resistencia vibración** | No especificada | ⚠️ Pendiente testing |
| **Resistencia impacto** | No especificada | ⚠️ Pendiente testing |

---

## 6. Análisis de Gaps - Validado vs Pendiente

### 6.1. VALIDADO ✅

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

### 6.2. PENDIENTE DE VALIDACIÓN ⚠️

| Capacidad | Riesgo | Prioridad | Tiempo Estimado |
|-----------|--------|-----------|-----------------|
| **Vehículos @ 30 km/h con 25 tags** | **ALTO** | **CRÍTICA** | **1 día** |
| **Autonomía batería 7 días @ 850K** | MEDIO | ALTA | 2-3 días |
| **Alta densidad (25-50 tags simultáneos)** | MEDIO | ALTA | 1 día |
| **NLOS severo (múltiples paredes)** | BAJO | MEDIA | 1 día |
| **Interferencia EMI industrial** | MEDIO | MEDIA | 1 semana |
| **Certificación IP (polvo/agua)** | BAJO | BAJA | Depende laboratorio |
| **Testing vibración/impacto** | BAJO | BAJA | 1 semana |
| **Temperatura extrema (-20°C, +60°C)** | BAJO | BAJA | 1 semana |

### 6.3. NO APLICABLE / DESCARTADO ❌

| Ítem Brochure | Realidad Técnica | Acción Requerida |
|---------------|------------------|------------------|
| Rango 80m caminando | Máx. 38m validado UWB | Actualizar brochure a 38m |
| Frecuencias VHF (143-187 MHz) | UWB ~6.5 GHz (Ch9) | Corregir brochure o aclarar híbrido |
| TDOA triangulation | TWR (Two-Way Ranging) | Actualizar descripción técnica |
| Leaky Feeder integration | UWB antenas omni (LoRa backhaul) | Aclarar arquitectura real |

---

## 7. Despliegue Físico - Especificaciones de Instalación

### 7.1. Configuración del Sniffer

**Ubicación recomendada:**
- **Altura sobre piso:** 3 metros (validado en túneles Ø 3m)
- **Orientación antenas:** Vertical (omnidireccional en plano horizontal)
- **Separación antenas A-B:** 2.4 metros (1.2m a cada lado del sniffer)
- **Alimentación:** 12V DC estabilizado (⚠️ crítico para performance)
- **Backhaul:** LoRa a servidor (frecuencia y potencia por configurar)

**Patrón de cobertura (plano horizontal):**
```
        Ant A (1.2m izq)    Sniffer    Ant B (1.2m der)
             │                 │              │
             ●─────────────────●──────────────●
                      2.4m total
                           
        ┌─────────────────────────────────────────┐
        │           ZONA DE COBERTURA             │
        │                                         │
        │    ●●●●●●●●●●●●●●●●●●●●●●●●●●●●●       │
        │    ●                             ●      │ ← 38m radio
        │    ●    12m robusto (garantizado)●      │   (LOS óptimo)
        │    ●         ┌────────┐          ●      │
        │    ●         │Sniffer │          ●      │
        │    ●         └────────┘          ●      │
        │    ●                             ●      │
        │    ●●●●●●●●●●●●●●●●●●●●●●●●●●●●●       │
        │                                         │
        └─────────────────────────────────────────┘
```

### 7.2. Configuración del Tag

**Portación recomendada:**
- **Ubicación:** Sobre ropa exterior (chaleco, casco)
- **Orientación:** Antena hacia arriba (vertical)
- **Evitar:** Bloqueo con objetos metálicos, mochilas pesadas
- **Indicador LED:** Verde (batería OK), Amarillo (batería media), Rojo (batería baja)

**Factores críticos de performance:**
- ✅ **Orientación antenna tag:** Vertical óptimo (omnidireccional horizontal)
- ⚠️ **Obstrucción corporal:** Tolerable hasta 19m (TEST-07 validado)
- ⚠️ **Orientación desfavorable:** Limita rango a 12m
- ✅ **Movimiento:** No afecta detección (TEST-07 @ 19m caminando)

### 7.3. Espaciado de Sniffers en Túnel

**Para cobertura continua:**

| Rango objetivo | Espaciado sniffers | Overlap | Notas |
|----------------|--------------------|---------|-------|
| **Robusto (12m)** | 20m | 4m (17%) | Cobertura garantizada |
| **Óptimo (38m)** | 70m | 6m (8%) | Requiere LOS, 12V, orientación |
| **Conservador (20m)** | 36m | 4m (10%) | Balance costo-confiabilidad |

**Recomendación:** Espaciado 36m (cobertura 20m por sniffer) para balance entre costo y confiabilidad.

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
- 0 falsos positivos

**Duración:** 1 día  
**Riesgo si falla:** ALTO - Caso de uso principal del brochure

**Análisis técnico:**
```
Tiempo disponible para detección:
- Ventana de paso: 2 × 38m / 8.3 m/s ≈ 9.2 segundos
- Tiempo por tag @ 850K: 160-200 ms
- Tags detectables teóricos: 9.2s / 0.2s ≈ 46 tags
- Conclusión: Factible técnicamente, requiere validación práctica
```

#### TEST-09: Autonomía de Batería @ 850K
**Objetivo:** Validar duración de batería en condiciones reales de operación

**Setup:**
- 3-5 tags en operación continua
- Ciclo: detección cada 15s (sleep_time_recived)
- Monitoreo voltage cada hora
- Hasta descarga completa (cutoff voltage)

**Métricas de éxito:**
- ≥5 días de operación continua
- Curva de descarga lineal y predecible
- Sin degradación de performance con batería baja (>10%)

**Duración:** 7 días (prueba continua)  
**Riesgo si falla:** MEDIO - Expectativa del cliente basada en brochure (7 días)

**Estimación teórica:**
- Consumo por ciclo: ~20 mAs (200ms @ 100mA activo + 14.8s @ 1µA sleep)
- Ciclos por día: 4 × 60 × 24 = 5760
- Consumo diario: 5760 × 20 mAs = 115.2 mAh/día
- **Autonomía proyectada:** 1200 mAh / 115.2 mAh/día ≈ **10.4 días**
- **Margen:** +48% sobre especificación brochure (7 días) ✅

#### TEST-10: Alta Densidad (25-50 Tags Simultáneos)
**Objetivo:** Validar performance con máxima densidad de tags especificada

**Setup:**
- 50 tags distribuidos en área 20m × 20m
- Sniffer central @ 3m altura
- Múltiples detecciones consecutivas (10 ciclos)
- Monitoreo de colisiones, timeouts, latencias

**Métricas de éxito:**
- 100% de tags detectados en cada ciclo (50/50)
- Latencia promedio <300 ms por tag
- Tiempo ciclo completo <15 segundos
- 0 colisiones de frames (protocolo secuencial)

**Duración:** 1 día  
**Riesgo si falla:** MEDIO - Especificación brochure (50 tags/sniffer)

**Análisis técnico:**
```
Tiempo ciclo 50 tags @ 850K:
- Tiempo por tag (promedio): 180 ms
- Total: 50 × 0.18s = 9 segundos
- Overhead (switches, sleep commands): ~1s
- Total estimado: ~10 segundos ✅

Nota: Sistema secuencial (no paralelo) → no hay colisiones RF
```

### 8.2. Fase 2: Validaciones de Robustez (Semana 3-4)

**Prioridad MEDIA - Recomendadas para ambientes industriales**

#### TEST-11: NLOS Severo (Múltiples Obstrucciones)
**Objetivo:** Caracterizar performance en NLOS con paredes, equipos, vehículos

**Setup:**
- Escenarios: 1 pared, 2 paredes, obstáculos metálicos
- Materiales: concreto, metal, madera
- Rangos: 5m, 10m, 15m

**Duración:** 1 día

#### TEST-12: Interferencia EMI/RFI Industrial
**Objetivo:** Validar inmunidad a interferencias electromagnéticas de equipos mineros

**Setup:**
- Co-ubicación con motores, inversores, soldadoras
- Medición con spectrum analyzer (baseline vs. interferido)
- Tasas de error CRC, RXFTO, RXPTO

**Duración:** 1 semana (requiere acceso a mina en operación)

#### TEST-13: Temperatura Extrema
**Objetivo:** Validar operación en -20°C y +60°C (especificación brochure)

**Setup:**
- Cámara climática o ambiente controlado
- Pruebas de rango, latencia, consumo
- Carga de batería en temperatura extrema

**Duración:** 1 semana (requiere cámara climática)

### 8.3. Fase 3: Certificaciones y Compliance (Semana 5-8)

**Prioridad BAJA - Requeridas para comercialización formal**

#### CERT-01: Certificación IP (Protección Ingreso)
**Objetivo:** IP54 mínimo (polvo, salpicaduras agua)

**Tests:**
- IP5X: Protección contra polvo
- IPX4: Protección contra salpicaduras
- Laboratorio certificado

**Duración:** 2-3 semanas (laboratorio externo)  
**Costo estimado:** $2,000-5,000 USD

#### CERT-02: Testing de Vibración e Impacto
**Objetivo:** MIL-STD-810G o equivalente minero

**Tests:**
- Vibración random (vehículos, equipos)
- Impacto (caídas desde 1.5m)
- Durabilidad conectores

**Duración:** 1-2 semanas (laboratorio externo)  
**Costo estimado:** $3,000-6,000 USD

#### CERT-03: Certificación RF (FCC, CE, regulaciones locales)
**Objetivo:** Cumplimiento regulatorio para comercialización

**Tests:**
- Emisiones RF (FCC Part 15, ETSI EN 300 328)
- SAR (Specific Absorption Rate) si portátil
- EMC (Electromagnetic Compatibility)

**Duración:** 4-6 semanas (laboratorio certificado)  
**Costo estimado:** $10,000-20,000 USD

### 8.4. Cronograma Consolidado

```
Semana 1-2: Validaciones Críticas
├─ TEST-08: Vehículos @ 30 km/h (1 día)
├─ TEST-09: Batería @ 850K (inicio, 7 días paralelo)
└─ TEST-10: Alta densidad 50 tags (1 día)

Semana 3-4: Validaciones de Robustez
├─ TEST-11: NLOS severo (1 día)
├─ TEST-12: EMI industrial (1 semana)
└─ TEST-13: Temperatura extrema (1 semana)

Semana 5-8: Certificaciones (paralelo con laboratorios)
├─ CERT-01: IP rating (2-3 semanas)
├─ CERT-02: Vibración/impacto (1-2 semanas)
└─ CERT-03: RF compliance (4-6 semanas)

═══════════════════════════════════════════════════
MILESTONE 1 (Fin Semana 2): Sistema Validado para Piloto
MILESTONE 2 (Fin Semana 4): Sistema Validado para Producción
MILESTONE 3 (Fin Semana 8): Sistema Certificado para Comercialización
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
| **Firmware Sniffer** | 85% | ✅ Estable | Validaciones pendientes (TEST-08, TEST-10) |
| **Firmware Tag** | 85% | ✅ Estable | Validación batería (TEST-09) |
| **Hardware Sniffer** | 90% | ✅ Producible | Certificaciones IP, EMC |
| **Hardware Tag** | 90% | ✅ Producible | Certificaciones IP, vibración |
| **Protocolo UWB** | 95% | ✅ Validado | Optimizaciones opcionales |
| **Integración Sistema** | 70% | ⚠️ Parcial | Servidor web, visualización 2D |
| **Documentación** | 60% | ⚠️ Incompleta | Manuales instalación, troubleshooting |

**Evaluación global:** Sistema en **TRL 7** (System prototype demonstration in operational environment)

**Camino a TRL 8-9 (comercialización):**
1. Completar TEST-08, TEST-09, TEST-10 (Fase 1 roadmap) → TRL 8
2. Validar en ambiente industrial real (Fase 2 roadmap) → TRL 8
3. Obtener certificaciones (Fase 3 roadmap) → TRL 9
4. Despliegue piloto con cliente (3-6 meses) → TRL 9

### 11.2. Fortalezas Técnicas

✅ **Tecnología UWB validada:** DW3000 chip maduro, protocolo TWR robusto  
✅ **Rango extendido demostrado:** 38m LOS (+90% vs baseline 6.8M)  
✅ **Triangulación 2D funcional:** Dual-antenna alternancia automática  
✅ **Firmware estable:** 0% CRC errors @ 38m, manejo de errores robusto  
✅ **Migración exitosa:** 6.8M → 850K en 1 semana (bajo riesgo)  
✅ **Bajo consumo:** STM32U5 + sleep profundo (~1 µA)  
✅ **Escalabilidad teórica:** 50 tags/sniffer (software ready, pending test)  

### 11.3. Debilidades Técnicas

⚠️ **Validaciones incompletas:** Vehículos, batería, alta densidad sin probar  
⚠️ **Documentación desactualizada:** Brochures describen VHF vs. UWB real  
⚠️ **Integración servidor:** Visualización 2D no documentada técnicamente  
⚠️ **Certificaciones ausentes:** IP, EMC, RF compliance pendientes  
⚠️ **Orientación crítica:** Performance degrada >12m con orientación desfavorable  
⚠️ **Dependencia 12V:** Alimentación inadecuada afecta rango  

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

**Prioridad BAJA (Long-term):**
13. Certificaciones formales (IP, EMC, RF) - **8-12 semanas total**
14. Rediseño mecánico para IP65 (vs. IP54 actual)
15. Integración acelerómetro en tag (sleep adaptativo)
16. Desarrollo servidor web custom (vs. solución actual no documentada)

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

### 12.3. Estándares y Normativas Aplicables

- **IEEE 802.15.4a/4z** - UWB PHY/MAC layer standard
- **FCC Part 15.503** - UWB emission limits (US)
- **ETSI EN 302 065** - UWB regulations (Europe)
- **MIL-STD-810G** - Environmental testing (vibration, shock, temperature)
- **IEC 60529** - IP rating classification
- **IEC 61000-6-2** - EMC immunity (industrial environments)

---

**Documento preparado por:** Equipo de Ingeniería SmartLocate  
**Fecha:** 27 de Noviembre de 2025  
**Versión:** 1.0  
**Próxima revisión:** Post TEST-08, TEST-09, TEST-10 (estimado: Diciembre 2025)

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