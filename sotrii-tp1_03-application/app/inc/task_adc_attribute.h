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
#include "task.h"
#include "queue.h"
/********************** macros and defines ***********************************************/
/* Definiciones para el Spooler (DMA Buffer) y Colas */
#define ADC_DMA_BUFFER_SIZE  16 // Tamaño del buffer DMA (Input Spooler)
#define ADC_QUEUE_LENGTH     1  // Latest Input Only requiere cola de tamaño 1
#define ADC_ITEM_SIZE        sizeof(uint32_t)

/********************** typedef **********************************************/
/* Structure of Task */


/* Structure of ADC Tx */

/* Estructura del Dispositivo ADC */
typedef struct {
    uint32_t            device_id;
    ADC_HandleTypeDef* h_adc;           // Referencia al hardware
    QueueHandle_t       device_queue;    // Cola para el patrón Latest Input Only
    StaticQueue_t       queue_cb;        // Control Block para asignación estática
    uint8_t             queue_storage[ADC_QUEUE_LENGTH * ADC_ITEM_SIZE]; // Memoria estática

    // Input Data Spooler (Buffer circular manejado por el DMA)
    uint32_t            dma_buffer[ADC_DMA_BUFFER_SIZE];
    volatile bool       is_initialized;  /* Control de errores */
} adc_device_t;

/********************** external data declaration ****************************/
/* Declaración externa de la instancia del dispositivo */
extern adc_device_t g_adc_device_1;
extern TaskHandle_t h_task_adc; /* Handle de la tarea Gatekeeper */

/********************** external functions declaration ***********************/

/********************** End of CPP guard *************************************/
#ifdef __cplusplus
}
#endif

#endif /* TASK_ADC_ATTRIBUTE_H_ */

/********************** end of file ******************************************/
