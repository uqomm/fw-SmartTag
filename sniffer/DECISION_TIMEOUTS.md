# Ajuste Conservador de Timeouts LoRa

## Cambios Realizados
- Envío con tags: 5000 ms -> 7000 ms
- Envío sin detección (keep-alive): 10000 ms -> 15000 ms

## Motivo
Reducir tráfico de frames redundantes y dar más margen para que tags con ciclos de sueño largos reaparezcan, sin sacrificar demasiada latencia.

## Beneficios
- Menos frames vacíos.
- Mayor probabilidad de incluir tags lentos en cada envío.
- Incremento ligero, bajo riesgo (no afecta lógica de estados existente).

## Futuras Mejores Opciones
- Ajuste adaptativo según máximo sleep observado.
- Límite por número de tags antes de forzar envío.
- Registro de métricas para afinar (tags por frame, ratio vacíos).

