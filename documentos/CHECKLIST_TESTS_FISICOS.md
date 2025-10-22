# Checklist de Tests Físicos - Corrección Detección >20m

**Fecha**: ___________ | **Responsable**: ___________ | **Versión FW**: ___________

---

## ✅ PREPARACIÓN

### Hardware Básico
- [ ] Sniffer con batería cargada + antenas A y B (separación 2m)
- [ ] Tag (Persona) con batería cargada
- [ ] Cable UART para logs
- [ ] Cinta métrica ≥30m
- [ ] Marcadores de distancia a: 10m, 15m, 20m, 25m, 30m

### Software Básico
- [ ] STM32CubeIDE funcionando
- [ ] Software serial (PuTTY/CoolTerm) para logs
- [ ] Git branch `fix/detection-over-20m` creado
- [ ] Commit baseline hecho

---

---

## 🔴 TEST-01: TIMEOUTS AUMENTADOS (Solución 1A)

### Implementación
- [ ] Modificar `sniffer/Core/Inc/uwb3000Fxx.h` → 1000, 500, 8
- [ ] Modificar `Persona/Core/Inc/uwb3000Fxx.h` → 1000, 500, 8 (valores idénticos)
- [ ] Compilar ambos sin warnings
- [ ] Flashear Sniffer y Persona
- [ ] Verificar sincronización con test rápido a 10m

### Test de Distancias (3 min por distancia)

| Distancia | Éxito A | Éxito B | Criterio | ✅ PASS/FAIL |
|-----------|---------|---------|----------|-------------|
| 10m  | ___/___  | ___/___  | 100% ambas | [ ] PASS [ ] FAIL |
| 15m  | ___/___  | ___/___  | 100% ambas | [ ] PASS [ ] FAIL |
| 20m  | ___/___  | ___/___  | ≥90% ambas | [ ] PASS [ ] FAIL |
| **25m** | ___/___  | ___/___  | **≥70% ambas** | [ ] PASS [ ] FAIL |
| 27m  | ___/___  | ___/___  | ≥60% ambas | [ ] PASS [ ] FAIL |
| 30m  | ___/___  | ___/___  | ≥50% ambas | [ ] PASS [ ] FAIL |

**Resultado**: [ ] EXITOSO (≥70% a 25m) [ ] PARCIAL [ ] FALLIDO → Si FAIL ir a TEST-01B

**Logs guardados**: test-01-session-X.log

---

## 🔴 TEST-02: VALIDACIÓN DE GUARDADO (Solución 2A)

### Implementación
- [ ] Agregar validación en `sniffer/Core/Src/main.cpp` líneas 542-546
- [ ] Compilar y flashear solo Sniffer

### Escenarios

**Escenario 1 (15m, 20 ciclos)**: Distancia óptima
- [ ] Tags guardados: ___/20
- [ ] Tags con ambas antenas: ___/20 (debe ser 100%)
- [ ] ✅ [ ] PASS [ ] FAIL

**Escenario 2 (28m, 30 ciclos)**: Distancia límite
- [ ] Tags guardados: ___/30
- [ ] Tags descartados: ___/30
- [ ] Tags guardados con solo 1 antena: ___ (debe ser 0)
- [ ] ✅ [ ] PASS [ ] FAIL

**Escenario 3 (20m, 10 ciclos)**: Obstrucción antena B
- [ ] Bloquear antena B con metal
- [ ] Tags descartados: ___/10 (debe ser 10)
- [ ] Logs muestran "A=X, B=0": [ ] SÍ
- [ ] ✅ [ ] PASS [ ] FAIL

**Resultado**: [ ] Todos PASS [ ] Alguno FAIL

---

## 🟠 TEST-03: QUERY_TIMEOUT AUMENTADO (Solución 3B - Simple)

### Implementación
- [ ] Cambiar `query_timeout` de 1000 a 2000 en `main.cpp` línea 347
- [ ] Compilar y flashear Sniffer

### Test Comparativo (25m, 20 ciclos cada uno)

**Baseline (timeout 1000ms)**:
- [ ] Ciclos completos: ___/20
- [ ] % Éxito: ___%

**Mejorado (timeout 2000ms)**:
- [ ] Ciclos completos: ___/20
- [ ] % Éxito: ___%

**Resultado**: [ ] Mejora ≥20% [ ] Sin mejora significativa

---

## 🟡 TEST-05: LOGGING (con cualquier test)

### Implementación
- [ ] Agregar `log_rx_result()` en `sniffer_tag.cpp`
- [ ] Compilar y flashear Sniffer
- [ ] Capturar logs UART durante tests

### Verificación
- [ ] Logs muestran: tag_id, antena (A/B), tipo error, tiempo
- [ ] Se identifica cuál antena falla más
- [ ] Se identifica tipo timeout más frecuente

**Ejemplo esperado**:
```
[A] Tag 12345678: RX OK, lecturas=1, tiempo=45ms
[B] Tag 12345678: PREAMBULO TIMEOUT
```

**Resultado**: [ ] Logs útiles [ ] Logs incompletos

---

## 🟡 TEST-06: ELIMINAR MODO ONE_DETECTION (Opcional)

### Implementación
- [ ] Comentar/eliminar `TAG_ONE_DETECTION` en `main.cpp`
- [ ] Compilar y flashear Sniffer

### Test (15m, 20 ciclos)
- [ ] Tags guardados con ambas antenas: ___/20 (debe ser 100%)
- [ ] ✅ [ ] PASS [ ] FAIL

---

## 🔴 TEST-01B: TIMEOUTS AGRESIVOS (Solo si TEST-01 FAIL)

### Implementación
- [ ] Modificar ambos equipos → 1400, 700, 12
- [ ] Flashear ambos

### Test
- [ ] Repetir TEST-01 completo
- [ ] 25m: ___% éxito (debe ser ≥70%)
- [ ] ✅ [ ] PASS [ ] FAIL

---

## 📊 RESUMEN FINAL

### Tests Completados
- [ ] TEST-01: Timeouts aumentados
- [ ] TEST-02: Validación guardado
- [ ] TEST-03: query_timeout
- [ ] TEST-05: Logging
- [ ] TEST-06: Modo MULTIPLE (opcional)
- [ ] TEST-01B: Timeouts agresivos (si aplica)

### Resultado Global
- [ ] ✅ Objetivo alcanzado: Detección estable a **25m con ≥70% éxito**
- [ ] ⚠️ Parcial: Funciona pero <70%
- [ ] ❌ Fallido: No mejora

### Soluciones en Producción
- [ ] Solución 1A (timeouts +50%) o 1B (+100%)
- [ ] Solución 2A (validación obligatoria)
- [ ] Solución 3B (query_timeout=2000)
- [ ] Solución 5A (logging)
- [ ] Solución 6A (solo MULTIPLE)

### Valores Finales en Código
```
POLL_TX_TO_RESP_RX_DLY_UUS_6M8 = _____ (ambos equipos)
RESP_RX_TIMEOUT_UUS_6M8 = _____ (ambos equipos)
PRE_TIMEOUT_6M8 = _____ (ambos equipos)
query_timeout = _____ ms (sniffer)
```

### Commit y Release
- [ ] Commit final realizado
- [ ] Tag `v0.4.0-fix-distance` creado
- [ ] Merge a `dev`
- [ ] CHANGELOG.md actualizado

---

## 📝 NOTAS

**Problemas encontrados**:
```
________________________________________________________________________
```

**Lecciones aprendidas**:
```
________________________________________________________________________
```

**Próximos pasos**:
```
________________________________________________________________________
```

---

**Completado el**: ___________ por: ___________
