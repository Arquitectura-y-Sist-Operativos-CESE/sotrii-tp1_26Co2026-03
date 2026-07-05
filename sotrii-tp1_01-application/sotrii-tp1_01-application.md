# Análisis del Sistema de Comunicación I2C con FreeRTOS

Este repositorio contiene una implementación de un **sistema basado en eventos (Event-Triggered System)** diseñado para ejecutarse sobre el sistema operativo en tiempo real **FreeRTOS**, utilizando las librerías de abstracción de hardware **HAL de STM32**.

El objetivo central de esta arquitectura es resolver el acoplamiento directo entre la lógica de aplicación y los periféricos de hardware mediante el patrón de diseño **Productor-Consumidor**, utilizando colas de mensajes (*Queues*) como intermediarias.

---

## 🏗️ Arquitectura y Flujo de Datos

A continuación se presenta el esquema conceptual de cómo interactúan las tareas de alto nivel (Aplicación) con las de bajo nivel (Drivers/Hardware) de forma asíncrona:

```text
 [ Capa de Aplicación ]              [ Interfaz / API ]             [ Capa de Hardware ]
+---------------------+             +------------------+           +--------------------+
|     task_sender     |             |    open_i2c()    |           |    task_i2c_tx     |
| (Genera dato 0x55)  |             | (Crea colas/tsks)|           |  (Espera en cola)  |
+----------+----------+             +--------+---------+           +---------+----------+
           |                                 |                               |
           v (Llama a)                       v (Asigna)                      v (Recibe de)
    write_i2c() ------------------------> queue_tx -------------------------->|
                                     (Cola de Mensajes)                      | (Llama a)
                                                                             v
                                                                   HAL_I2C_Master_Transmit()
                                                                             |
                                                                             v
                                                                     [ Bus Físico I2C ]
```
## 1. Arquitectura del Sistema

A continuación se detalla el funcionamiento de cada módulo, agrupados por su rol en el sistema:

### 1.1 Inicialización y Gestión del Sistema Operativo
Estos archivos configuran el entorno de ejecución, arrancan las tareas y gestionan eventos del RTOS.

* **`app.c` (Punto de entrada):**
  * Contiene la función `app_init()`, que inicializa contadores globales.
  * Crea las tareas principales de la aplicación (`Task Sender` y `Task Receiver`) usando `xTaskCreate`, ambas con prioridad `tskIDLE_PRIORITY + 1ul`.
  * Llama a la inicialización del hardware I2C (`open_i2c(&hi2c1)`) y configura las interrupciones (`app_it_init()`).
* **`freertos.c` (Hooks del RTOS):**
  * `vApplicationIdleHook()`: Incrementa `g_task_idle_cnt` cuando el sistema está en reposo.
  * `vApplicationTickHook()`: Incrementa `g_app_tick_cnt` en cada interrupción del reloj del OS.
  * `vApplicationStackOverflowHook()`: Fuerza un cuelgue (`configASSERT(0)`) si una tarea desborda su memoria de pila, facilitando la depuración.
* **`app_it.c` (Gestión de Interrupciones):**
  * Define `HAL_GPIO_EXTI_Callback` para manejar interrupciones externas (ej. el botón `BTN_A_PIN`).

### 1.2 Capa de Aplicación (Lógica de Usuario)
Tareas de alto nivel que generan o consumen datos, aisladas del hardware físico.

* **`task_sender.c` (Emisor):**
  * Simula el envío de datos a un expansor PCF8574 (dirección `0x27`).
  * En un bucle infinito, alterna los bits de una variable (`~dev_data`) y la envía mediante `write_i2c()`.
  * Se bloquea por 250 milisegundos (`TASK_SENDER_DEL_MAX`) en cada ciclo.
* **`task_receiver.c` (Receptor):**
  * Tarea encargada de consumir los datos provenientes del bus I2C invocando a la API del driver.

### 1.3 Interfaz e Implementación del Driver I2C
Desacopla la aplicación principal de los bloqueos del hardware físico utilizando colas.

* **`task_i2c_interface.h` / `.c` (API del Controlador):**
  * `open_i2c()`: Crea las colas de transmisión (`queue_tx`, tamaño 5) y recepción (`queue_rx`, tamaño 10), y lanza las tareas de hardware (`Task I2C Tx` y `Task I2C Rx`).
  * `write_i2c()`: Empaqueta dirección y dato, enviándolos de forma segura a `queue_tx` mediante `xQueueSend`.
  * `read_i2c()`: Extrae datos de la cola `queue_rx` bloqueando la tarea solicitante si no hay datos disponibles (`xQueueReceive`).
* **`task_i2c.c` (Hardware / Gatekeepers):**
  * **`task_i2c_tx` (Transmisión):** Espera datos en `queue_tx`. Al recibirlos, despierta y ejecuta físicamente `HAL_I2C_Master_Transmit()`.
  * **`task_i2c_rx` (Recepción):** Tarea Gatekeeper que lee periódicamente el bus I2C mediante `HAL_I2C_Master_Receive()` y deposita los resultados en `queue_rx`.

---

## 2. Resolución del Paso 03: Lógica de Recepción (RX)

Para completar el flujo de recepción desde el hardware hasta la capa de aplicación, se implementaron los siguientes cambios en el código base:

### 2.1 Actualización de la Interfaz (`task_i2c_interface.h` y `.c`)
Se modificó la firma de `read_i2c` para devolver el dato leído mediante un puntero, extrayéndolo de la cola de recepción.

**`task_i2c_interface.h`:**
```c
extern void read_i2c(I2C_HandleTypeDef *h_i2c_device, uint8_t *p_data);
```
---
# Observaciones y Medición WCET

## 1. Comportamiento Observado
Al implementar el driver I2C utilizando el patrón Gatekeeper y sincronización mediante colas (`xQueueSend` / `xQueueReceive`), se observó el siguiente comportamiento:
* **Sincronización:** Las tareas de aplicación (`task_sender` y `task_receiver`) pasan correctamente al estado **Blocked** gracias al uso de `portMAX_DELAY`, liberando la CPU.
* **Gatekeeper:** El acceso físico al bus I2C queda estrictamente limitado a las tareas internas del driver (`task_i2c_tx` y `task_i2c_rx`), evitando colisiones o condiciones de carrera en el hardware.
* **Robustez:** La tarea RX valida el estado `HAL_OK` y descarta los bytes vacíos (`0x00`), optimizando el uso de la cola de recepción.

## 2. Medición de WCET (Worst Case Execution Time)
Las mediciones se realizaron utilizando el contador de ciclos (DWT) del microcontrolador STM32, rodeando las primitivas bloqueantes de la HAL (`HAL_I2C_Master_Transmit` y `HAL_I2C_Master_Receive`).

* **Función de Interfaz TX (Driver I2C):** `[COMPLETAR_CON_TU_MEDICION] µs`
* **Función de Interfaz RX (Driver I2C):** `[COMPLETAR_CON_TU_MEDICION] µs`

**Conclusión de la medición:**
Se observa que el método de *polling* retiene a la CPU durante el tiempo de transmisión/recepción de los bytes en el bus I2C. [Agregar comentario sobre si el tiempo medido era el esperado según la velocidad del bus I2C configurada].