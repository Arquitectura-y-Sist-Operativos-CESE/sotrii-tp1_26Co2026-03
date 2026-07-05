/*
 * Copyright (c) 2026 Juan Manuel Cruz <jcruz@fi.uba.ar> <jcruz@frba.utn.edu.ar>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * @author : Juan Manuel Cruz <jcruz@fi.uba.ar> <jcruz@frba.utn.edu.ar>
 */

/********************** inclusions *******************************************/
/* Project includes */
#include "main.h"
#include "cmsis_os.h"

/* Demo includes */
#include "logger.h"
#include "dwt.h"

/* Application & Tasks includes */
#include "board.h"
#include "app.h"
#include "app_it.h"
#include "task_uart.h"
#include "task_uart_attribute.h"
#include "task_uart_interface.h"

/********************** macros and definitions *******************************/

/********************** internal data declaration ****************************/

/********************** internal data declaration ****************************/
 uart_driver_t g_drivers[UART_DRIVERS_MAX_INSTANCES] = {0};

/********************** internal functions declaration ***********************/

/********************** internal data definition *****************************/

/********************** external data declaration ****************************/

/********************** external functions definition ************************/

/* Función auxiliar interna para asociar el handle de hardware de la HAL con nuestro Driver de FreeRTOS */
static uart_driver_t* get_driver_instance(UART_HandleTypeDef *h_uart_device)
{
    if (h_uart_device == NULL) return NULL;

    for (int i = 0; i < UART_DRIVERS_MAX_INSTANCES; i++) {
        if (g_drivers[i].is_active && g_drivers[i].h_uart_device == h_uart_device) {
            return &g_drivers[i];
        }
    }
    return NULL; /* Instancia no encontrada o no abierta */
}

/* Interface functions */
void open_uart(UART_HandleTypeDef *h_uart_device)
{
	configASSERT(h_uart_device != NULL);

	/* Buscar una ranura libre en nuestro arreglo de instancias */
	uart_driver_t *p_driver = NULL;
	for (int i = 0; i < UART_DRIVERS_MAX_INSTANCES; i++) {
		if (!g_drivers[i].is_active) {
	}
		p_driver = &g_drivers[i];
	    p_driver->device_id = i + 1;
	    break;
	        }
	/* Si no hay lugares libres, detenemos el sistema en modo Debug */
	    configASSERT(p_driver != NULL);

	    p_driver->h_uart_device = h_uart_device;

	    /* Creación de colas que transportarán ESTRUCTURAS DE PUNTEROS (Spooler por bloques) */
	    p_driver->tx_queue = xQueueCreate(UART_QUEUE_MAX_ITEMS, sizeof(task_uart_tx_dta_t));
	    configASSERT(p_driver->tx_queue != NULL);

	    p_driver->rx_queue = xQueueCreate(UART_QUEUE_MAX_ITEMS, sizeof(task_uart_rx_dta_t));
	    configASSERT(p_driver->rx_queue != NULL);

	    /* Creación dinámica de las tareas Gatekeeper pasando el puntero al objeto driver específico */
	    BaseType_t ret;
	    ret = xTaskCreate(task_uart_tx, "UART_TX_GK", configMINIMAL_STACK_SIZE * 2, (void*)p_driver, tskIDLE_PRIORITY + 2, &p_driver->tx_task_handle);
	    configASSERT(ret == pdPASS);

	    ret = xTaskCreate(task_uart_rx, "UART_RX_GK", configMINIMAL_STACK_SIZE * 2, (void*)p_driver, tskIDLE_PRIORITY + 2, &p_driver->rx_task_handle);
	    configASSERT(ret == pdPASS);

	    p_driver->is_active = true;
}

void release_uart(UART_HandleTypeDef *h_uart_device)
{
    uart_driver_t *p_driver = get_driver_instance(h_uart_device);
    if (p_driver == NULL) return;

    vTaskDelete(p_driver->tx_task_handle);
    vTaskDelete(p_driver->rx_task_handle);
    vQueueDelete(p_driver->tx_queue);
    vQueueDelete(p_driver->rx_queue);
    p_driver->is_active = false;
}

void write_uart(UART_HandleTypeDef *h_uart_device, const uint8_t *p_data, uint16_t size)
{
    /* Programación defensiva inicial */
    if (h_uart_device == NULL || p_data == NULL || size == 0) return;

    uart_driver_t *p_driver = get_driver_instance(h_uart_device);
    if (p_driver == NULL) return; /* Driver no inicializado para este periférico */

    /* 1. ASIGNACIÓN DINÁMICA: Reservamos memoria en el Heap de FreeRTOS para el bloque */
    uint8_t *p_dynamic_buffer = pvPortMalloc(size);

    /* Validamos si nos quedamos sin memoria RAM */
    if (p_dynamic_buffer == NULL) {
        return; /* Abortamos si el heap está lleno */
    }

    /* 2. Copiamos los datos del usuario hacia nuestro buffer dinámico seguro */
    memcpy(p_dynamic_buffer, p_data, size);

    /* 3. Empaquetamos el puntero y el tamaño en la estructura de transferencia */
    task_uart_tx_dta_t tx_msg = {
        .p_buffer = p_dynamic_buffer,
        .size = size
    };

    /* 4. Enviamos la estructura a la cola. Esperamos máximo 20ms si está saturada */
    if (xQueueSend(p_driver->tx_queue, &tx_msg, pdMS_TO_TICKS(20)) != pdPASS) {
        /* Si la cola rechazó el mensaje, debemos liberar la memoria
           inmediatamente para prevenir un Memory Leak (fuga de memoria) */
        vPortFree(p_dynamic_buffer);
    }
}

void read_uart(UART_HandleTypeDef *h_uart_device, uint8_t *p_data, uint16_t size)
{
    if (h_uart_device == NULL || p_data == NULL || size == 0) return;

    uart_driver_t *p_driver = get_driver_instance(h_uart_device);
    if (p_driver == NULL) return;

    task_uart_rx_dta_t rx_msg;
    uint16_t bytes_copied = 0;

    /* Consumimos bloques de la cola de recepción hasta llenar el pedido del usuario */
    while (bytes_copied < size) {
        if (xQueueReceive(p_driver->rx_queue, &rx_msg, portMAX_DELAY) == pdPASS) {

            uint16_t remaining_space = size - bytes_copied;
            uint16_t to_copy = (rx_msg.size < remaining_space) ? rx_msg.size : remaining_space;

            memcpy(&p_data[bytes_copied], rx_msg.p_buffer, to_copy);
            bytes_copied += to_copy;

            /* La memoria dinámica alocada por el Gatekeeper RX es liberada
               aquí tras haber extraído/copiado los datos de manera segura */
            vPortFree(rx_msg.p_buffer);
        }
    }
}

void ioctl_uart(UART_HandleTypeDef *h_uart_device)
{
	/* Prevent unused argument(s) compilation warning */
	UNUSED(h_uart_device);
}

/********************** end of file ******************************************/
