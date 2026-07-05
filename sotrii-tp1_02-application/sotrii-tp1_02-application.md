# Análisis del Código Fuente: Sistema Basado en FreeRTOS

Este documento presenta un análisis detallado del conjunto de archivos fuente proporcionados, los cuales conforman la estructura base de una aplicación basada en **FreeRTOS** para un microcontrolador (aparentemente de la familia STM32, debido al uso de las librerías HAL). 

El objetivo principal de este código es establecer un sistema disparado por eventos (*Event-Triggered System*) utilizando tareas concurrentes, interrupciones y manejo de periféricos como la UART.

---

## 1. `app.c` (Inicialización y Configuración Principal)
Este archivo actúa como el punto de configuración de la aplicación sobre el Sistema Operativo en Tiempo Real (RTOS).

* **Inicialización global:** Configura contadores globales para los ticks de la aplicación, ciclos de inactividad (*idle*) y desbordamientos de pila, inicializándolos a cero.
* **Creación de tareas:** Instancia y registra en FreeRTOS dos tareas principales: `task_sender` y `task_receiver`. Ambas son creadas con la misma prioridad (`tskIDLE_PRIORITY + 1`) y un tamaño de pila de `2 * configMINIMAL_STACK_SIZE`.
* **Inicialización de hardware y periféricos:** Llama a funciones para inicializar el driver de la UART (`open_uart`), las interrupciones de la aplicación (`app_it_init`) y el contador de ciclos del procesador (`cycle_counter_init`).

## 2. `app_it.c` (Manejo de Interrupciones - ISR)
Contiene las Rutinas de Servicio de Interrupción (Callbacks de la capa HAL), que responden a eventos de hardware.

* **Inicialización segura (`app_it_init`):** Deshabilita temporalmente las interrupciones globales (`__asm("CPSID i")`) para inicializar variables compartidas de forma segura, volviéndolas a habilitar al terminar.
* **Interrupción de Botón (EXTI):** Define `HAL_GPIO_EXTI_Callback` para detectar pulsaciones en un pin específico (`BTN_A_PIN`). La lógica interna para manejar la pulsación aún no está implementada (está vacía).
* **Interrupción UART Tx:** Define `HAL_UART_TxCpltCallback`, que se ejecuta cuando el hardware de la UART termina de transmitir un dato. Al dispararse, activa una bandera booleana (`hal_xxxx_callback_flag`), incrementa un contador y registra el tiempo de ejecución en microsegundos.

## 3. `freertos.c` (Hooks del Sistema Operativo)
Implementa funciones tipo *Hook* (ganchos) de FreeRTOS. Estas son funciones *callback* llamadas por el sistema operativo en momentos o estados específicos.

* **Idle Hook (`vApplicationIdleHook`):** Se ejecuta cuando el procesador está libre (ninguna tarea de mayor prioridad requiere ejecución). Aquí se incrementa el contador de inactividad `g_task_idle_cnt`.
* **Tick Hook (`vApplicationTickHook`):** Se ejecuta con cada interrupción del "tick" (el reloj base interno del RTOS), incrementando el contador global de ticks del sistema.
* **Stack Overflow Hook (`vApplicationStackOverflowHook`):** Actúa como medida de seguridad. Si el sistema detecta que la pila (*stack*) de una tarea se ha desbordado, el RTOS llama a esta función. El código detiene la ejecución abruptamente (`configASSERT(0)`) dentro de una sección crítica de interrupciones deshabilitadas para facilitar la depuración, e incrementa un contador de errores.

## 4. `task_sender.c` y `task_receiver.c` (Tareas Básicas)
Ambos archivos contienen la lógica individual de las dos tareas instanciadas en `app.c`. Comparten una estructura idéntica enfocada en la ejecución periódica.

* **Ciclo de vida:** Ambas consisten en un bucle infinito (`for(;;)`) típico de las tareas de RTOS.
* **Operaciones:** En cada iteración:
    1. Incrementan un contador local.
    2. Imprimen un mensaje en consola (usando `LOGGER_INFO`) indicando que esperarán 250 milisegundos.
    3. Se bloquean llamando a `vTaskDelay(TASK_SENDER_DEL_MAX)` o `TASK_RECEIVER_DEL_MAX` respectivamente. 
    4. Al bloquearse (entrar en estado *Blocked*), ceden el control del procesador a otras tareas que estén listas para ejecutarse.

## 5. `task_uart.c` (Tareas de Comunicación UART)
Define el código para dos tareas encargadas de la comunicación por puerto serie (`task_uart_tx` y `task_uart_rx`).

* **Estado actual:** A diferencia de `task_sender` y `task_receiver`, estas tareas están definidas en el código pero **no son creadas ni instanciadas** en `app.c`. Por lo tanto, no se ejecutan en la configuración actual.
* **Comportamiento interno:** Al igual que las otras tareas, contienen bucles infinitos. Dentro del bucle, reinician un contador de ciclos, cambian el estado de un LED (`HAL_GPIO_TogglePin`), guardan el tiempo de ejecución actual, emiten un mensaje y entran en un estado de espera (delay) de 250ms.

## 6. `task_uart_interface.c` y `.h` (Capa de Abstracción)
Estos archivos proveen una interfaz estandarizada (API) para interactuar con la UART, separando la lógica de alto nivel de la aplicación de los detalles específicos del hardware (STM32 HAL).

* **Funciones Declaradas:** Define operaciones estándar como `open_uart`, `release_uart`, `write_uart`, `read_uart` y `ioctl_uart`.
* **Implementación Dummy (*Stubs*):** Actualmente, el cuerpo de todas estas funciones está vacío. Solo contienen una macro `UNUSED(h_uart_device)` para evitar advertencias (*warnings*) del compilador respecto a variables que se pasan como argumento pero no se utilizan.

---

## Resumen de la Arquitectura
El código configura un ecosistema multitarea clásico donde FreeRTOS gestiona el tiempo de CPU. 

Actualmente, el sistema alternará su ejecución entre `task_sender` y `task_receiver` de forma concurrente cada 250ms. Utiliza los *Hooks* del sistema operativo (en `freertos.c`) para monitorear la salud del sistema (conteo de ticks, tiempo en reposo y verificación de memoria). 
Por otro lado, la base para la comunicación por hardware (UART) y el manejo de interrupciones externas está preparada (mediante interfaces vacías en `task_uart_interface.c` y rutinas que actualizan banderas en `app_it.c`), dejándola lista para que la lógica de negocio se implemente en un futuro sin alterar la estructura general.

---

# Diseño e implementación de Device Driver UART FreeRTOS con Interrupciones

## Medición del WCET y Comportamiento del Device Driver UART

### Metodología de Medición
Para obtener el tiempo de ejecución en el peor de los casos (WCET) de las funciones de interfaz, se utilizó el DWT (Data Watchpoint and Trace) del microcontrolador ARM Cortex-M, accesible mediante `cycle_counter_get_time_us()`. Se tomaron marcas de tiempo inmediatamente antes y después de invocar a la primitiva `write_uart()` desde la tarea de aplicación (`task_sender`).

### Resultados Observados
Se observó que el tiempo de ejecución de la primitiva de interfaz `write_uart()` es de aproximadamente **[X] us** (microsegundos). 

### Análisis del Comportamiento (Arquitectura Asincrónica)
El tiempo medido es extremadamente corto e independiente del Baud Rate o del tamaño del hardware. Esto demuestra el éxito del **patrón de diseño asincrónico** implementado:

1. **Desacople del Hardware:** La función `write_uart()` no espera a que el periférico STM32 termine de enviar los datos bit a bit. Su WCET está dictado únicamente por el tiempo que le toma al procesador:
   - Alocar memoria en el Heap (`pvPortMalloc`).
   - Copiar los datos al buffer dinámico (`memcpy`).
   - Empaquetar el puntero en una estructura y enviarlo a la cola de FreeRTOS (`xQueueSend`).
2. **Eficiencia por Spooler de Punteros:** Al utilizar un "Queue with Input and Output Data Spooler" con asignación dinámica, evitamos copiar grandes ráfagas de datos a través de la cola byte por byte. La cola solo transporta un puntero de 32-bits y un entero, manteniendo el WCET predecible y bajo.
3. **Manejo de Interrupciones (Gatekeepers):** Mientras el hardware transmite físicamente, la tarea `task_sender` ya está libre para realizar otros cálculos o bloquearse. El Gatekeeper de transmisión (`task_uart_tx`) negocia con la HAL y se bloquea eficientemente (`ulTaskNotifyTake`) esperando ser despertado por la interrupción (ISR) al finalizar la transmisión, logrando un uso de CPU cercano al 0% durante la espera activa.