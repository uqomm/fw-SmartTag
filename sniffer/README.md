# Sniffer - Sistema de Detección y Monitoreo UWB/LoRa

## Descripción
Dispositivo maestro que detecta tags UWB, calcula distancias y transmite datos vía LoRa.

## Hardware
- **MCU**: STM32G474RET
- **UWB**: DW3000 (dos transceptores para distancia A/B)
- **LoRa**: SX1278
- **EEPROM**: Almacenamiento de configuración
- **UART**: Debug y comandos

## Características
- Detección continua de tags UWB
- Cálculo de distancia con compensación de clock offset
- Transmisión LoRa con intervalos configurables
- Serialización de múltiples tags por frame
- Modo one-detection y multiple-detection
- Keep-alive automático sin detecciones

## Configuración Actual
- Envío con tags: cada 7 s
- Envío sin detección: cada 15 s
- Timeout UWB: 100 ms

## Build
```bash
# Abrir en STM32CubeIDE
# Build → Debug/Release
# Flash con ST-LINK
```

## Ver También
- [DECISION_TIMEOUTS.md](DECISION_TIMEOUTS.md) - Explicación ajuste intervalos
- [CHANGELOG.md](CHANGELOG.md) - Historial de cambios
