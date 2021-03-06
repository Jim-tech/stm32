/**
  ******************************************************************************
  * @file           : usbd_cdc_if.c
  * @brief          :
  ******************************************************************************
  * This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * Copyright (c) 2017 STMicroelectronics International N.V. 
  * All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without 
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice, 
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other 
  *    contributors to this software may be used to endorse or promote products 
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this 
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under 
  *    this license is void and will automatically terminate your rights under 
  *    this license. 
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS" 
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT 
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT 
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
*/

/* Includes ------------------------------------------------------------------*/
#include "usbd_cdc_if.h"
/* USER CODE BEGIN INCLUDE */
/* USER CODE END INCLUDE */

/** @addtogroup STM32_USB_OTG_DEVICE_LIBRARY
  * @{
  */

/** @defgroup USBD_CDC 
  * @brief usbd core module
  * @{
  */ 

/** @defgroup USBD_CDC_Private_TypesDefinitions
  * @{
  */ 
/* USER CODE BEGIN PRIVATE_TYPES */
/* USER CODE END PRIVATE_TYPES */ 
/**
  * @}
  */ 

/** @defgroup USBD_CDC_Private_Defines
  * @{
  */ 
/* USER CODE BEGIN PRIVATE_DEFINES */
/* Define size for the receive and transmit buffer over CDC */
/* It's up to user to redefine and/or remove those define */

/* USER CODE END PRIVATE_DEFINES */
/**
  * @}
  */ 

/** @defgroup USBD_CDC_Private_Macros
  * @{
  */ 
/* USER CODE BEGIN PRIVATE_MACRO */
/* USER CODE END PRIVATE_MACRO */

/**
  * @}
  */ 
  
/** @defgroup USBD_CDC_Private_Variables
  * @{
  */
/* Create buffer for reception and transmission           */
/* It's up to user to redefine and/or remove those define */
/* Received Data over USB are stored in this buffer       */
LINE_CODING LineCoding[VCP_NUM];
/* USER CODE BEGIN PRIVATE_VARIABLES */

/* USER CODE END PRIVATE_VARIABLES */
uint8_t  RxBuffer[VCP_NUM][CDC_DATA_FS_OUT_PACKET_SIZE];
uint8_t  IoBuffer[CDC_CMD_PACKET_SIZE];

USART_Q  TxQueue[VCP_NUM];
uint16_t g_adcdata[ADC_MAX_SAMPLES];

/**
  * @}
  */ 
  
/** @defgroup USBD_CDC_IF_Exported_Variables
  * @{
  */ 
extern USBD_HandleTypeDef hUsbDeviceFS;
/* USER CODE BEGIN EXPORTED_VARIABLES */
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern ADC_HandleTypeDef hadc1;

/* USER CODE END EXPORTED_VARIABLES */

/**
  * @}
  */ 
  
/** @defgroup USBD_CDC_Private_FunctionPrototypes
  * @{
  */
static int8_t CDC_Init_FS     (void);
static int8_t CDC_DeInit_FS   (void);
static int8_t CDC_Control_FS  (uint16_t windex, uint8_t cmd, uint8_t* pbuf, uint16_t length);
static int8_t CDC_Receive_FS  (uint8_t epnum, uint32_t Len);

/* USER CODE BEGIN PRIVATE_FUNCTIONS_DECLARATION */
/* USER CODE END PRIVATE_FUNCTIONS_DECLARATION */

/**
  * @}
  */ 
  
USBD_CDC_ItfTypeDef USBD_Interface_fops_FS = 
{
  CDC_Init_FS,
  CDC_DeInit_FS,
  CDC_Control_FS,  
  CDC_Receive_FS
};

static void MX_USART_UART_CfgSet(uint8_t index, LINE_CODING *plinecode)
{
  UART_HandleTypeDef *phuart[VCP_NUM] = {&huart1, &huart2};
  
  if(HAL_UART_DeInit(phuart[index]) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }
  
  /* set the Stop bit */
  switch (plinecode->format)
  {
  case 0:
    phuart[index]->Init.StopBits = UART_STOPBITS_1;
    break;
  case 2:
    phuart[index]->Init.StopBits = UART_STOPBITS_2;
    break;
  default :
    phuart[index]->Init.StopBits = UART_STOPBITS_1;
    break;
  }
  
  /* set the parity bit*/
  switch (plinecode->paritytype)
  {
  case 0:
    phuart[index]->Init.Parity = UART_PARITY_NONE;
    break;
  case 1:
    phuart[index]->Init.Parity = UART_PARITY_ODD;
    break;
  case 2:
    phuart[index]->Init.Parity = UART_PARITY_EVEN;
    break;
  default :
    phuart[index]->Init.Parity = UART_PARITY_NONE;
    break;
  }
  
  /*set the data type : only 8bits and 9bits is supported */
  switch (plinecode->datatype)
  {
  case 0x07:
    /* With this configuration a parity (Even or Odd) must be set */
    phuart[index]->Init.WordLength = UART_WORDLENGTH_8B;
    break;
  case 0x08:
    if(phuart[index]->Init.Parity == UART_PARITY_NONE)
    {
      phuart[index]->Init.WordLength = UART_WORDLENGTH_8B;
    }
    else 
    {
      phuart[index]->Init.WordLength = UART_WORDLENGTH_9B;
    }
    
    break;
  default :
    phuart[index]->Init.WordLength = UART_WORDLENGTH_8B;
    break;
  }
  
  phuart[index]->Init.BaudRate = plinecode->bitrate;
  phuart[index]->Init.HwFlowCtl  = UART_HWCONTROL_NONE;
  phuart[index]->Init.Mode       = UART_MODE_TX_RX;
  
  if(HAL_UART_Init(phuart[index]) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }

  if(HAL_UART_Receive_DMA(phuart[index], TxQueue[index].Buffer, 1) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }     

  TxQueue[index].InPos = 0;
  TxQueue[index].OutPos = 0;
}
/* Private functions ---------------------------------------------------------*/
/**
  * @brief  CDC_Init_FS
  *         Initializes the CDC media low layer over the FS USB IP
  * @param  None
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_Init_FS(void)
{ 
  uint8_t index;
  uint8_t epaddr[] = {VCP1_OUT_EP, VCP2_OUT_EP};
  
  /* USER CODE BEGIN 3 */ 
  for (index = 0; index < VCP_NUM; index++)
  {
     LineCoding[index].bitrate = 115200;
     LineCoding[index].format = 0;
     LineCoding[index].paritytype = 0;
     LineCoding[index].datatype = 8;

     USBD_LL_PrepareReceive(&hUsbDeviceFS, epaddr[index], RxBuffer[index], CDC_DATA_FS_OUT_PACKET_SIZE);

     TxQueue[index].InPos = 0;
     TxQueue[index].OutPos = 0;
  }

  USBD_LL_PrepareReceive(&hUsbDeviceFS, IO_OUT_EP, IoBuffer, sizeof(IoBuffer));
  
  return (USBD_OK);
  /* USER CODE END 3 */ 
}

/**
  * @brief  CDC_DeInit_FS
  *         DeInitializes the CDC media low layer
  * @param  None
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_DeInit_FS(void)
{
  /* USER CODE BEGIN 4 */ 
  return (USBD_OK);
  /* USER CODE END 4 */ 
}

/**
  * @brief  CDC_Control_FS
  *         Manage the CDC class requests
  * @param  cmd: Command code            
  * @param  pbuf: Buffer containing command data (request parameters)
  * @param  length: Number of data to be sent (in bytes)
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_Control_FS  (uint16_t windex, uint8_t cmd, uint8_t* pbuf, uint16_t length)
{ 
	uint8_t vcpidx = 0;
	
  /* USER CODE BEGIN 5 */
  switch (cmd)
  {
  case CDC_SEND_ENCAPSULATED_COMMAND:
 
    break;

  case CDC_GET_ENCAPSULATED_RESPONSE:
 
    break;

  case CDC_SET_COMM_FEATURE:
 
    break;

  case CDC_GET_COMM_FEATURE:

    break;

  case CDC_CLEAR_COMM_FEATURE:

    break;

  /*******************************************************************************/
  /* Line Coding Structure                                                       */
  /*-----------------------------------------------------------------------------*/
  /* Offset | Field       | Size | Value  | Description                          */
  /* 0      | dwDTERate   |   4  | Number |Data terminal rate, in bits per second*/
  /* 4      | bCharFormat |   1  | Number | Stop bits                            */
  /*                                        0 - 1 Stop bit                       */
  /*                                        1 - 1.5 Stop bits                    */
  /*                                        2 - 2 Stop bits                      */
  /* 5      | bParityType |  1   | Number | Parity                               */
  /*                                        0 - None                             */
  /*                                        1 - Odd                              */ 
  /*                                        2 - Even                             */
  /*                                        3 - Mark                             */
  /*                                        4 - Space                            */
  /* 6      | bDataBits  |   1   | Number Data bits (5, 6, 7, 8 or 16).          */
  /*******************************************************************************/
  case CDC_SET_LINE_CODING:   
    vcpidx = windex / 2; /*每个vcp两个interface  */

    if (vcpidx < VCP_NUM)
    {
        LineCoding[vcpidx].bitrate    = (uint32_t)(pbuf[0] | (pbuf[1] << 8) | (pbuf[2] << 16) | (pbuf[3] << 24));
        LineCoding[vcpidx].format     = pbuf[4];
        LineCoding[vcpidx].paritytype = pbuf[5];
        LineCoding[vcpidx].datatype   = pbuf[6];

        MX_USART_UART_CfgSet(vcpidx, &LineCoding[vcpidx]);
    }
    break;

  case CDC_GET_LINE_CODING:   
    vcpidx = windex / 2; /*每个vcp两个interface  */

    if (vcpidx < VCP_NUM)
    {
        pbuf[0] = (uint8_t)(LineCoding[vcpidx].bitrate);
        pbuf[1] = (uint8_t)(LineCoding[vcpidx].bitrate >> 8);
        pbuf[2] = (uint8_t)(LineCoding[vcpidx].bitrate >> 16);
        pbuf[3] = (uint8_t)(LineCoding[vcpidx].bitrate >> 24);
        pbuf[4] = LineCoding[vcpidx].format;
        pbuf[5] = LineCoding[vcpidx].paritytype;
        pbuf[6] = LineCoding[vcpidx].datatype;   
    }
    break;

  case CDC_SET_CONTROL_LINE_STATE:

    break;

  case CDC_SEND_BREAK:
 
    break;    
    
  default:
    break;
  }

  return (USBD_OK);
  /* USER CODE END 5 */
}

static int8_t CDC_IoResponse (uint8_t *resp, uint32_t Len)
{
    return CDC_Transmit_FS(2, resp, Len);
}


void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *AdcHandle)
{
    uint32_t i;
    uint32_t sum = 0;
    ADC_CMD_S  stresp = {0};

    for (i = 0; i < sizeof(g_adcdata)/sizeof(uint16_t); i++)
    {
        sum += g_adcdata[i];
    }

    stresp.error = 0;
    stresp.val = sum / ADC_MAX_SAMPLES;
    CDC_IoResponse((uint8_t *)&stresp, sizeof(stresp));
}

static void CDC_IoReq_Process (uint8_t *req, uint32_t Len)
{
    uint8_t  cmd = req[0];
    GPIO_TypeDef* gpioports[] = {GPIOA, GPIOB, GPIOC, GPIOD};

    switch (cmd)
    {
        case IO_RESET_CMD:
            __set_FAULTMASK(1);
            HAL_NVIC_SystemReset();
            break;
            
        case IO_GPIO_GET:
            {
                GPIO_CMD_S *preq = (GPIO_CMD_S *)req;
                GPIO_CMD_S  stresp = {0};
                
                if (preq->id >= 64)
                {
                    stresp.error = 0xFF;
                }
                else
                {
                    stresp.level = HAL_GPIO_ReadPin(gpioports[preq->id/16], 1<<(preq->id%16));
                    stresp.error = 0;
                }

                CDC_IoResponse((uint8_t *)&stresp, sizeof(stresp));
            }
            break;
        case IO_GPIO_SET:
            {
                GPIO_CMD_S *preq = (GPIO_CMD_S *)req;
                GPIO_CMD_S  stresp = {0};
                
                if (preq->id >= 64)
                {
                    stresp.error = 0xFF;
                }
                else
                {
                    HAL_GPIO_WritePin(gpioports[preq->id/16], 1<<(preq->id%16), (GPIO_PinState)(preq->level));
                    stresp.error = 0;
                }

                CDC_IoResponse((uint8_t *)&stresp, sizeof(stresp));
            }            
            break;
        case IO_GPIO_INIT:
            {
                GPIO_CMD_S *preq = (GPIO_CMD_S *)req;
                GPIO_CMD_S  stresp = {0};
                
                if (preq->id >= 64)
                {
                    stresp.error = 0xFF;
                }
                else
                {
                    GPIO_InitTypeDef  gpioinitstruct = {0};

                    gpioinitstruct.Pin    = 1<<(preq->id%16);

                    if (preq->dir)
                    {
                        gpioinitstruct.Mode   = GPIO_MODE_OUTPUT_PP;
                    }
                    else
                    {
                        gpioinitstruct.Mode   = GPIO_MODE_INPUT;
                    }
                    
                    gpioinitstruct.Pull   = GPIO_NOPULL;
                    gpioinitstruct.Speed  = GPIO_SPEED_FREQ_HIGH;

                    HAL_GPIO_Init(gpioports[preq->id/16], &gpioinitstruct);
                    HAL_GPIO_WritePin(gpioports[preq->id/16], 1<<(preq->id%16), (GPIO_PinState)(preq->level));
                    
                    stresp.error = 0;
                }

                CDC_IoResponse((uint8_t *)&stresp, sizeof(stresp));
            }            
            break;
            
        case IO_ADC_CMD:
            {
                ADC_CMD_S  stresp = {0};
                
                memset(g_adcdata, 0, sizeof(g_adcdata));
                if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)g_adcdata, ADC_MAX_SAMPLES ) != HAL_OK)
                {
                    stresp.error = 0xFF;
                    CDC_IoResponse((uint8_t *)&stresp, sizeof(stresp));
                }
                else
                {
                    //do nothing
                }
            }
            break; 

        case IO_DFU_CMD:
            {
                DFU_CMD_S stresp = {0};
                
                extern void set_boot_dfu();
                set_boot_dfu();

                CDC_IoResponse((uint8_t *)&stresp, sizeof(stresp));
            }
            break;

        case IO_VER_CMD:
            {
                DFU_CMD_S stresp = {0};

                stresp.val = FW_VER;
 
                CDC_IoResponse((uint8_t *)&stresp, sizeof(stresp));
            }
            break;
            
        default:
            break;
    }

    return;
}

/**
  * @brief  CDC_Receive_FS
  *         Data received over USB OUT endpoint are sent over CDC interface 
  *         through this function.
  *           
  *         @note
  *         This function will block any OUT packet reception on USB endpoint 
  *         untill exiting this function. If you exit this function before transfer
  *         is complete on CDC interface (ie. using DMA controller) it will result 
  *         in receiving more data while previous ones are still not sent.
  *                 
  * @param  Buf: Buffer of data to be received
  * @param  Len: Number of data received (in bytes)
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_Receive_FS (uint8_t epnum, uint32_t Len)
{
  uint8_t  vcpidx = VCP_NUM;
  UART_HandleTypeDef *phuart[VCP_NUM] = {&huart1, &huart2};

  /* USER CODE BEGIN 6 */
  if (Len == 0)
  {
    return USBD_OK;
  }

  if (VCP1_OUT_EP == epnum)
  {
    vcpidx = 0;
  }
  else if (VCP2_OUT_EP == epnum)
  {
    vcpidx = 1;
  }
  else
  {
    //do nothing
  }

  if (vcpidx < VCP_NUM)
  {
      if (Len > CDC_DATA_FS_OUT_PACKET_SIZE)
      {
        Len = CDC_DATA_FS_OUT_PACKET_SIZE;
      }

      USBD_LL_PrepareReceive(&hUsbDeviceFS,
                              epnum,
                              RxBuffer[vcpidx],
                              CDC_DATA_FS_OUT_PACKET_SIZE);

      HAL_UART_Transmit_DMA(phuart[vcpidx], RxBuffer[vcpidx], Len);
  }
  else
  {
      if (Len > sizeof(IoBuffer))
      {
        Len = sizeof(IoBuffer);
      }

      USBD_LL_PrepareReceive(&hUsbDeviceFS,
                              epnum,
                              IoBuffer,
                              sizeof(IoBuffer));

      CDC_IoReq_Process(IoBuffer, Len);
  }
  
  return (USBD_OK);
  /* USER CODE END 6 */ 
}

/**
  * @brief  CDC_Transmit_FS
  *         Data send over USB IN endpoint are sent over CDC interface 
  *         through this function.           
  *         @note
  *         
  *                 
  * @param  Buf: Buffer of data to be send
  * @param  Len: Number of data to be send (in bytes)
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL or USBD_BUSY
  */
uint8_t CDC_Transmit_FS(uint8_t vcpidx, uint8_t* Buf, uint16_t Len)
{
  uint8_t epaddr[] = {VCP1_IN_EP, VCP2_IN_EP, IO_IN_EP};
  
  if(hUsbDeviceFS.pClassData != NULL)
  {
      /* Transmit next packet */
      //USBD_StatusTypeDef status;
      USBD_LL_Transmit(&hUsbDeviceFS, epaddr[vcpidx], Buf, Len);
      //printf("send(%d):status=%d\r\n", Len, status);
      return USBD_OK;
  }
  else
  {
    return USBD_FAIL;
  }

}

/* USER CODE BEGIN PRIVATE_FUNCTIONS_IMPLEMENTATION */
/* USER CODE END PRIVATE_FUNCTIONS_IMPLEMENTATION */

/**
  * @}
  */ 

/**
  * @}
  */ 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

