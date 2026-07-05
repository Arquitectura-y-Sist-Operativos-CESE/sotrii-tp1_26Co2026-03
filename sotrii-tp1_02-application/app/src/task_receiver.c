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
#include "task_uart_interface.h"

/********************** macros and definitions *******************************/
#define G_TASK_RECEIVER_CNT_INI	0ul

#define TASK_RECEIVER_DEL_ZERO		(pdMS_TO_TICKS(0ul))
#define TASK_RECEIVER_DEL_MAX		(pdMS_TO_TICKS(10ul))

/********************** internal data declaration ****************************/

/********************** internal functions declaration ***********************/

/********************** internal data definition *****************************/
const char *p_task_receiver_wait_250mS		= "   ==> Task RECEIVER - Wait:    10mS";

/********************** external data declaration ****************************/
uint32_t g_task_receiver_cnt;

/********************** external functions definition ************************/
/* Task thread */
void task_receiver(void *parameters)
{
	task_uart_echo_dta_t task_uart_echo_dta;
	char rx_text[17];
	uint16_t rx_size;
	uint16_t i;
	task_uart_status_t status;

	UNUSED(parameters);

	/*  Declare & Initialize Task Function variables */
	g_task_receiver_cnt = G_TASK_RECEIVER_CNT_INI;

	/* Print out: Task Initialized */
	LOGGER_INFO(" ");
	LOGGER_INFO("  %s is running - Tick [mS] = %lu", pcTaskGetName(NULL), xTaskGetTickCount());

	/* As per most tasks, this task is implemented in an infinite loop. */
	for (;;)
    {
		/* Update Task Counter */
		g_task_receiver_cnt++;

		/* Echo demo: drain the UART RX driver and forward received blocks to
		 * task_sender through an application queue. */
		do
		{
			status = read_uart(&huart2, task_uart_echo_dta.data, sizeof(task_uart_echo_dta.data), &rx_size);
			if (TASK_UART_STATUS_OK == status)
			{
				task_uart_echo_dta.size = rx_size;
				//LOGGER_INFO("   ==> Task RECEIVER - UART bytes received: %u", (unsigned int)rx_size);
				for (i = 0u; (i < rx_size) && (i < (sizeof(rx_text) - 1u)); i++)
				{
					rx_text[i] = (char)task_uart_echo_dta.data[i];
				}
				//rx_text[i] = '\0';
				//LOGGER_INFO("   ==> Task RECEIVER - UART data: %s", rx_text);

				if (pdTRUE != xQueueSend(h_queue_uart_echo, &task_uart_echo_dta, 0u))
				{
					LOGGER_INFO("   ==> Task RECEIVER - UART echo queue full");
				}
			}
			else if (TASK_UART_STATUS_EMPTY != status)
			{
				LOGGER_INFO("   ==> Task RECEIVER - UART read error status: %d", (int)status);
			}
		} while (TASK_UART_STATUS_OK == status);

    	/* Print out: Wait 10mS */
		// LOGGER_INFO(p_task_receiver_wait_250mS);
		vTaskDelay(TASK_RECEIVER_DEL_MAX);
	}
}

/********************** end of file ******************************************/
