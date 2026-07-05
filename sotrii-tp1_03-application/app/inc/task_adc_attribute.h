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

#ifndef TASK_ADC_ATTRIBUTE_H_
#define TASK_ADC_ATTRIBUTE_H_

/********************** CPP guard ********************************************/
#ifdef __cplusplus
extern "C" {
#endif

/********************** inclusions *******************************************/
#include "cmsis_os.h"

/********************** macros ***********************************************/
#define TASK_ADC_CHANNEL_QTY			10u
#define TASK_ADC_RX_QUEUE_LENGTH		1u
#define TASK_ADC_RX_STACK_WORDS			(2u * configMINIMAL_STACK_SIZE)

/********************** typedef **********************************************/
typedef enum
{
	TASK_ADC_STATUS_OK = 0,
	TASK_ADC_STATUS_ERROR,
	TASK_ADC_STATUS_BUSY,
	TASK_ADC_STATUS_TIMEOUT,
	TASK_ADC_STATUS_EMPTY,
	TASK_ADC_STATUS_FULL
} task_adc_status_t;

typedef enum
{
	TASK_ADC_EVENT_CONVERSION_READY = 0
} task_adc_event_t;

typedef struct
{
	task_adc_event_t	event;
} task_adc_rx_dta_t;

/* Structure of Task */
typedef struct
{
	ADC_HandleTypeDef *	device_id;

	TaskHandle_t		task_rx;
	StaticTask_t		task_rx_tcb;
	StackType_t			task_rx_stack[TASK_ADC_RX_STACK_WORDS];

	QueueHandle_t		queue_rx;
	StaticQueue_t		queue_rx_cb;
	uint8_t				queue_rx_storage[TASK_ADC_RX_QUEUE_LENGTH * sizeof(task_adc_rx_dta_t)];

	uint16_t			dma_buffer[TASK_ADC_CHANNEL_QTY];
	uint16_t			latest_buffer[TASK_ADC_CHANNEL_QTY];
	uint16_t			latest_size;
	bool				latest_available;

	task_adc_status_t	rx_status;
} task_adc_dta_t;


/* Structure of ADC Tx */


/********************** external data declaration ****************************/

/********************** external functions declaration ***********************/

/********************** End of CPP guard *************************************/
#ifdef __cplusplus
}
#endif

#endif /* TASK_ADC_ATTRIBUTE_H_ */

/********************** end of file ******************************************/
