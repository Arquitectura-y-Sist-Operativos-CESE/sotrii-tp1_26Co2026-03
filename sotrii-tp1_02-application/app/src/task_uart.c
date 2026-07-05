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
#include "task_uart_attribute.h"


/********************** macros and definitions *******************************/
#define G_TASK_XXXX_CNT_INI	0ul
#define G_TASK_XXXX_RUNTIME_US_INI	0ul

#define TASK_XXXX_DEL_ZERO	(pdMS_TO_TICKS(0ul))
#define TASK_XXXX_DEL_MAX	(pdMS_TO_TICKS(250ul))

#define RX_BLOCK_SIZE   8   /* Tamaño fijo del buffer dinámico de recepción por bloques */

/********************** internal data declaration ****************************/

/********************** internal data declaration ****************************/

/********************** internal functions declaration ***********************/
void task_uart_tx(void *parameters);
void task_uart_rx(void *parameters);

/********************** internal data definition *****************************/
const char *p_task_uart_tx_wait_250mS	= "   ==> Task UART TX - Wait:   250mS";
const char *p_task_uart_rx_wait_250mS	= "   ==> Task UART RX - Wait:   250mS";

/********************** external data declaration ****************************/
uint32_t g_task_xxxx_tx_cnt;
uint32_t g_task_xxxx_tx_runtime_us;

uint32_t g_task_xxxx_rx_cnt;
uint32_t g_task_xxxx_rx_runtime_us;

/********************** external functions definition ************************/
/* Task UART TX thread */
void task_uart_tx(void *parameters)
{
	uart_driver_t *p_driver = (uart_driver_t *)parameters;
	configASSERT(p_driver != NULL);

	task_uart_tx_dta_t tx_msg;
	HAL_StatusTypeDef hal_status;

	/*  Declare & Initialize Task Function variables */
	g_task_xxxx_tx_cnt = G_TASK_XXXX_CNT_INI;
	g_task_xxxx_tx_runtime_us = G_TASK_XXXX_RUNTIME_US_INI;

	/* Print out: Task Initialized */
	LOGGER_INFO(" ");
	LOGGER_INFO("%s is running - Tick [mS] = %3d", pcTaskGetName(NULL), (int)xTaskGetTickCount());

	/* As per most tasks, this task is implemented in an infinite loop. */
	for (;;)
	{
		/* Update Task Counter */
		g_task_xxxx_tx_cnt++;

		cycle_counter_reset();

		HAL_GPIO_TogglePin(LED_A_PORT, LED_A_PIN);

		g_task_xxxx_tx_runtime_us = cycle_counter_get_time_us();
		/* 1. Espera pasiva en la cola hasta que se solicite una transmisión por bloques */
		if (xQueueReceive(p_driver->tx_queue, &tx_msg, portMAX_DELAY) == pdPASS) {
			configASSERT(tx_msg.p_buffer != NULL);

			/* 2. Disparamos la transmisión asincrónica por Interrupción de la HAL */
			hal_status = HAL_UART_Transmit_IT(p_driver->h_uart_device,
					tx_msg.p_buffer, tx_msg.size);

			/* 3. MANEJO DE ERRORES DE LA HAL */
			if (hal_status == HAL_OK) {
				/* Bloqueo eficiente de la tarea: la ISR nos despertará al terminar */
				ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

				/* Transmisión completada con éxito -> Liberamos el bloque dinámico */
				vPortFree(tx_msg.p_buffer);
			} else if (hal_status == HAL_BUSY) {
				/* El hardware está temporalmente ocupado: Devolvemos el puntero al frente de
				 la cola para no alterar el orden físico y reintentamos brevemente */
				xQueueSendToFront(p_driver->tx_queue, &tx_msg, 0);
				vTaskDelay(pdMS_TO_TICKS(2));
			} else {
				/* HAL_ERROR: Problema severo de hardware. Abortamos y limpiamos el buffer para evitar fugas */
				HAL_UART_AbortTransmit_IT(p_driver->h_uart_device);
				vPortFree(tx_msg.p_buffer);
				vTaskDelay(pdMS_TO_TICKS(10));
			}
		}

	}
}

/* Task UART RX thread */
void task_uart_rx(void *parameters)
{
	uart_driver_t *p_driver = (uart_driver_t *)parameters;
	configASSERT(p_driver != NULL);

	HAL_StatusTypeDef hal_status;

	/*  Declare & Initialize Task Function variables */
	g_task_xxxx_rx_cnt = G_TASK_XXXX_CNT_INI;
	g_task_xxxx_rx_runtime_us = G_TASK_XXXX_RUNTIME_US_INI;

	/* Print out: Task Initialized */
	LOGGER_INFO(" ");
	LOGGER_INFO("%s is running - Tick [mS] = %3d", pcTaskGetName(NULL), (int)xTaskGetTickCount());

	/* As per most tasks, this task is implemented in an infinite loop. */
	for (;;) {
		/* Update Task Counter */
		g_task_xxxx_rx_cnt++;

		cycle_counter_reset();

		HAL_GPIO_TogglePin(LED_A_PORT, LED_A_PIN);

		g_task_xxxx_rx_runtime_us = cycle_counter_get_time_us();

		/* 1. ASIGNACIÓN DINÁMICA: Reservamos espacio para alojar el próximo bloque a recibir */
		uint8_t *p_rx_buffer = pvPortMalloc(RX_BLOCK_SIZE);
		if (p_rx_buffer == NULL) {
			vTaskDelay(pdMS_TO_TICKS(5)); /* Breve espera si no hay RAM disponible */
			continue;
		}

		/* 2. Armamos la recepción asincrónica apuntando al bloque dinámico */
		hal_status = HAL_UART_Receive_IT(p_driver->h_uart_device, p_rx_buffer,
				RX_BLOCK_SIZE);

		if (hal_status == HAL_OK) {
			/* 3. Dormimos de forma pasiva hasta que la interrupción nos avise que el bloque está lleno */
			ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

			/* 4. Preparamos el mensaje para el spooler */
			task_uart_rx_dta_t rx_msg = { .p_buffer = p_rx_buffer, .size =
					RX_BLOCK_SIZE };

			/* 5. Colocamos el puntero en el spooler de entrada */
			if (xQueueSend(p_driver->rx_queue, &rx_msg, 0) != pdPASS) {
				/* Si la aplicación no está consumiendo datos y la cola se llena,
				 debemos destruir el buffer para evitar el memory leak */
				vPortFree(p_rx_buffer);
			}
		} else {
			/* Error al armar el periférico (Overrun / Frame Error). Limpiamos y reintentamos */
			HAL_UART_AbortReceive_IT(p_driver->h_uart_device);
			vPortFree(p_rx_buffer);
			vTaskDelay(pdMS_TO_TICKS(5));
		}
	}
}

/********************** end of file ******************************************/
