1. Introducción
SmartLocate es una solución de localización basada en tecnología Ultra-Wideband (UWB) diseñada para rastreo de personal en ambientes confinados, específicamente túneles mineros subterráneos. El sistema utiliza tecnología UWB para medición de distancias, validado con rango operativo de 38 metros en condiciones de línea de vista y 12 metros garantizado con orientación variable.

2. Objetivo
Proporcionar monitoreo de localización de personal dentro de la mina en puntos estratégicos mediante detección de distancia radial, permitiendo identificar y registrar la posición de trabajadores con autonomía de batería de 6.5 a 14 días para optimizar seguridad y eficiencia operacional en espacios confinados.

3. Usuarios y casos de uso
Los usuarios finales son trabajadores mineros en túneles subterráneos que portan el SmartTag durante su jornada laboral. El sistema permite monitorear su posición mediante Sniffers instalados en puntos estratégicos a 3 metros de altura, detectando personal en rangos de 12-38 metros. Los casos de uso incluyen: seguimiento de personal para optimización operacional, identificación rápida de trabajadores en zonas de riesgo durante emergencias, y registro de tránsito en áreas críticas. El sistema opera con detección automática cada 15 segundos sin intervención del usuario, transmitiendo la identificación de la persona y nivel de batería al servidor central vía LoRa.

4. Requisitos Funcionales

RF-01 Detección de Personal: El sistema debe detectar automáticamente SmartTags portados por trabajadores en rangos de 12 a 38 metros.

RF-02 Identificación de Trabajador: El sistema debe transmitir la identificación única del trabajador (Tag ID) en cada detección junto con el nivel de batería al servidor.

RF-03 Ciclo de Escaneo: El sistema debe ejecutar detecciones automáticas cada 15 segundos sin intervención del usuario.

RF-04 Monitoreo de Batería: El SmartTag debe reportar continuamente su nivel de batería en cada detección y proporcionar indicadores visuales (LED RGB) del estado energético (verde: OK, amarillo: medio, rojo: bajo).

RF-05 Autonomía Energética: El SmartTag debe mantener operación continua con autonomía de 6.5 a 14 días, incluyendo carga USB-C con tiempo de recarga completo aproximado de 2 horas.
