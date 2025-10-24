# 🧪 Checklist Tests Físicos - Corrección Detección >20m

**Fecha**: ___________ | **Responsable**: ___________ | **Versión FW**: ___________

---

## ✅ PREPARACIÓN RÁPIDA

- [ ] **Hardware**: Sniffer + Tag cargados, antenas separadas 2m, cable UART, cinta métrica 30m
- [ ] **Software**: STM32CubeIDE, terminal serial, branch `fix/detection-over-20m`
- [ ] **Marcadores**: 10m, 15m, 20m, 25m, 30m

---

## � TESTS SIMPLIFICADOS

| # | Prueba | Valida | Cómo hacerlo | ✅ Validado |
|---|--------|--------|--------------|-------------|
| **TEST-01** | **Timeouts DW3000 aumentados** | Detección funciona >20m | 1. Modificar `uwb3000Fxx.h` → 1000, 500, 8 (ambos equipos)<br>2. Compilar y flashear<br>3. Test 10-30m: ≥70% éxito a 25m | [ ] SÍ [ ] NO |
| **TEST-02** | **Validación de guardado** | No guarda datos incompletos | 1. Agregar validación en `main.cpp` líneas 542-546<br>2. Flashear sniffer<br>3. Test 3 escenarios: 15m, 28m, obstrucción | [ ] SÍ [ ] NO |
| **TEST-03** | **Query timeout aumentado** | Más tiempo para queries lentas | 1. Cambiar `query_timeout` 1000→2000 en `main.cpp`<br>2. Flashear sniffer<br>3. Comparar 25m: mejora ≥20% | [ ] SÍ [ ] NO |
| **TEST-05** | **Sistema de logging** | Diagnóstico de fallos | 1. Agregar `log_rx_result()` en `sniffer_tag.cpp`<br>2. Flashear sniffer<br>3. Capturar logs UART durante tests | [ ] SÍ [ ] NO |
| **TEST-06** | **Modo MULTIPLE obligatorio** | Siempre ambas antenas | 1. Eliminar `TAG_ONE_DETECTION` en `main.cpp`<br>2. Flashear sniffer<br>3. Test 15m: 100% ambas antenas | [ ] SÍ [ ] NO |
| **TEST-01B** | **Timeouts agresivos** | Último recurso si TEST-01 falla | 1. Modificar → 1400, 700, 12 (ambos)<br>2. Repetir TEST-01<br>3. ≥70% a 25m | [ ] SÍ [ ] NO |

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
query_timeout = _____ ms               // Sniffer
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
