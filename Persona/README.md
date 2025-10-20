# Persona - Tag UWB de Bajo Consumo

## Descripción
Tag portátil con UWB, gestión de batería y modos de sleep configurables para maximizar autonomía.

## Hardware
- **MCU**: STM32U535VET (ultra-bajo consumo)
- **UWB**: DW3000
- **Carga**: BQ25150 (gestión de batería Li-Ion)
- **LED**: WS2812 RGB (indicador de batería/estado)
- **Timer**: LPTIM1 para sleep preciso

## Características
- Respuesta a queries del sniffer con timestamps UWB
- Sleep adaptativo según detección:
  - `sleep_time_recived`: 15 s (configurado por sniffer)
  - `sleep_time_not_recived`: 500 ms (fallback)
- Modo STOP con wake-up automático
- Ship mode para almacenamiento (ultra-bajo consumo)
- Indicación LED RGB según nivel de batería

## Estados
1. `TAG_WAIT_FOR_FIRST_DETECTION`: Espera query inicial
2. `TAG_WAIT_SEND_TX`: Espera configuración de sleep
3. `TAG_WAIT_FOR_TIMESTAMPT_QUERY`: Responde con distancias
4. `TAG_SLEEP_RECIVED/NOT_RECIVED`: Entra en sleep
5. `TAG_SHIP_MODE_SET`: Apagado profundo

## Consumo Típico
- Activo (UWB RX/TX): ~150 mA
- Sleep (STOP mode): <10 µA
- Ship mode: <1 µA

## Build
```bash
# Abrir en STM32CubeIDE
# Build → Debug/Release
# Flash con ST-LINK
```

## Ver También
- [CHANGELOG.md](CHANGELOG.md) - Historial de cambios
