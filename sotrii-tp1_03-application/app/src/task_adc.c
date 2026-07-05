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
#include "task_adc_attribute.h"

/********************** macros and definitions *******************************/
#define G_TASK_XXXX_CNT_INI			0ul
#define G_TASK_XXXX_RUNTIME_US_INI	0ul

#define TASK_XXXX_DEL_ZERO	(pdMS_TO_TICKS(0ul))
#define TASK_XXXX_DEL_MAX	(pdMS_TO_TICKS(250ul))

/********************** internal data declaration ****************************/

/********************** internal data declaration ****************************/

/********************** internal functions declaration ***********************/
void task_adc_rx(void *parameters);
static task_adc_status_t task_adc_hal_to_status(HAL_StatusTypeDef hal_status);

/********************** internal data definition *****************************/
const char *p_task_adc_rx_wait_250mS	= "   ==> Task ADC RX - Wait:   250mS";

/********************** external data declaration ****************************/
uint32_t g_task_xxxx_tx_cnt;
uint32_t g_task_xxxx_tx_runtime_us;

uint32_t g_task_xxxx_rx_cnt;
uint32_t g_task_xxxx_rx_runtime_us;

/********************** external functions definition ************************/
/* Task ADC RX thread */
void task_adc_rx(void *parameters)
{
	task_adc_dta_t *p_task_adc_rx_dta = (task_adc_dta_t *)parameters;
	task_adc_rx_dta_t task_adc_rx_dta;

	/*  Declare & Initialize Task Function variables */
	g_task_xxxx_rx_cnt = G_TASK_XXXX_CNT_INI;
	g_task_xxxx_rx_runtime_us = G_TASK_XXXX_RUNTIME_US_INI;

	/* Print out: Task Initialized */
	LOGGER_INFO(" ");
	LOGGER_INFO("%s is running - Tick [mS] = %3d", pcTaskGetName(NULL), (int)xTaskGetTickCount());

	/* ADC RX gatekeeper owns DMA starts. The DMA completion callback only
	 * sends an event to queue_rx. */
	p_task_adc_rx_dta->rx_status = task_adc_hal_to_status(HAL_ADC_Start_DMA(p_task_adc_rx_dta->device_id,
																			(uint32_t *)p_task_adc_rx_dta->dma_buffer,
																			TASK_ADC_CHANNEL_QTY));
	if (TASK_ADC_STATUS_OK != p_task_adc_rx_dta->rx_status)
	{
		LOGGER_INFO("   ==> Task ADC RX - DMA start error status: %d", (int)p_task_adc_rx_dta->rx_status);
	}

	/* As per most tasks, this task is implemented in an infinite loop. */
	for (;;)
	{
		/* Update Task Counter */
		g_task_xxxx_rx_cnt++;

		cycle_counter_reset();

		xQueueReceive(p_task_adc_rx_dta->queue_rx, &task_adc_rx_dta, portMAX_DELAY);
		if (TASK_ADC_EVENT_CONVERSION_READY == task_adc_rx_dta.event)
		{
			/* The ISR already updated latest_buffer. The gatekeeper only
			 * restarts the DMA acquisition cycle for the ADC peripheral. */
			p_task_adc_rx_dta->rx_status = task_adc_hal_to_status(HAL_ADC_Start_DMA(p_task_adc_rx_dta->device_id,
																					(uint32_t *)p_task_adc_rx_dta->dma_buffer,
																					TASK_ADC_CHANNEL_QTY));
			if (TASK_ADC_STATUS_OK != p_task_adc_rx_dta->rx_status)
			{
				LOGGER_INFO("   ==> Task ADC RX - DMA restart error status: %d", (int)p_task_adc_rx_dta->rx_status);
			}
		}

		g_task_xxxx_rx_runtime_us = cycle_counter_get_time_us();

    	/* Print out: Wait 250mS */
		// LOGGER_INFO(p_task_adc_rx_wait_250mS);
	}
}

static task_adc_status_t task_adc_hal_to_status(HAL_StatusTypeDef hal_status)
{
	switch (hal_status)
	{
		case HAL_OK:
			return TASK_ADC_STATUS_OK;

		case HAL_BUSY:
			return TASK_ADC_STATUS_BUSY;

		case HAL_TIMEOUT:
			return TASK_ADC_STATUS_TIMEOUT;

		case HAL_ERROR:
		default:
			return TASK_ADC_STATUS_ERROR;
	}
}

/********************** end of file ******************************************/
