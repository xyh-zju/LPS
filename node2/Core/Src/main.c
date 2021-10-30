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
#include "route.h"

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

extern int EntryNumber;
extern int RTindex[];
extern RT_Entry* RouteTable[];
extern uint32_t reply_ip;
extern uint32_t My_addr;
extern uint16_t SEQ;

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
uint8_t* TxData;
uint8_t LPTIM1Count = 0;
char RTT_UpBuffer[4096];
struct
{
  uint8_t AliveTxInterval;
}AliveTxParams;
struct
{
  uint8_t TxDone;
}ProcDoneFlag;
typedef struct 
{
  uint32_t ID;
  uint32_t LastSeen;
}RouteTableElement_t;

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
void HAL_LPTIM1_INT_Callback(void) //waiting list resend
{
  MeshPackage* p=(MeshPackage*)malloc(sizeof(MeshPackage));
	p->type=2;
	p->length=0;
  p->des_addr=2;
	p->hop_addr=2;
	p->src_addr=My_addr;
	p->ttl=0;
	p->ack=0;
	p->seq=SEQ;
	p->hops=0;
	Mesh_Send(p, NULL, 0);
  printf("Send success!:%s-end\n", p);
  free(p);
}

// void HAL_RECV_Callback(void){
// 	switch (DeviceState)
//     {
//     case DEVICE_MODE_IDLERX:
//       if(RxDoneFlag)
//       {
//         SX1280GetPayload(RxData, &RxSize, 20);
// 				MeshPackage* p=(MeshPackage*)RxData;
// 				Mesh_Recieve((char*)RxData, RxSize);
// //        printf("RT_num=%d\n",EntryNumber);
// //				for(int i=0; i<EntryNumber; i++){
// //					printf("des:%d, flag:%d, fresh:%d, next:%d, num:%d\n",RouteTable[i]->des_addr, RouteTable[i]->flag, RouteTable[i]->Freshness, RouteTable[i]->next_hop, RouteTable[i]->num_hops);
// //				}

//         RxDoneFlag = 0;
//         LoRaSetRx();
//       }
//       break;
//     default:
//       break;
//     }
// }

void parse_package(MeshPackage* package){
  if(package->type==0) //join
  {
    Mesh_Reply_Join(package); //未实现，结点入网申请，是否需要？
  }
  else if(package->type==1&&package->des_addr==My_addr) //针对自己的应答包
  {
    Mesh_Handle_Reply(package); //处理应答
  }
  else if(package->type==2&&package->hop_addr==My_addr) //转发包
  {
    if(package->des_addr==My_addr) //收到的是发给自己的包
    {
      printf("Get my package from %d, SEQ=%d\n", package->src_addr, package->seq);

      Mesh_Reply(package); //进行应答
      
      //处理数据

    }
    else //发给别人的包
    {
      Mesh_transmit(package); //进行转发
    }
  }
  else if(package->type==3) //广播查找
  {
    Mesh_Handle_Broadcast(package); //处理广播请求
  }
  else //undefined package
  {
    printf("Undefined package");
  }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if(GPIO_Pin == GPIO_PIN_4)//SX1280 Interrupt
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
        ProcDoneFlag.TxDone = 1;
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
  AliveTxParams.AliveTxInterval = 60;
  ProcDoneFlag.TxDone =0;
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
  
  HAL_LPTIM_TimeOut_Start_IT(&hlptim1,0xfffff,0);
  
  LoRaSetRx();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    switch (DeviceState)
    {
    case DEVICE_MODE_IDLERX:
      if(RxDoneFlag)
      {
        SX1280GetPayload(RxData, &RxSize, 20);
				MeshPackage* package=(MeshPackage*)RxData;
        
        parse_package(package);//解析包

        RxDoneFlag = 0;
        LoRaSetRx();
			// 	Mesh_Recieve((char*)RxData, RxSize);
      //  printf("RT_num=%d\n",EntryNumber);
			// 	for(int i=0; i<EntryNumber; i++){
			// 		printf("des:%d, flag:%d, fresh:%d, next:%d, num:%d\n",RouteTable[i]->des_addr, RouteTable[i]->flag, RouteTable[i]->Freshness, RouteTable[i]->next_hop, RouteTable[i]->num_hops);
			// 	}
      }
      break;
    case DEVICE_MODE_RANGING:
      //waiting for ranging
    default:
      break;
    }
    //
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
	
//	for(int i=0; i<10; i++){
//		MeshPackage* p=(MeshPackage*)malloc(sizeof(MeshPackage));
//		p->type=0;
//		p->length=1;
//		p->dest=i;
//		p->src=11;
//		p->ttl=i;
//		TxLength = sizeof(*p);
//		LoRaSendData((uint8_t*)p, TxLength);
//		LoRaSetRx();
//		uint32_t timer=0;
//		//uint8_t success_flag=0;
//		while(timer<=10000000){
////			if(reply_ip==i) {
////				success_flag=1;
////				break;
////			}
//			HAL_RECV_Callback();
//			timer++;
//		}
//		//HAL_RECV_Callback();
//		
//		if(reply_ip==i) printf("success transmit!");
//		else {
//			printf("repeat send des=%d", i--);
//		}
//		
//	}
	
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
