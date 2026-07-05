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
#define UART_DRIVERS_MAX_INSTANCES   2   /* Soporte para hasta 2 canales simultáneos (ej. UART1 y UART2) */
#define UART_QUEUE_MAX_ITEMS         16  /* Capacidad de mensajes en colas en espera */
/********************** CPP guard ********************************************/
#ifdef __cplusplus
extern "C" {
#endif

/********************** inclusions *******************************************/
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/********************** macros ***********************************************/

/********************** typedef **********************************************/
/* Structure of Task */


/* Estructura interna de control del driver */
typedef struct {
    uint32_t            device_id;
    UART_HandleTypeDef* h_uart_device;
    QueueHandle_t       tx_queue;       /* Transportará elementos del tipo task_uart_tx_dta_t */
    QueueHandle_t       rx_queue;       /* Transportará elementos del tipo task_uart_rx_dta_t */
    TaskHandle_t        tx_task_handle; /* Gatekeeper TX */
    TaskHandle_t        rx_task_handle; /* Gatekeeper RX */
    bool                is_active;      /* Flag de instancia inicializada */
} uart_driver_t;

/* Estructura para el Spooler de Transmisión Dinámica */
typedef struct
{
    uint8_t* p_buffer;  /* Puntero al bloque creado dinámicamente con pvPortMalloc */
    uint16_t size;      /* Cantidad de bytes reales del bloque */
} task_uart_tx_dta_t;

/* Estructura para el Spooler de Recepción Dinámica */
typedef struct
{
    uint8_t* p_buffer;  /* Puntero al bloque dinámico donde el Gatekeeper alojó la recepción */
    uint16_t size;      /* Cantidad de bytes recibidos */
} task_uart_rx_dta_t;


/********************** external data declaration ****************************/

/********************** external functions declaration ***********************/

/********************** End of CPP guard *************************************/
#ifdef __cplusplus
}
#endif

#endif /* TASK_UART_ATTRIBUTE_H_ */

/********************** end of file ******************************************/
