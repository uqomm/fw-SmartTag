			Cambios y pruebas realizadas con SmartLocate

*22-10-2025

- ESTADO BASE DEL SISTEMA: Sistema estable en cuanto a detección hasta +-20 metros de distancia, pero en software no había "permanencia" de los tags detectados.
con Arturo se analizaron los tiempos de detección de tags y de envío al servidor cuando no hay ningún tag, modificando este tiempo se arreglo este problema.

- Se realiza un análisis completo del funcionamiento del sistema, esto culmina en varios documentos de análisis de la comunicación entre el sniffer y el tag, además de un documento detallado del flujo la máquina de estados de cada equipo.

*23-10-2025

- En base este análisis, se llega a la conclusión de implementar un logging mediante uart de estado de las fallas de detección. 
Luego de implementar el Logging, de probar y de analizar la comunicación entre tag y sniffer, se llega a la conclusión de probar cambios en los timeouts de respuesta de los transmisores. (se tienen mediciones de antena b, pero no de antena a cuando se está sobre 20 metros) (intermitencia).

- Se efectúa el primer cambio en PRE_TIMEOUT_6M8 pasando de 5 a 8 en ambos equipos (sniffer y tag) y se obtienen resultados similares, cambiando los resultdos y teniendo algubnas mediciones de la antena a.

- Estas pruebas se realizaron con el camión puesto, lo cual puede interferir. 

*24-10-2025

- Dado los resultados anteriores, se va a intentar mejorar la detección aumentando el valor PRE_TIMEOUT_6M8 a 12, no se lograron hacer pruebas hoy por pruebas que se están realizando en el head end.

*27-10-2025
- Todas las pruebas anteriores se estaban realizando con las antenas del AP, por orden del Daniel se cambian a antenas anteriores.

- Se hicieron las mismas pruebas aumentando la cantidad de simbolos PRE_TIMEOUT_6M8 a 12, en la bajada del camión (22.3mts la antena b) se instaló un tag encima de un taburete con una batería externa, en una posición lo detecta (con 1 antena) mientras que si se gira en otra posición no lo detecta ninguna antena. Considerar que en todo caso el tag estuvo "mirando, con la luz apuntando " en dirección al sniffer.

*28-10-2025
- Se siguió con el analisis de los timeouts y posibles problemas con la comunicación entre el sniffer y el tag persona.

- Se modificó el main.cpp del tag persona, se elimino un hal_delay(1) que se enocntraba en la comunicación  MULTIPLE_DETECTION en caso de falla, pero los resultados son similares a los anteriores. En el mismo taburete y misma posición (21.7 metros apuntando al sniffer) solo detecta la antena B.

- Para descartar un posible probelma de hardware, ya que se detectan errores constantes con la antena A, se hace un cruzamiento físico de las antenas. Luego de este cruzamiento de antenas, se obtiene el mismo resultado, solo toma detecciones la antena b y la antena a sigue con RX_PREAMBLE_DETECTION

- Mediante otro analisis se encontró que los transmisores utilizan un método de calibración el cual es propio de cada transmisor, en conjunto a esto, se estaba utilizando la misma estructura para ambos transmisores en el sniffer, lo cual es incorrecto en caso de que la calibración afecte mucho en la detección. se opta por que cada transmisor utilice su propia estructura para calibrarse. Los rersultados siguen siendo los mismos, esto no solucionó el error.
Se contrastó que se estaba utilizadno bien la estructura asiganda a cada transmisor mediante un log en la inicialización de cada transmisor.



*29-10-2025

- 