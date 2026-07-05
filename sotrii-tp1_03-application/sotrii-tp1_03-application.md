# Análisis de Código: Device Driver ADC con FreeRTOS

Este documento describe el funcionamiento y la estructura de los archivos fuente proporcionados, los cuales conforman la base (esqueleto) para la implementación de un Device Driver para un ADC utilizando FreeRTOS sobre un microcontrolador STM32.

---

## 1. `app.c` - Inicialización de la Aplicación
Este archivo actúa como el punto de entrada principal para la lógica de la aplicación a nivel de usuario, antes de que el *scheduler* (planificador) de FreeRTOS tome el control.

* **Inicialización de Variables:** Resetea los contadores globales (ticks, idle, overflow).
* **Creación de Tareas:** Utiliza la API `xTaskCreate` de FreeRTOS para instanciar la tarea `task_receiver`. Se le asigna memoria para el stack y una prioridad base (`tskIDLE_PRIORITY + 1ul`).
* **Inicialización del Driver:** Llama a `open_adc(&hadc1)` para inicializar el dispositivo (paso previo a usar el hardware).
* **Configuración de Interrupciones y Contadores:** Llama a `app_it_init()` para preparar las variables usadas en interrupciones, y a `cycle_counter_init()` para habilitar el conteo de ciclos de reloj (útil para perfilar y medir tiempos de ejecución).

## 2. `app_it.c` - Gestión de Interrupciones (Callbacks)
Aquí se manejan las rutinas asociadas a las interrupciones del hardware (ISRs) delegadas por la capa de abstracción de hardware (HAL) de STM32.

* **`app_it_init()`:** Inicializa las banderas y contadores usados por las interrupciones. Utiliza instrucciones en ensamblador (`__asm("CPSID i")` y `__asm("CPSIE i")`) para deshabilitar y habilitar las interrupciones globalmente, creando una **sección crítica** a nivel de hardware para proteger la inicialización.
* **`HAL_GPIO_EXTI_Callback()`:** Es el callback que se ejecuta cuando ocurre una interrupción externa en un pin GPIO (por ejemplo, al presionar el botón `BTN_A_PIN`). Actualmente está vacío, listo para ser implementado.
* **`HAL_ADC_ConvCpltCallback()`:** Es el callback que se dispara cuando el ADC termina una conversión en modo no bloqueante. Activa una bandera (`hal_xxxx_callback_flag`), incrementa un contador y captura el tiempo exacto usando `cycle_counter_get_time_us()`.

## 3. `freertos.c` - Funciones Hook del Sistema Operativo
Este archivo contiene los *Hooks* (ganchos) de FreeRTOS. Son funciones de *callback* que el RTOS llama automáticamente bajo ciertas condiciones de estado.

* **`vApplicationIdleHook()`:** Se ejecuta repetidamente cuando no hay ninguna tarea de mayor prioridad lista para ejecutarse (estado IDLE). Aquí se incrementa `g_task_idle_cnt`. Es el lugar ideal para enviar el microcontrolador a un modo de bajo consumo (Low Power).
* **`vApplicationTickHook()`:** Se ejecuta en cada *tick* del sistema (interrupción del SysTick timer). Incrementa `g_app_tick_cnt`. Al ser llamada desde una interrupción, su ejecución debe ser extremadamente rápida.
* **`vApplicationStackOverflowHook()`:** Actúa como una medida de seguridad. Si el RTOS detecta que una tarea excedió el tamaño de su stack (desbordamiento), ingresa a esta función. Usa `taskENTER_CRITICAL()` y congela el sistema con un `configASSERT(0)` para facilitar la depuración.

## 4. `task_receiver.c` - Tarea Receptora
Implementa un hilo de FreeRTOS básico cuya función principal será, presumiblemente, recibir los datos procesados por el driver del ADC.

* **Estructura Base:** Posee inicialización local y un **ciclo infinito** (`for(;;)`), que es el patrón de diseño estándar para tareas en FreeRTOS.
* **Bloqueo y Retardos:** Utiliza `vTaskDelay(TASK_RECEIVER_DEL_MAX)` para suspender la tarea por 250 ms. Durante este tiempo, la tarea pasa a estado *Blocked*, permitiendo que otras tareas de menor o igual prioridad, o la tarea Idle, utilicen el procesador.

## 5. `task_adc.c` - Tarea del Driver ADC
Contiene la lógica de ejecución periódica relacionada con el ADC (`task_adc_rx`).

* **Profiling (Medición de rendimiento):** Usa las funciones `cycle_counter_reset()` y `cycle_counter_get_time_us()` para medir exactamente cuántos microsegundos toma ejecutar la lógica central de la tarea.
* **Indicador Visual:** Alterna el estado de un LED (`HAL_GPIO_TogglePin`) en cada iteración para dar un indicio visual de que la tarea está viva (Heartbeat).
* Al igual que la tarea receptora, utiliza `vTaskDelay()` para ceder el control del procesador.

## 6. `task_adc_interface.c` y `task_adc_interface.h` - Interfaz del Driver (API)
Estos archivos definen la interfaz estándar (tipo POSIX) para interactuar con el periférico ADC. Esto oculta la complejidad del hardware a las tareas de aplicación (abstracción).

* **`open_adc()` / `release_adc()`:** Para adquirir, configurar y liberar el periférico.
* **`read_adc()` / `write_adc()`:** Para solicitar datos convertidos del ADC o enviar comandos/configuraciones.
* **`ioctl_adc()`:** (Input/Output Control) Para configuraciones específicas del periférico fuera del flujo normal de lectura/escritura (por ejemplo, cambiar la resolución del ADC o cambiar el canal en tiempo de ejecución).
* *Estado actual:* Las funciones reciben el manejador del hardware (`ADC_HandleTypeDef *`), pero actualmente usan el macro `UNUSED()` para evitar advertencias de compilación. En los siguientes pasos del TP, aquí es donde deberás implementar la integración entre las colas (Queues) de FreeRTOS, los semáforos y las llamadas a la HAL.

---

# Implementación del Device Driver ADC

En esta sección se detallan las decisiones de diseño y las métricas obtenidas al implementar el Device Driver del ADC utilizando FreeRTOS.

## Decisiones de Arquitectura
* **Gestión del Periférico (DMA y Spooling):** Se configuró el ADC en modo continuo usando el DMA (`HAL_ADC_Start_DMA`). El DMA llena un *Input Data Spooler* (un arreglo de tamaño `ADC_DMA_BUFFER_SIZE`) sin intervención de la CPU, reduciendo la carga de interrupciones.
* **Tarea Gatekeeper:** Se creó una tarea dedicada (`task_adc_rx`) que recibe una referencia al canal del MCU. Esta tarea se bloquea esperando una notificación del callback `HAL_ADC_ConvCpltCallback` y es la **única** responsable de actualizar la cola del dispositivo, protegiendo el acceso a los datos concurrentes.
* **Asignación Estática (Static Allocation):** Las colas fueron creadas mediante `xQueueCreateStatic()`. Esto garantiza que la memoria se reserve en tiempo de compilación (BSS/Data), evitando problemas de fragmentación de memoria dinámica (Heap) típicos en sistemas embebidos de alta criticidad.
* **Patrón de Diseño "Latest Input Only":** Se implementó configurando la longitud de la cola (`ADC_QUEUE_LENGTH`) en 1 y utilizando la API `xQueueOverwrite()`. Esto garantiza que la tarea receptora (`task_receiver`) siempre obtenga la lectura más fresca y reciente del ADC, descartando valores viejos (comportamiento ideal para sensores de monitoreo continuo).

## Observaciones y Medición de WCET (Worst-Case Execution Time)

Se utilizó el DWT (Data Watchpoint and Trace) del núcleo ARM Cortex-M4 (mediante `cycle_counter_get_time_us()`) para medir el tiempo de ejecución en el peor de los casos de las funciones de interfaz:

* **`open_adc()` WCET:** `[COMPLETAR_CON_TU_MEDICION] µs`
    * *Observación:* Es la función que más tarda ya que configura las estructuras estáticas del RTOS e inicializa el hardware del DMA y el ADC. Al ejecutarse una sola vez en el startup, su peso no afecta el tiempo real.
* **`read_adc()` WCET:** `[COMPLETAR_CON_TU_MEDICION] µs`
    * *Observación:* Es extremadamente rápida porque simplemente extrae un dato de la cola mediante `xQueueReceive`. Al no bloquearse esperando al hardware (porque de eso se encarga el DMA y el Gatekeeper), su WCET es bajo y determinístico, cumpliendo con los requisitos de RTOS.
* **Comportamiento de la Tarea Gatekeeper:** Al observar la ejecución mediante el depurador, se constata que la tarea permanece en estado *Blocked* (consumiendo 0% de CPU) hasta que el DMA dispara la interrupción de completado.