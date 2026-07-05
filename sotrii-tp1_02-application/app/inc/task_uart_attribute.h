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

#ifndef TASK_UART_ATTRIBUTE_H_
#define TASK_UART_ATTRIBUTE_H_

/********************** CPP guard ********************************************/
#ifdef __cplusplus
extern "C" {
#endif

/********************** inclusions *******************************************/
#include "cmsis_os.h"

/********************** macros ***********************************************/
#define TASK_UART_TX_QUEUE_LENGTH	5
#define TASK_UART_RX_QUEUE_LENGTH	32
#define TASK_UART_TX_SPOOLER_LENGTH	256
#define TASK_UART_RX_SPOOLER_LENGTH	256
#define TASK_UART_TX_CHUNK_LENGTH	32

/********************** typedef **********************************************/
typedef enum
{
	TASK_UART_STATUS_OK = 0,
	TASK_UART_STATUS_ERROR,
	TASK_UART_STATUS_BUSY,
	TASK_UART_STATUS_TIMEOUT,
	TASK_UART_STATUS_EMPTY,
	TASK_UART_STATUS_FULL
} task_uart_status_t;

typedef enum
{
	TASK_UART_EVENT_DATA_READY = 0
} task_uart_event_t;

typedef struct
{
	uint8_t *		buffer;
	uint16_t		length;
	uint16_t		head;
	uint16_t		tail;
	uint16_t		count;
} task_uart_spooler_t;

/* Structure of Task */
typedef struct
{
	UART_HandleTypeDef *	device_id;

	TaskHandle_t			task_tx;
	TaskHandle_t			task_rx;

	QueueHandle_t			queue_tx;
	QueueHandle_t			queue_rx;

	SemaphoreHandle_t		mutex_tx_spooler;
	SemaphoreHandle_t		sem_tx_done;
	task_uart_spooler_t		tx_spooler;
	task_uart_status_t		tx_status;

	task_uart_spooler_t		rx_spooler;
	uint8_t					rx_byte;
	task_uart_status_t		rx_status;
} task_uart_dta_t;

/* Structure of UART Tx */
typedef struct
{
	task_uart_event_t	event;
} task_uart_tx_dta_t;

/* Structure of UART Rx */
typedef struct
{
	task_uart_event_t	event;
} task_uart_rx_dta_t;


/********************** external data declaration ****************************/

/********************** external functions declaration ***********************/

/********************** End of CPP guard *************************************/
#ifdef __cplusplus
}
#endif

#endif /* TASK_UART_ATTRIBUTE_H_ */

/********************** end of file ******************************************/
