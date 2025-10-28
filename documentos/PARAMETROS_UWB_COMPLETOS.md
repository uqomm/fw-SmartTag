# Parámetros UWB Completos - DW3000

## Resumen Ejecutivo

Este documento lista TODOS los parámetros de timeout y delay disponibles en el firmware para el chip DW3000, incluyendo variantes para diferentes velocidades de transmisión.

**Estado actual del sistema:**
- ✅ Velocidad configurada: **6.8 Mbps** (RATE_6M8) - hardcoded
- ❌ Velocidad 850K: Definida pero NO utilizada en el código
- 📊 GUI actual: Incluye los 8 parámetros principales (6.8M)

---

## 1. Parámetros Actualmente Documentados y en GUI

### 1.1. RX Timeouts (Tiempos de espera de recepción)

| Parámetro | Valor Default | Unidad | Documentado | En GUI | En Firmware |
|-----------|---------------|--------|-------------|---------|-------------|
| `PRE_TIMEOUT_6M8` | 12 (antes: 5) | PAC | ✅ Sí | ✅ Sí | ✅ USADO |
| `RESP_RX_TIMEOUT_UUS_6M8` | 600 (antes: 300) | μs | ✅ Sí | ✅ Sí | ✅ USADO |
| `FINAL_RX_TIMEOUT_UUS_6M8` | 220 | μs | ✅ Sí | ✅ Sí | ✅ USADO |

**Documentación:** 
- `RELACION_TIMEOUTS_UWB.md`
- `ANALISIS_IMPACTO_TIEMPOS_UWB.md`

---

### 1.2. TX→RX Delays (Retrasos entre transmisión y recepción)

| Parámetro | Valor Default | Unidad | Documentado | En GUI | En Firmware |
|-----------|---------------|--------|-------------|---------|-------------|
| `POLL_TX_TO_RESP_RX_DLY_UUS_6M8` | 700 | μs | ✅ Sí | ✅ Sí | ✅ USADO |
| `RESP_TX_TO_FINAL_RX_DLY_UUS_6M8` | 500 | μs | ✅ Sí | ✅ Sí | ✅ USADO |

**Descripción:**
- Tiempo entre el **final de la transmisión** y la **apertura de la ventana de recepción**
- Críticos para sincronización del protocolo DS-TWR

**Documentación:** 
- `RELACION_TIMEOUTS_UWB.md`
- `PROTOCOLO_UWB_COMPLETO.md`

---

### 1.3. RX→TX Delays (Retrasos entre recepción y transmisión)

| Parámetro | Valor Default | Unidad | Documentado | En GUI | En Firmware |
|-----------|---------------|--------|-------------|---------|-------------|
| `POLL_RX_TO_RESP_TX_DLY_UUS_6M8` | 900 | μs | ✅ Sí | ✅ Sí | ✅ USADO |
| `RESP_RX_TO_FINAL_TX_DLY_UUS_6M8` | 700 | μs | ⚠️ Parcial | ✅ Sí | ✅ USADO (implícito) |

**Descripción:**
- Tiempo entre la **recepción de un mensaje** y el **inicio de transmisión del siguiente**
- Usados en cálculos de timestamps para transmisión diferida

**Documentación:** 
- `RELACION_TIMEOUTS_UWB.md`

---

### 1.4. Inter-Ranging Delay (Retardo entre ciclos de ranging)

| Parámetro | Valor Default | Unidad | Documentado | En GUI | En Firmware |
|-----------|---------------|--------|-------------|---------|-------------|
| `RNG_DELAY_MS` | 1000 | ms | ❌ No | ✅ Sí | ❌ NO USADO |

**⚠️ IMPORTANTE:** 
- Este parámetro está **definido pero NO implementado** en el firmware
- El firmware usa `sleep_time_recived` y `sleep_time_not_recived` en su lugar
- Comandos LTEL existentes: 0x19 (CHANGE_SLEEP_TIME_RECIVED), 0x20 (CHANGE_SLEEP_NOT_RECIVED)
- **Recomendación:** Considerar eliminar del GUI o implementar comando 0x37

---

## 2. Parámetros NO Documentados (Variantes 850K)

### 2.1. RX Timeouts - 850K Data Rate

| Parámetro | Valor Default | Unidad | Documentado | En GUI | En Firmware |
|-----------|---------------|--------|-------------|---------|-------------|
| `PRE_TIMEOUT_850K` | 0 | PAC | ❌ No | ❌ No | ✅ Definido |
| `RESP_RX_TIMEOUT_UUS_850K` | 2000 | μs | ❌ No | ❌ No | ✅ Definido |
| `FINAL_RX_TIMEOUT_UUS_850K` | 5000 | μs | ❌ No | ❌ No | ✅ Definido |

---

### 2.2. TX→RX Delays - 850K Data Rate

| Parámetro | Valor Default | Unidad | Documentado | En GUI | En Firmware |
|-----------|---------------|--------|-------------|---------|-------------|
| `POLL_TX_TO_RESP_RX_DLY_UUS_850K` | 60 | μs | ❌ No | ❌ No | ✅ Definido |
| `RESP_TX_TO_FINAL_RX_DLY_UUS_850K` | 60 | μs | ❌ No | ❌ No | ✅ Definido |

---

### 2.3. RX→TX Delays - 850K Data Rate

| Parámetro | Valor Default | Unidad | Documentado | En GUI | En Firmware |
|-----------|---------------|--------|-------------|---------|-------------|
| `POLL_RX_TO_RESP_TX_DLY_UUS_850K` | 900 | μs | ❌ No | ❌ No | ✅ Definido |
| `RESP_RX_TO_FINAL_TX_DLY_UUS_850K` | 1500 | μs | ❌ No | ❌ No | ✅ Definido |

---

## 3. Análisis de Uso en Firmware

### 3.1. Modo 6.8 Mbps (ACTUAL)

```c
// sniffer_tag.cpp - líneas 71, 105
if (tag_init(&dwt_cfg, &dwt_tx_cfg, pdw3000local, DEV_UWB3000F27, RATE_6M8))
```

**Código en uwb3000Fxx.c:**
```c
if (rate == RATE_6M8) {
    dwt_setrxaftertxdelay(POLL_TX_TO_RESP_RX_DLY_UUS_6M8);
    dwt_setrxtimeout(RESP_RX_TIMEOUT_UUS_6M8);
    dwt_setpreambledetecttimeout(PRE_TIMEOUT_6M8);
}
```

**✅ Estado:** Completamente implementado y funcional

---

### 3.2. Modo 850K (NO UTILIZADO)

```c
// uwb3000Fxx.c - líneas 115-117 (código presente pero nunca ejecutado)
else {
    dwt_setrxaftertxdelay(POLL_TX_TO_RESP_RX_DLY_UUS_850K);
    dwt_setrxtimeout(RESP_RX_TIMEOUT_UUS_850K);
    dwt_setpreambledetecttimeout(PRE_TIMEOUT_850K);
}
```

**❌ Estado:** Definido pero nunca usado (sistema hardcoded a RATE_6M8)

---

## 4. Comparación de Valores: 6.8M vs 850K

| Parámetro | 6.8 Mbps | 850K | Diferencia | Razón |
|-----------|----------|------|------------|-------|
| PRE_TIMEOUT | 12 PAC | 0 PAC | -100% | 850K más lento = más tiempo inherente |
| RESP_RX_TIMEOUT | 600 μs | 2000 μs | +233% | Frames más largos a 850K |
| FINAL_RX_TIMEOUT | 220 μs | 5000 μs | +2173% | Frames más largos a 850K |
| POLL_TX_RESP_RX | 700 μs | 60 μs | -91% | ⚠️ Parece error? |
| RESP_TX_FINAL_RX | 500 μs | 60 μs | -88% | ⚠️ Parece error? |
| POLL_RX_RESP_TX | 900 μs | 900 μs | 0% | Mismo valor |
| RESP_RX_FINAL_TX | 700 μs | 1500 μs | +114% | Mayor tiempo de procesamiento |

**⚠️ Nota:** Los valores de 60 μs para TX→RX delays en 850K parecen sospechosamente bajos considerando que los frames son más largos a 850K. Revisar si hay error en definiciones.

---

## 5. Recomendaciones

### 5.1. Corto Plazo
- ✅ **Mantener GUI actual** con los 8 parámetros 6.8M
- ⚠️ **Resolver inconsistencia RNG_DELAY_MS:** Implementar comando 0x37 o eliminar del GUI
- 📝 **Documentar** este archivo como referencia completa

### 5.2. Mediano Plazo
- 🔄 Si se planea **soportar 850K** en el futuro:
  - Agregar selector de data rate en GUI (6.8M / 850K)
  - Agregar pestañas separadas para parámetros 850K
  - Implementar comandos LTEL adicionales (0x38-0x3E)
  - Revisar valores de 60 μs que parecen incorrectos

### 5.3. Largo Plazo
- 📊 **Configuración dinámica de data rate:** Permitir cambio en runtime
- 🔧 **Optimización automática:** Ajustar timeouts según distancia medida
- 📈 **Telemetría:** Enviar estadísticas de timeouts para análisis

---

## 6. Comandos LTEL Propuestos (Futuros)

### 6.1. Comandos 6.8M (Actuales - 0x30 a 0x37)
| Comando | Parámetro | Estado |
|---------|-----------|--------|
| 0x30 | SET_PRE_TIMEOUT | ✅ En GUI |
| 0x31 | SET_RESP_RX_TIMEOUT | ✅ En GUI |
| 0x32 | SET_FINAL_RX_TIMEOUT | ✅ En GUI |
| 0x33 | SET_POLL_TX_RESP_RX_DLY | ✅ En GUI |
| 0x34 | SET_RESP_TX_FINAL_RX_DLY | ✅ En GUI |
| 0x35 | SET_POLL_RX_RESP_TX_DLY | ✅ En GUI |
| 0x36 | SET_RESP_RX_FINAL_TX_DLY | ✅ En GUI |
| 0x37 | SET_RNG_DELAY | ⚠️ En GUI pero NO implementado |

### 6.2. Comandos 850K (Propuestos - 0x38 a 0x3F)
| Comando | Parámetro | Estado |
|---------|-----------|--------|
| 0x38 | SET_PRE_TIMEOUT_850K | ❌ No implementado |
| 0x39 | SET_RESP_RX_TIMEOUT_850K | ❌ No implementado |
| 0x3A | SET_FINAL_RX_TIMEOUT_850K | ❌ No implementado |
| 0x3B | SET_POLL_TX_RESP_RX_DLY_850K | ❌ No implementado |
| 0x3C | SET_RESP_TX_FINAL_RX_DLY_850K | ❌ No implementado |
| 0x3D | SET_POLL_RX_RESP_TX_DLY_850K | ❌ No implementado |
| 0x3E | SET_RESP_RX_FINAL_TX_DLY_850K | ❌ No implementado |
| 0x3F | SET_DATA_RATE | ❌ No implementado (cambiar 6.8M ↔ 850K) |

---

## 7. Referencias

### Documentación Existente
- `RELACION_TIMEOUTS_UWB.md` - Interacción entre timeouts
- `ANALISIS_IMPACTO_TIEMPOS_UWB.md` - Impacto a >20m
- `PROTOCOLO_UWB_COMPLETO.md` - Flujo completo DS-TWR

### Archivos de Código
- `sniffer-tag/Core/Inc/uwb3000Fxx.h` (líneas 690-750) - Definiciones
- `sniffer-tag/Core/Src/uwb3000Fxx.c` (líneas 110-120) - Uso de timeouts
- `sniffer-tag/Core/Src/sniffer_tag.cpp` (líneas 71, 105) - Inicialización

### GUI
- `tools/sniffer_tag_gui.py` - Interfaz de configuración

---

## 8. Historial de Cambios

| Fecha | Cambio | Razón |
|-------|--------|-------|
| 27-28 Oct 2024 | PRE_TIMEOUT: 5 → 12 PAC | Mejorar detección >20m (0% → 50% éxito) |
| 27-28 Oct 2024 | RESP_RX_TIMEOUT: 300 → 600 μs | Mayor tolerancia a distancias largas |
| 28 Oct 2024 | Tooltips agregados en GUI | Facilitar comprensión de parámetros |

---

> **Nota Final:** Este documento es la referencia completa de TODOS los parámetros UWB disponibles en el firmware DW3000. Para uso diario, solo los parámetros 6.8M son relevantes hasta que se implemente soporte 850K.
