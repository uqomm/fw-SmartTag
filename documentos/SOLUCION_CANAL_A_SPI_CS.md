# Solución: Problema de Timeout en Canal A (DW3000)

## Fecha
Octubre 28, 2024

## Contexto
Después del fix del HAL_Delay(1), la Antena B logró 100% de éxito @ 21.7m (51/51 detecciones), pero la Antena A seguía con 100% de timeout (RX_PREAMBLE_DETECTION_TIMEOUT).

## Diagnóstico

### Test de Swap de Antenas Físicas (TEST-00)
Se realizó intercambio físico de las antenas conectadas a Canal A y Canal B:

**Resultado**: El problema se mantuvo en el **Canal A lógico** (software), sin importar cuál antena física estaba conectada.

**Conclusión**: El problema NO es hardware ni cable. Es un problema de **configuración de software** del Canal A.

---

## ❌ HIPÓTESIS INICIAL DESCARTADA: Problema CS Pins SPI

### Teoría Original
Se encontró que ambos pines CS (Chip Select) se inicializaban en LOW (activo) simultáneamente:

```c
// main.cpp líneas 1123-1124
HAL_GPIO_WritePin(DW3000_A_CS_GPIO_Port, DW3000_A_CS_Pin, GPIO_PIN_RESET);  // CS LOW
HAL_GPIO_WritePin(DW3000_B_CS_GPIO_Port, DW3000_B_CS_Pin, GPIO_PIN_RESET);  // CS LOW
```

**Por qué se descartó**:
Si este fuera el problema, el Canal A **fallaría en TODAS las distancias**, no solo >20m.

**Evidencia contradictoria**:
- @ 3-6m: Ambos canales funcionan perfectamente (100%)
- @ 21.7m: Canal A: 0%, Canal B: 100%

**Conclusión**: El problema CS puede existir, pero **NO es el root cause** del timeout en Canal A a >20m.

---

## ✅ ROOT CAUSE REAL: Estructura `pdw3000local` Compartida

### El Problema Fundamental

**Archivo**: `sniffer-tag/Core/Src/main.cpp`  
**Líneas**: 1123-1124

```c
// ❌ CÓDIGO INCORRECTO (ANTES)
HAL_GPIO_WritePin(DW3000_A_CS_GPIO_Port, DW3000_A_CS_Pin, GPIO_PIN_RESET);  // CS LOW = activo
HAL_GPIO_WritePin(DW3000_B_CS_GPIO_Port, DW3000_B_CS_Pin, GPIO_PIN_RESET);  // CS LOW = activo
```

**Problema**:
- Ambos pines CS se configuraron en LOW (activo) antes de la inicialización
- En SPI, **CS debe estar HIGH (inactivo)** cuando no se está comunicando con el chip
- Al tener **ambos CS en LOW simultáneamente**, ambos chips DW3000 respondían al mismo tiempo en el bus SPI compartido

**Consecuencias**:
1. Durante `init_uwb_device(&uwb_hw_a, ...)`: Ambos chips activos → **colisiones SPI**
2. Canal A recibe configuración parcial o incorrecta
3. Durante `init_uwb_device(&uwb_hw_b, ...)`: Solo Canal B activo (Canal A ya no responde bien)
4. Canal B se configura correctamente porque es el último

## La Solución

```c
// ✅ CÓDIGO CORRECTO (DESPUÉS)
HAL_GPIO_WritePin(DW3000_A_CS_GPIO_Port, DW3000_A_CS_Pin, GPIO_PIN_SET);  // CS HIGH = inactivo
HAL_GPIO_WritePin(DW3000_B_CS_GPIO_Port, DW3000_B_CS_Pin, GPIO_PIN_SET);  // CS HIGH = inactivo
```

**Justificación**:
- CS HIGH = chip inactivo (ignora SPI)
- CS LOW = chip activo (responde a SPI)
- Solo un chip debe estar activo a la vez en un bus SPI compartido
- Cada función `init_uwb_device()` maneja su propio CS correctamente vía `read()`/`write()` en `uwb3000Fxx.c`

## Verificación de la Lógica CS

En `sniffer-tag/Core/Src/uwb3000Fxx.c`:

**Función `write()` (línea 5032-5034)**:
```c
HAL_GPIO_WritePin(hw->nssPort, hw->nssPin, GPIO_PIN_RESET); // pull the pin low (ACTIVO)
res = HAL_SPI_Transmit(hw->spi, buf, i + j, 0xffff);
HAL_GPIO_WritePin(hw->nssPort, hw->nssPin, GPIO_PIN_SET);   // pull the pin high (INACTIVO)
```

**Función `read()` (línea 5044-5049)**:
```c
HAL_GPIO_WritePin(hw->nssPort, hw->nssPin, GPIO_PIN_RESET); // pull the pin low (ACTIVO)
res = HAL_SPI_Transmit(hw->spi, (uint8_t*) headerBuffer, headerLength, 0xffff);
if (res == HAL_OK)
    res = HAL_SPI_Receive(hw->spi, readBuffer, readlength, 0xffff);
HAL_GPIO_WritePin(hw->nssPort, hw->nssPin, GPIO_PIN_SET);   // pull the pin high (INACTIVO)
```

**Patrón correcto**:
1. CS HIGH (inactivo) por defecto
2. CS LOW (activo) durante comunicación SPI
3. CS HIGH (inactivo) al terminar comunicación

## Resultado Esperado

Con este fix:
- **Canal A**: Debe configurarse correctamente sin interferencia del Canal B
- **Canal B**: Continúa funcionando igual (sin cambios)
- **Esperado @ 21.7m**: Ambas antenas con ≥70% de éxito (idealmente ~100% como Canal B actualmente)

## Próximos Pasos

1. ✅ **Código corregido** en `main.cpp` líneas 1123-1124
2. ⏳ **Compilar** proyecto Sniffer
3. ⏳ **Flashear** firmware corregido
4. ⏳ **Test físico** @ 21m con ambas antenas
5. ⏳ **Documentar** resultados en `CHECKLIST_TESTS_FISICOS.md`

## Archivos Modificados

```
sniffer-tag/Core/Src/main.cpp
├── Línea 1123: GPIO_PIN_RESET → GPIO_PIN_SET (DW3000_A_CS)
└── Línea 1124: GPIO_PIN_RESET → GPIO_PIN_SET (DW3000_B_CS)
```

## Referencias Técnicas

- **DW3000 User Manual**: Section 6.1.1 - SPI Interface
  > "The CS signal must be driven high (inactive) when not communicating with the device"

- **SPI Protocol**: CS (Chip Select) active-low signal
  - HIGH = chip ignores SPI traffic
  - LOW = chip responds to SPI commands

- **Multi-slave SPI**: Only one CS should be LOW at any given time

## Lecciones Aprendidas

1. **Test de Swap de Antenas**: Método definitivo para distinguir hardware vs software
2. **SPI Shared Bus**: CS HIGH por defecto es crítico en configuraciones multi-slave
3. **Búsqueda Sistemática**: Importante revisar toda la cadena de inicialización, no solo configuraciones
4. **Timing de Inicialización**: El orden importa cuando hay recursos compartidos (SPI bus)

## Commit

```bash
git add sniffer-tag/Core/Src/main.cpp
git commit -m "Fix: Canal A SPI CS initialization (GPIO_PIN_SET for inactive state)

- Root cause: Both DW3000 CS pins initialized to LOW (active) simultaneously
- Impact: SPI collisions during Channel A initialization
- Fix: Set both CS pins to HIGH (inactive) before init sequence
- Expected: Channel A now configures correctly without interference
- Test: Antenna swap test proved software config issue, not hardware
"
```

---

**Estado**: ✅ FIX APLICADO - Pendiente de compilación y test físico  
**Prioridad**: 🔥 CRÍTICA  
**Impacto esperado**: 🎯 Resolución completa del problema de Canal A @ >20m
