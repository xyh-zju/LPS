/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "lptim.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ranging.h"
#include "SEGGER_RTT.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum
{
    DEVICE_MODE_IDLERX                              = 0x00,
    DEVICE_MODE_TXALIVE,
    DEVICE_MODE_RANGING,                                                        
}DeviceStates_t;
DeviceStates_t DeviceState=DEVICE_MODE_IDLERX;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define LORANGE_ENTITY MASTER
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
double distance;
uint32_t RangingDemoAddress =  0x00010001;
uint8_t RangingDoneFlag = 0;
uint8_t RxDoneFlag = 0;
uint8_t RxData[20];
uint8_t RxSize = 1;
uint32_t TxTimestamp;
uint8_t TxLength;
uint8_t TxData[10]="aassaappqq";
uint8_t LPTIM1Count = 0;
char RTT_UpBuffer[4096];
struct{
  int distance;
}RTT_Value;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void HAL_LPTIM1_INT_Callback(void)
{
  HAL_GPIO_TogglePin(GPIOA,GPIO_PIN_4);
  LPTIM1Count++;
  if(LPTIM1Count>10)
  {
    DeviceState = DEVICE_MODE_TXALIVE;
    TxTimestamp = HAL_GetTick();
    sprintf(TxData,"%8dAA",TxTimestamp);
    TxLength = strlen(TxData);
    LoRaSendData(TxData, TxLength);
    LPTIM1Count = 0;
  }
}
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if(GPIO_Pin == GPIO_PIN_4)//Ranging Interrupt
  {
    SX1280_PacketStatus_t *pktStatus;
    SX1280GetPacketStatus(pktStatus);
    uint16_t IrqStatus = SX1280GetIrqStatus();
    printf("EXTI, RSSI=%f, IRQ=%d, RSSI2=%d\n",((double)(-((int8_t)SX1280GetRssiInst())))/2,IrqStatus,pktStatus->Params.LoRa.RssiPkt);
    if((IrqStatus & SX1280_IRQ_TX_DONE) == SX1280_IRQ_TX_DONE)
    {
        DeviceState = DEVICE_MODE_IDLERX; 
        LoRaSetRx();
        printf("TxDone %d \n",TxTimestamp);
    }
    if((IrqStatus & SX1280_IRQ_RX_DONE) == SX1280_IRQ_RX_DONE)
    {
        RxDoneFlag = 1;
        printf("RxDone\n");
    }

    if(DeviceState == DEVICE_MODE_RANGING) RangingDoneFlag = 1;

    SX1280ClearIrqStatus(SX1280_IRQ_RADIO_ALL);
  }
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI1_Init();
  MX_USART2_UART_Init();
  MX_LPTIM1_Init();
  /* USER CODE BEGIN 2 */
  printf("LoRange| Bulid:%s\n",__DATE__);
  RangingSetParams();
  RangingInitRadio();
  printf("Version:%x\n",SX1280GetFirmwareVersion());
  // if(LORANGE_ENTITY)
  // {
  //   printf("Slave\n");
  //   RangingInit(SX1280_RADIO_RANGING_ROLE_SLAVE,RangingDemoAddress);
  // }
  // else
  // {
  //   printf("Master\n");
  //   RangingInit(SX1280_RADIO_RANGING_ROLE_MASTER,RangingDemoAddress);
  // }
  
  HAL_LPTIM_TimeOut_Start_IT(&hlptim1,0xFFFF,0);
  LoRaSetRx();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    // if(RangingDoneFlag)
    // {
    //   if(LORANGE_ENTITY)//slave
    //   {
    //     RangingInit(SX1280_RADIO_RANGING_ROLE_SLAVE,RangingDemoAddress);
    //   }
    //   else//master
    //   {
    //     distance = SX1280GetRangingResult(SX1280_RANGING_RESULT_RAW);
    //     printf("distance is : %.2lfm\n",distance);
    //     RTT_Value.distance = distance * 10;
    //     SEGGER_RTT_Write(1,&RTT_Value,sizeof(RTT_Value));
    //     HAL_Delay(10);
    //     RangingInit(SX1280_RADIO_RANGING_ROLE_MASTER,RangingDemoAddress);
    //   }
    //   RangingDoneFlag = 0;
    // }

    switch (DeviceState)
    {
    case DEVICE_MODE_IDLERX:
      if(RxDoneFlag)
      {
        SX1280GetPayload(RxData, &RxSize, 20);
        printf("Receive%d %s\n",RxSize,RxData);
        RxDoneFlag = 0;
        LoRaSetRx();
      }
      break;
    default:
      break;
    }
    //
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 40;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART2|RCC_PERIPHCLK_LPTIM1;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
  PeriphClkInit.Lptim1ClockSelection = RCC_LPTIM1CLKSOURCE_PCLK;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
