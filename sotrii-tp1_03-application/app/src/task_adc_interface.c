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
#include "task_adc.h"
#include "task_adc_attribute.h"
#include "task_adc_interface.h"

/********************** macros and definitions *******************************/

/********************** internal data declaration ****************************/

/********************** internal data declaration ****************************/
static task_adc_dta_t task_adc_dta;

/********************** internal functions declaration ***********************/

/********************** internal data definition *****************************/

/********************** external data declaration ****************************/

/********************** external functions definition ************************/
/* Interface functions */
void open_adc(ADC_HandleTypeDef *h_adc_device)
{
	task_adc_dta_t *p_task_adc_dta = &task_adc_dta;

	p_task_adc_dta->device_id = h_adc_device;
	p_task_adc_dta->latest_size = TASK_ADC_CHANNEL_QTY;
	p_task_adc_dta->latest_available = false;
	p_task_adc_dta->rx_status = TASK_ADC_STATUS_OK;

	/* Static allocation: the queue storage and control block live inside the
	 * device data structure. The ISR only posts an event; samples stay in the
	 * driver's static input spooler buffers. */
	p_task_adc_dta->queue_rx = xQueueCreateStatic(TASK_ADC_RX_QUEUE_LENGTH,
												  sizeof(task_adc_rx_dta_t),
												  p_task_adc_dta->queue_rx_storage,
												  &p_task_adc_dta->queue_rx_cb);
	configASSERT(NULL != p_task_adc_dta->queue_rx);
	vQueueAddToRegistry(p_task_adc_dta->queue_rx, "Task ADC Rx Queue Handle");

	p_task_adc_dta->task_rx = xTaskCreateStatic(task_adc_rx,
												"Task ADC Rx",
												TASK_ADC_RX_STACK_WORDS,
												(void *)p_task_adc_dta,
												(tskIDLE_PRIORITY + 1ul),
												p_task_adc_dta->task_rx_stack,
												&p_task_adc_dta->task_rx_tcb);
	configASSERT(NULL != p_task_adc_dta->task_rx);
}

void release_adc(ADC_HandleTypeDef *h_adc_device)
{
	task_adc_dta_t *p_task_adc_dta = &task_adc_dta;

	// Check which version of the adc triggered this function
	if (p_task_adc_dta->device_id == h_adc_device)
	{
		(void)HAL_ADC_Stop_DMA(p_task_adc_dta->device_id);

		vQueueUnregisterQueue(p_task_adc_dta->queue_rx);
		vQueueDelete(p_task_adc_dta->queue_rx);

		vTaskDelete(p_task_adc_dta->task_rx);
	}
}

task_adc_status_t write_adc(ADC_HandleTypeDef *h_adc_device)
{
	/* Prevent unused argument(s) compilation warning */
	UNUSED(h_adc_device);

	return TASK_ADC_STATUS_ERROR;
}

task_adc_status_t read_adc(ADC_HandleTypeDef *h_adc_device, uint16_t *data, uint16_t size, uint16_t *read_size)
{
	task_adc_dta_t *p_task_adc_dta = &task_adc_dta;
	uint16_t i;
	task_adc_status_t status = TASK_ADC_STATUS_ERROR;

	// Check which version of the adc triggered this function
	if ((p_task_adc_dta->device_id == h_adc_device) && (NULL != data) && (NULL != read_size) && (0u != size))
	{
		taskENTER_CRITICAL();
		if (true == p_task_adc_dta->latest_available)
		{
			*read_size = (size < p_task_adc_dta->latest_size) ? size : p_task_adc_dta->latest_size;
			for (i = 0u; i < *read_size; i++)
			{
				data[i] = p_task_adc_dta->latest_buffer[i];
			}
			status = TASK_ADC_STATUS_OK;
		}
		else
		{
			*read_size = 0u;
			status = TASK_ADC_STATUS_EMPTY;
		}
		taskEXIT_CRITICAL();
	}

	return status;
}

void ioctl_adc(ADC_HandleTypeDef *h_adc_device)
{
	/* Prevent unused argument(s) compilation warning */
	UNUSED(h_adc_device);
}

void adc_cplt_callback(ADC_HandleTypeDef *h_adc_device)
{
	task_adc_dta_t *p_task_adc_dta = &task_adc_dta;
	task_adc_rx_dta_t task_adc_rx_dta = { TASK_ADC_EVENT_CONVERSION_READY };
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	UBaseType_t saved_interrupt_status;
	uint16_t i;

	// Check which version of the adc triggered this callback
	if (p_task_adc_dta->device_id == h_adc_device)
	{
		/* Latest Input Only: the ISR overwrites the shared memory with the
		 * newest DMA block. There is no history queue; old data is replaced. */
		saved_interrupt_status = taskENTER_CRITICAL_FROM_ISR();
		for (i = 0u; i < TASK_ADC_CHANNEL_QTY; i++)
		{
			p_task_adc_dta->latest_buffer[i] = p_task_adc_dta->dma_buffer[i];
		}
		p_task_adc_dta->latest_size = TASK_ADC_CHANNEL_QTY;
		p_task_adc_dta->latest_available = true;
		taskEXIT_CRITICAL_FROM_ISR(saved_interrupt_status);

		/* Queue length is one: overwrite keeps only the newest conversion
		 * event for the gatekeeper. The data itself is already in shared
		 * memory, not stored in the queue. */
		xQueueOverwriteFromISR(p_task_adc_dta->queue_rx, &task_adc_rx_dta, &xHigherPriorityTaskWoken);
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
}

/********************** end of file ******************************************/
