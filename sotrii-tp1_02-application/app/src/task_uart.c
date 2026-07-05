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

/********************** internal data declaration ****************************/

/********************** internal data declaration ****************************/

/********************** internal functions declaration ***********************/
void task_uart_tx(void *parameters);
void task_uart_rx(void *parameters);
static task_uart_status_t task_uart_hal_to_status(HAL_StatusTypeDef hal_status);
static uint16_t task_uart_tx_spooler_read(task_uart_dta_t *p_task_uart_dta, uint8_t *data, uint16_t size);

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
	/*  Declare & Initialize Task Function variables */
	g_task_xxxx_tx_cnt = G_TASK_XXXX_CNT_INI;
	g_task_xxxx_tx_runtime_us = G_TASK_XXXX_RUNTIME_US_INI;

	task_uart_dta_t *p_task_uart_tx_dta = (task_uart_dta_t *)parameters;

	/* Print out: Task Initialized */
	LOGGER_INFO(" ");
	LOGGER_INFO("%s is running - Tick [mS] = %3d", pcTaskGetName(NULL), (int)xTaskGetTickCount());

	/* As per most tasks, this task is implemented in an infinite loop. */
	for (;;)
	{
		/* Update Task Counter */
		g_task_xxxx_tx_cnt++;

		task_uart_tx_dta_t task_uart_tx_dta;
		uint8_t tx_chunk[TASK_UART_TX_CHUNK_LENGTH];
		uint16_t tx_size;

		cycle_counter_reset();

		xQueueReceive(p_task_uart_tx_dta->queue_tx, &task_uart_tx_dta, portMAX_DELAY);
		if (TASK_UART_EVENT_DATA_READY != task_uart_tx_dta.event)
		{
			continue;
		}

		/* TX gatekeeper owns the peripheral. The queue only wakes it up; the
		 * bytes are drained from the Output Data Spooler in small chunks. */
		do
		{
			tx_size = task_uart_tx_spooler_read(p_task_uart_tx_dta, tx_chunk, TASK_UART_TX_CHUNK_LENGTH);
			if (0u != tx_size)
			{
				(void)xSemaphoreTake(p_task_uart_tx_dta->sem_tx_done, 0u);
				p_task_uart_tx_dta->tx_status = task_uart_hal_to_status(HAL_UART_Transmit_IT(p_task_uart_tx_dta->device_id,
																							 tx_chunk,
																							 tx_size));
				if (TASK_UART_STATUS_OK == p_task_uart_tx_dta->tx_status)
				{
					xSemaphoreTake(p_task_uart_tx_dta->sem_tx_done, portMAX_DELAY);
				}
				else
				{
					LOGGER_INFO("   ==> Task UART TX - transmit error status: %d", (int)p_task_uart_tx_dta->tx_status);
				}
			}
		}
		while (0u != tx_size);

		g_task_xxxx_tx_runtime_us = cycle_counter_get_time_us();

    	/* Print out: Wait 250mS */
		// LOGGER_INFO(p_task_uart_tx_wait_250mS);
	}
}

/* Task UART RX thread */
void task_uart_rx(void *parameters)
{
	task_uart_dta_t *p_task_uart_rx_dta = (task_uart_dta_t *)parameters;

	/*  Declare & Initialize Task Function variables */
	g_task_xxxx_rx_cnt = G_TASK_XXXX_CNT_INI;
	g_task_xxxx_rx_runtime_us = G_TASK_XXXX_RUNTIME_US_INI;

	/* Print out: Task Initialized */
	LOGGER_INFO(" ");
	LOGGER_INFO("%s is running - Tick [mS] = %3d", pcTaskGetName(NULL), (int)xTaskGetTickCount());

	/* As per most tasks, this task is implemented in an infinite loop. */
	for (;;)
	{
		/* Update Task Counter */
		g_task_xxxx_rx_cnt++;

		task_uart_rx_dta_t task_uart_rx_dta;

		cycle_counter_reset();

		xQueueReceive(p_task_uart_rx_dta->queue_rx, &task_uart_rx_dta, portMAX_DELAY);
		if (TASK_UART_EVENT_DATA_READY != task_uart_rx_dta.event)
		{
			continue;
		}

		/* RX is interrupt-driven: HAL_UART_RxCpltCallback() stores incoming
		 * bytes in the Input Data Spooler and queue_rx only wakes this
		 * gatekeeper so the driver keeps the asynchronous pattern explicit. */
		if (TASK_UART_STATUS_OK != p_task_uart_rx_dta->rx_status)
		{
			LOGGER_INFO("   ==> Task UART RX - receive error status: %d", (int)p_task_uart_rx_dta->rx_status);
			p_task_uart_rx_dta->rx_status = TASK_UART_STATUS_OK;
		}

		g_task_xxxx_rx_runtime_us = cycle_counter_get_time_us();

    	/* Print out: Wait 250mS */
		// LOGGER_INFO(p_task_uart_rx_wait_250mS);
	}
}

static task_uart_status_t task_uart_hal_to_status(HAL_StatusTypeDef hal_status)
{
	switch (hal_status)
	{
		case HAL_OK:
			return TASK_UART_STATUS_OK;

		case HAL_BUSY:
			return TASK_UART_STATUS_BUSY;

		case HAL_TIMEOUT:
			return TASK_UART_STATUS_TIMEOUT;

		case HAL_ERROR:
		default:
			return TASK_UART_STATUS_ERROR;
	}
}

static uint16_t task_uart_tx_spooler_read(task_uart_dta_t *p_task_uart_dta, uint8_t *data, uint16_t size)
{
	uint16_t i = 0u;
	task_uart_spooler_t *spooler = &p_task_uart_dta->tx_spooler;

	if (pdTRUE == xSemaphoreTake(p_task_uart_dta->mutex_tx_spooler, portMAX_DELAY))
	{
		while ((i < size) && (0u != spooler->count))
		{
			data[i] = spooler->buffer[spooler->tail];
			spooler->tail = (uint16_t)((spooler->tail + 1u) % spooler->length);
			spooler->count--;
			i++;
		}

		xSemaphoreGive(p_task_uart_dta->mutex_tx_spooler);
	}

	return i;
}

/********************** end of file ******************************************/
