/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "math.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */
enum DemoMode
{
    MASTER = 0,
    SLAVE
};

typedef enum
{
    DEVICE_MODE_IDLERX                              = 0x00,
    DEVICE_MODE_TXALIVE,
    DEVICE_MODE_RANGING,                                                        
}DeviceStates_t;

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);
void HAL_LPTIM1_INT_Callback(void);
void HAL_RECV_Callback(void);
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define KEY1_Pin GPIO_PIN_13
#define KEY1_GPIO_Port GPIOC
#define KEY1_EXTI_IRQn EXTI15_10_IRQn
#define SX1280_NRESET_Pin GPIO_PIN_0
#define SX1280_NRESET_GPIO_Port GPIOA
#define SX1280_SCK_Pin GPIO_PIN_5
#define SX1280_SCK_GPIO_Port GPIOA
#define SX1280_MISO_Pin GPIO_PIN_6
#define SX1280_MISO_GPIO_Port GPIOA
#define SX1280_MOSI_Pin GPIO_PIN_7
#define SX1280_MOSI_GPIO_Port GPIOA
#define SX1280_NSS_CTS_Pin GPIO_PIN_8
#define SX1280_NSS_CTS_GPIO_Port GPIOA
#define SX1280_BUSY_Pin GPIO_PIN_3
#define SX1280_BUSY_GPIO_Port GPIOB
#define SX1280_DIO1_Pin GPIO_PIN_4
#define SX1280_DIO1_GPIO_Port GPIOB
#define SX1280_DIO1_EXTI_IRQn EXTI4_IRQn
/* USER CODE BEGIN Private defines */
#define bool _Bool
#define true 1
#define false 0
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
