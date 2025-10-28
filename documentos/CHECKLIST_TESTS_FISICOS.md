# 🧪 Checklist Tests Físicos - Corrección Detección >20m

**Fecha**: ___________ | **Responsable**: ___________ | **Versión FW**: ___________

---

## ✅ PREPARACIÓN RÁPIDA

- [ ] **Hardware**: Sniffer + Tag cargados, antenas separadas 2m, cable UART, cinta métrica 30m
- [ ] **Software**: STM32CubeIDE, terminal serial, branch `fix/detection-over-20m`
- [ ] **Marcadores**: 10m, 15m, 20m, 25m, 30m

---

## 🎯 HOJA DE RUTA: 5 ENFOQUES PARA MEJORAR DETECCIÓN >20m

### ⚙️ **Prioridad 1: Aumentar PRE_TIMEOUT** [✅ YA OPTIMIZADO]
**Estado actual**: `PRE_TIMEOUT_6M8 = 12 PAC` (96 símbolos de preámbulo)  
**¿Por qué ayuda?**: Espera más tiempo para detectar inicio de preámbulo en señales débiles  
**Máximo recomendado**: 16 PAC (100% del preámbulo de 128 símbolos)  
**Resultado pruebas**:
- PRE_TIMEOUT=5 @ 23m → Antena A: 0% éxito, B: 100%
- PRE_TIMEOUT=8 @ 20m → Antena A: 3% éxito, B: 10%
- PRE_TIMEOUT=12 (actual) → Esperado: 50% ambas antenas @ 25m

**✅ Acción**: Ya configurado al valor óptimo (12 PAC). No aumentar más a menos que tests muestren <50% éxito.

---

### ⚡ **Prioridad 2: Aumentar TX Power** [⚠️ PENDIENTE CALIBRACIÓN]
**Estado actual**: `TX_POWER = 0xFDFDFDFD` (Coarse gain=1, Fine gain=63)  
**¿Por qué ayuda?**: Mayor potencia transmitida = señal más fuerte en recepción a largas distancias  
**Máximo permitido**: `0xFFFFFFFF` (Coarse gain=3, Fine gain=63) - pero requiere **calibración RF**  
**⚠️ Advertencia**: 
- Aumentar power sin calibrar puede violar límites regulatorios (-41.3 dBm/MHz)
- Requiere medir espectro RF con analizador antes de incrementar

**🔧 Acción propuesta**:
```c
// En sniffer_tag.cpp y main.cpp línea ~56:
static dwt_txconfig_t dwt_tx_cfg = { 
    0x34,       /* PG delay */
    0xFEFEFEFE, /* TX power: Coarse gain=2, Fine gain=63 (moderado) */
    0x0         /* PG count */
};
```

**⚠️ CRÍTICO**: Medir con analizador de espectro antes de aprobar cambio. Si no tienes equipo, deja en 0xFDFDFDFD.

---

### 📡 **Prioridad 3: Mejorar RX Sensitivity / Calibración** [🧪 REQUIERE PRUEBA]
**Estado actual**: Sin calibración de sensibilidad documentada  
**¿Por qué ayuda?**: Mejor discriminación señal/ruido = detecta señales más débiles  
**Acciones sugeridas**:
1. **Calibración AGC (Automatic Gain Control)**:
   - Usar `dwt_rxcal()` para calibrar ganancia RX
   - Configurar umbrales de detección más agresivos
2. **Ajustar Threshold de detección**:
   - Bajar threshold en registro `DRX_CONF` para detectar señales más débiles
   - Trade-off: Mayor tasa de falsos positivos

**🔧 Test propuesto**:
- Capturar niveles RX con `dwt_readdiagnostics()` @ 25m
- Si First Path Power < -95 dBm → Señal muy débil, ajustar thresholds
- Si > -85 dBm → Problema no es RX sensitivity

---

### 📶 **Prioridad 4: Antenas de Mayor Ganancia** [💰 HARDWARE UPGRADE]
**Estado actual**: Antenas desconocidas (probablemente ≤2 dBi)  
**¿Por qué ayuda?**: Mejor ganancia = ±4-6 dB extra = duplica/triplica rango efectivo  
**Opciones**:
- **Bajo costo**: Antenas 3 dBi monopolo → +1 dB mejora (~10% más rango)
- **Medio costo**: Antenas 5 dBi direccionales → +3 dB mejora (~40% más rango)
- **Profesional**: Antenas 8 dBi patch → +6 dB mejora (2x rango)

**📋 Especificaciones requeridas**:
- Frecuencia: 6.0-6.5 GHz (Canal 5 UWB)
- Impedancia: 50Ω
- Conector: U.FL/IPEX compatible con DW3000

**💡 Tip**: Primero optimizar firmware/power, luego invertir en antenas si sigue fallando.

---

### 🔇 **Prioridad 5: Reducir Interferencias** [🧪 CAMBIO DE CANAL]
**Estado actual**: Canal 5 (6.5 GHz) hardcoded  
**¿Por qué ayuda?**: Otras redes UWB/WiFi 6E pueden interferir en 6.5 GHz  
**Acción propuesta**:
```c
// En sniffer_tag.cpp línea ~89 y ~51:
dwt_config_t dwt_cfg = { 
    9,  /* Channel number: cambiar de 5 → 9 (8.0 GHz) */
    // ... resto igual
};
```

**⚠️ Nota**: Canal 9 tiene menor rango máximo pero menos interferencia. Test en ambos canales.

**🔧 Test interferencia**:
1. Ejecutar sniffer en modo spectrum analyzer (si disponible)
2. Medir ocupación espectral en Canal 5 vs Canal 9
3. Elegir canal con menor ruido de fondo

---

## 🚀 TESTS SIMPLIFICADOS

| # | Prueba | Valida | Cómo hacerlo | ✅ Validado |
|---|--------|--------|--------------|-------------|
| **TEST-01** | **Timeouts DW3000 aumentados** | Detección funciona >20m | 1. Modificar `uwb3000Fxx.h` → 1000, 500, 12 (ambos equipos)<br>2. Compilar y flashear<br>3. Test 10-30m: ≥70% éxito a 25m | [ ] SÍ [ ] NO |
| **TEST-02** | **Validación de guardado** | No guarda datos incompletos | 1. Agregar validación en `main.cpp` líneas 542-546<br>2. Flashear sniffer<br>3. Test 3 escenarios: 15m, 28m, obstrucción | [ ] SÍ [ ] NO |
| **TEST-03** | **Query timeout aumentado** | Más tiempo para queries lentas | 1. Cambiar `query_timeout` 1000→2000 en `main.cpp`<br>2. Flashear sniffer<br>3. Comparar 25m: mejora ≥20% | [ ] SÍ [ ] NO |
| **TEST-05** | **Sistema de logging** | Diagnóstico de fallos | 1. Agregar `log_rx_result()` en `sniffer_tag.cpp`<br>2. Flashear sniffer<br>3. Capturar logs UART durante tests | [ ] SÍ [ ] NO |
| **TEST-06** | **Modo MULTIPLE obligatorio** | Siempre ambas antenas | 1. Eliminar `TAG_ONE_DETECTION` en `main.cpp`<br>2. Flashear sniffer<br>3. Test 15m: 100% ambas antenas | [ ] SÍ [ ] NO |
| **TEST-01B** | **Timeouts agresivos** | Último recurso si TEST-01 falla | 1. Modificar → 1400, 700, 16 (ambos)<br>2. Repetir TEST-01<br>3. ≥70% a 25m | [ ] SÍ [ ] NO |

---

## 📊 RESULTADO FINAL

### ✅ **Éxito Global**
- [ ] **OBJETIVO ALCANZADO**: Detección estable ≥70% a 25m
- [ ] **PARCIAL**: Funciona pero <70%
- [ ] **FALLIDO**: Sin mejora significativa

### 🔧 **Configuración Final**
```c
// Valores finales implementados
POLL_TX_TO_RESP_RX_DLY_UUS_6M8 = _____  // Ambos equipos
RESP_RX_TIMEOUT_UUS_6M8 = _____         // Ambos equipos
PRE_TIMEOUT_6M8 = _____                 // Ambos equipos
query_timeout = _____ ms                // Sniffer
TX_POWER = 0x_______                    // Si se modificó
```

### 📝 **Resumen de Cambios**
- [ ] TEST-01 implementado
- [ ] TEST-02 implementado
- [ ] TEST-03 implementado
- [ ] TEST-05 implementado
- [ ] TEST-06 implementado
- [ ] TEST-01B (solo si necesario)

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
