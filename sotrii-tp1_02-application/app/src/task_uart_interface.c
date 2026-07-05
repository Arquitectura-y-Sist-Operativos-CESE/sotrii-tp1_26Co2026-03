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
static task_uart_dta_t task_uart_dta;

/********************** internal functions declaration ***********************/
static task_uart_status_t task_uart_hal_to_status(HAL_StatusTypeDef hal_status);
static void task_uart_spooler_reset(task_uart_spooler_t *spooler);
static uint16_t task_uart_spooler_free_space(task_uart_spooler_t *spooler);
static task_uart_status_t task_uart_tx_spooler_write(task_uart_dta_t *p_task_uart_dta, uint8_t *data, uint16_t size);
static uint16_t task_uart_rx_spooler_read(task_uart_dta_t *p_task_uart_dta, uint8_t *data, uint16_t size);
static task_uart_status_t task_uart_rx_spooler_write_from_isr(task_uart_spooler_t *spooler, uint8_t data);

/********************** internal data definition *****************************/

/********************** external data declaration ****************************/

/********************** external functions definition ************************/
/* Interface functions */
void open_uart(UART_HandleTypeDef *h_uart_device)
{
	BaseType_t ret;
	task_uart_dta_t *p_task_uart_dta = &task_uart_dta;

	p_task_uart_dta->device_id = h_uart_device;
	p_task_uart_dta->tx_status = TASK_UART_STATUS_OK;
	p_task_uart_dta->rx_status = TASK_UART_STATUS_OK;
	p_task_uart_dta->tx_spooler.buffer = (uint8_t *)pvPortMalloc(TASK_UART_TX_SPOOLER_LENGTH);
	configASSERT(NULL != p_task_uart_dta->tx_spooler.buffer);
	p_task_uart_dta->tx_spooler.length = TASK_UART_TX_SPOOLER_LENGTH;
	task_uart_spooler_reset(&p_task_uart_dta->tx_spooler);

	p_task_uart_dta->rx_spooler.buffer = (uint8_t *)pvPortMalloc(TASK_UART_RX_SPOOLER_LENGTH);
	configASSERT(NULL != p_task_uart_dta->rx_spooler.buffer);
	p_task_uart_dta->rx_spooler.length = TASK_UART_RX_SPOOLER_LENGTH;
	task_uart_spooler_reset(&p_task_uart_dta->rx_spooler);

    /* Before a queue or semaphore is used it must be explicitly created.
	 * Check the queue or semaphore was created successfully.
     * Add queue or semaphore to registry. */
	p_task_uart_dta->queue_tx = xQueueCreate(TASK_UART_TX_QUEUE_LENGTH, sizeof(task_uart_tx_dta_t));
	configASSERT(NULL != p_task_uart_dta->queue_tx);
	vQueueAddToRegistry(p_task_uart_dta->queue_tx, "Task UART Tx Queue Handle");

	p_task_uart_dta->queue_rx = xQueueCreate(TASK_UART_RX_QUEUE_LENGTH, sizeof(task_uart_rx_dta_t));
	configASSERT(NULL != p_task_uart_dta->queue_rx);
	vQueueAddToRegistry(p_task_uart_dta->queue_rx, "Task UART Rx Queue Handle");

	p_task_uart_dta->mutex_tx_spooler = xSemaphoreCreateMutex();
	configASSERT(NULL != p_task_uart_dta->mutex_tx_spooler);
	vQueueAddToRegistry(p_task_uart_dta->mutex_tx_spooler, "Task UART Tx Spooler Mutex");

	p_task_uart_dta->sem_tx_done = xSemaphoreCreateBinary();
	configASSERT(NULL != p_task_uart_dta->sem_tx_done);
	vQueueAddToRegistry(p_task_uart_dta->sem_tx_done, "Task UART Tx Done Semaphore");

    /* Before a task is executed it must be explicitly created.
	 * Check the task was created successfully. */
	ret = xTaskCreate(task_uart_tx, "Task UART Tx", (configMINIMAL_STACK_SIZE),
					  (void *)p_task_uart_dta,
					  (tskIDLE_PRIORITY + 1ul), &p_task_uart_dta->task_tx);
	configASSERT(pdPASS == ret);

	ret = xTaskCreate(task_uart_rx, "Task UART Rx", (configMINIMAL_STACK_SIZE),
					  (void *)p_task_uart_dta,
					  (tskIDLE_PRIORITY + 1ul), &p_task_uart_dta->task_rx);
	configASSERT(pdPASS == ret);

	/* Start the first interrupt-driven reception. Each RX callback rearms it. */
	p_task_uart_dta->rx_status = task_uart_hal_to_status(HAL_UART_Receive_IT(p_task_uart_dta->device_id,
																			 &p_task_uart_dta->rx_byte,
																			 sizeof(p_task_uart_dta->rx_byte)));
	if (TASK_UART_STATUS_OK != p_task_uart_dta->rx_status)
	{
		LOGGER_INFO("   ==> open_uart - RX start error status: %d", (int)p_task_uart_dta->rx_status);
	}
}

void release_uart(UART_HandleTypeDef *h_uart_device)
{
	task_uart_dta_t *p_task_uart_dta = &task_uart_dta;

	// Check which version of the uart triggered this function
	if (p_task_uart_dta->device_id == h_uart_device)
	{
		HAL_UART_Abort_IT(p_task_uart_dta->device_id);

		vQueueUnregisterQueue(p_task_uart_dta->queue_tx);
		vQueueDelete(p_task_uart_dta->queue_tx);
		vQueueUnregisterQueue(p_task_uart_dta->queue_rx);
		vQueueDelete(p_task_uart_dta->queue_rx);
		vQueueUnregisterQueue(p_task_uart_dta->mutex_tx_spooler);
		vSemaphoreDelete(p_task_uart_dta->mutex_tx_spooler);
		vQueueUnregisterQueue(p_task_uart_dta->sem_tx_done);
		vSemaphoreDelete(p_task_uart_dta->sem_tx_done);

		vPortFree(p_task_uart_dta->tx_spooler.buffer);
		p_task_uart_dta->tx_spooler.buffer = NULL;
		vPortFree(p_task_uart_dta->rx_spooler.buffer);
		p_task_uart_dta->rx_spooler.buffer = NULL;

		vTaskDelete(p_task_uart_dta->task_tx);
		vTaskDelete(p_task_uart_dta->task_rx);
	}
}

task_uart_status_t write_uart(UART_HandleTypeDef *h_uart_device, uint8_t *data, uint16_t size)
{
	task_uart_dta_t *p_task_uart_dta = &task_uart_dta;
	task_uart_tx_dta_t task_uart_tx_dta = { TASK_UART_EVENT_DATA_READY };
	task_uart_status_t status = TASK_UART_STATUS_ERROR;

	// Check which version of the uart triggered this function
	if ((p_task_uart_dta->device_id == h_uart_device) && (NULL != data) && (0u != size))
	{
		/* Output Data Spooler: write_uart() stores the bytes in the driver
		 * circular buffer and only uses queue_tx to wake the TX gatekeeper. */
		status = task_uart_tx_spooler_write(p_task_uart_dta, data, size);
		if (TASK_UART_STATUS_OK == status)
		{
			/* If queue_tx is already full there is still a pending wake-up for
			 * the gatekeeper, so the data stored in the spooler remains valid. */
			(void)xQueueSend(p_task_uart_dta->queue_tx, &task_uart_tx_dta, 0u);
		}
	}

	if (TASK_UART_STATUS_OK != status)
	{
		LOGGER_INFO("   ==> write_uart - error status: %d", (int)status);
	}

	return status;
}

task_uart_status_t read_uart(UART_HandleTypeDef *h_uart_device, uint8_t *data, uint16_t size, uint16_t *read_size)
{
	task_uart_dta_t *p_task_uart_dta = &task_uart_dta;
	uint16_t i = 0u;
	task_uart_status_t status = TASK_UART_STATUS_ERROR;

	// Check which version of the uart triggered this function
	if ((p_task_uart_dta->device_id == h_uart_device) && (NULL != data) && (NULL != read_size) && (0u != size))
	{
		/* Input Data Spooler: read_uart() drains the circular RX buffer and
		 * returns immediately if no received bytes are pending. */
		i = task_uart_rx_spooler_read(p_task_uart_dta, data, size);
		*read_size = i;
		status = (0u == i) ? TASK_UART_STATUS_EMPTY : TASK_UART_STATUS_OK;
	}

	return status;
}

void ioctl_uart(UART_HandleTypeDef *h_uart_device)
{
	/* Prevent unused argument(s) compilation warning */
	UNUSED(h_uart_device);
}

void uart_tx_cplt_callback(UART_HandleTypeDef *h_uart_device)
{
	task_uart_dta_t *p_task_uart_dta = &task_uart_dta;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	// Check which version of the uart triggered this callback
	if (p_task_uart_dta->device_id == h_uart_device)
	{
		p_task_uart_dta->tx_status = TASK_UART_STATUS_OK;
		xSemaphoreGiveFromISR(p_task_uart_dta->sem_tx_done, &xHigherPriorityTaskWoken);
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
}

void uart_rx_cplt_callback(UART_HandleTypeDef *h_uart_device)
{
	task_uart_dta_t *p_task_uart_dta = &task_uart_dta;
	task_uart_rx_dta_t task_uart_rx_dta = { TASK_UART_EVENT_DATA_READY };
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	// Check which version of the uart triggered this callback
	if (p_task_uart_dta->device_id == h_uart_device)
	{
		if (TASK_UART_STATUS_OK != task_uart_rx_spooler_write_from_isr(&p_task_uart_dta->rx_spooler,
																	   p_task_uart_dta->rx_byte))
		{
			p_task_uart_dta->rx_status = TASK_UART_STATUS_FULL;
			(void)xQueueSendFromISR(p_task_uart_dta->queue_rx, &task_uart_rx_dta, &xHigherPriorityTaskWoken);
		}
		else
		{
			p_task_uart_dta->rx_status = TASK_UART_STATUS_OK;
			(void)xQueueSendFromISR(p_task_uart_dta->queue_rx, &task_uart_rx_dta, &xHigherPriorityTaskWoken);
		}

		/* Rearm one-byte reception so the next hardware event is captured. */
		if (HAL_OK != HAL_UART_Receive_IT(p_task_uart_dta->device_id,
										  &p_task_uart_dta->rx_byte,
										  sizeof(p_task_uart_dta->rx_byte)))
		{
			p_task_uart_dta->rx_status = TASK_UART_STATUS_ERROR;
		}

		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
}

/********************** internal functions definition ************************/
static void task_uart_spooler_reset(task_uart_spooler_t *spooler)
{
	spooler->head = 0u;
	spooler->tail = 0u;
	spooler->count = 0u;
}

static uint16_t task_uart_spooler_free_space(task_uart_spooler_t *spooler)
{
	return (uint16_t)(spooler->length - spooler->count);
}

static task_uart_status_t task_uart_tx_spooler_write(task_uart_dta_t *p_task_uart_dta, uint8_t *data, uint16_t size)
{
	uint16_t i;
	task_uart_status_t status = TASK_UART_STATUS_BUSY;
	task_uart_spooler_t *spooler = &p_task_uart_dta->tx_spooler;

	if (pdTRUE == xSemaphoreTake(p_task_uart_dta->mutex_tx_spooler, 0u))
	{
		status = TASK_UART_STATUS_FULL;
		if (size <= task_uart_spooler_free_space(spooler))
		{
			for (i = 0u; i < size; i++)
			{
				spooler->buffer[spooler->head] = data[i];
				spooler->head = (uint16_t)((spooler->head + 1u) % spooler->length);
				spooler->count++;
			}

			status = TASK_UART_STATUS_OK;
		}

		xSemaphoreGive(p_task_uart_dta->mutex_tx_spooler);
	}

	return status;
}

static uint16_t task_uart_rx_spooler_read(task_uart_dta_t *p_task_uart_dta, uint8_t *data, uint16_t size)
{
	uint16_t i = 0u;
	task_uart_spooler_t *spooler = &p_task_uart_dta->rx_spooler;

	taskENTER_CRITICAL();
	while ((i < size) && (0u != spooler->count))
	{
		data[i] = spooler->buffer[spooler->tail];
		spooler->tail = (uint16_t)((spooler->tail + 1u) % spooler->length);
		spooler->count--;
		i++;
	}
	taskEXIT_CRITICAL();

	return i;
}

static task_uart_status_t task_uart_rx_spooler_write_from_isr(task_uart_spooler_t *spooler, uint8_t data)
{
	task_uart_status_t status = TASK_UART_STATUS_FULL;

	if (spooler->count < spooler->length)
	{
		spooler->buffer[spooler->head] = data;
		spooler->head = (uint16_t)((spooler->head + 1u) % spooler->length);
		spooler->count++;
		status = TASK_UART_STATUS_OK;
	}

	return status;
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

/********************** end of file ******************************************/
