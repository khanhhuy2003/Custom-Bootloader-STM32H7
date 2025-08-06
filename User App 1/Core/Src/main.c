/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include "Define.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
uint8_t user_buffer[300];
//uint64_t magic_number[4] = {0xDEADBEEFDEADBEEF,0xDEADBEEFDEADBEEF,0xDEADBEEFDEADBEEF,0xDEADBEEFDEADBEEF};
//#define USER_CODE_STATUS_FLAG ((uint64_t*) 0x080C0000)
#define RUN_OK_VALUE  0xDEADBEEF
#define USER_TO_BL_VALUE 0xDEADBEEF
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
//#define APP1_ADDR_START 0x080200E0
//#define APP1_ADDR_START_TEST 0x08000000
//#define APP2_ADDR_START 0x080600E0
#define RUN_OK_FLAG     0x080C0000
#define VERSION_OK_FLAG_ADDR_START 0x080C0020
#define VERSION_OK_FLAG_ADDR_END   0X080CFFFF
#define RUN_OK_VALUE 0xDEADBEEF
//#define RUN_OK_FLAG_2     0x080C0020
#define USER_TO_BL_VALUE 0xDEADBEEF
#define USER_TO_BL_ADDR_START  0x080D0020
#define USER_TO_BL_ADDR_END    0x080DFFFF
#define USER_TO_BL   0x080C0020
//#define USER_TO_BL_2   0x080C0020
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
typedef struct{
	uint32_t run_ok_flag;
	uint32_t retry_counter;
}__attribute__((packed, aligned(32))) FirmwareUserStatus;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MPU_Config(void);
void MX_GPIO_Init(void);
void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
//uint32_t get_next_flash_address_run_ok_dummy() {
//    uint32_t *flash_ptr = (uint32_t *)VERSION_OK_FLAG_ADDR_START;
//
//    while ((uint32_t)flash_ptr < VERSION_OK_FLAG_ADDR_END) {
//        int is_empty = 1;
//        for (int i = 0; i < 8; i++) {
//            if (flash_ptr[i] != 0xFFFFFFFF) {
//                is_empty = 0;
//                break;
//            }
//        }
//        if (is_empty)
//            return (uint32_t)flash_ptr;
//
//        flash_ptr += 8;  // nhảy 32 byte
//    }
//
//    // Nếu không còn chỗ trống, trả về VERSION_OK_FLAG_ADDR_END
//    return VERSION_OK_FLAG_ADDR_END;
//}

int del_mem(uint8_t start, uint8_t number_of_sector){
	HAL_FLASH_Unlock();
    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t SectorError = 0;

    EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
    EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    EraseInitStruct.Banks = FLASH_BANK_1;
    EraseInitStruct.Sector = start;
    EraseInitStruct.NbSectors = number_of_sector;

    if (HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError) == HAL_OK)
    {
        // Erase failed
    	HAL_FLASH_Lock();
    	return 1;
    }
    else
    {
    	HAL_FLASH_Lock();
        return 0;
    }
}

void jump_to_bl_code_uart(){
	__disable_irq();
	uint32_t app_msp = *(volatile uint32_t* )BL_ADDR;
	uint32_t app_reset_handler = *(volatile uint32_t* )(BL_ADDR + 4);
   __set_MSP(app_msp);
   SCB->VTOR = BL_ADDR;
   __DSB();
   __ISB();
   void (*Jump_To_APP)(void) = (void (*)(void))app_reset_handler;
   Jump_To_APP();

}


//void set_run_ok_flag(void) {
//    uint32_t temp_addr = get_next_flash_address_run_ok_dummy();
//	FirmwareUserStatus *status = (FirmwareUserStatus *)temp_addr;
//	//del_mem(FLASH_SECTOR_6, 1);
//
//
//    if (status->run_ok_flag != RUN_OK_VALUE) {
//        FirmwareUserStatus new_status;
//        new_status.run_ok_flag = RUN_OK_VALUE;
//        new_status.retry_counter = 0;
//
//        uint64_t status_block[8] __attribute__((aligned(32)));
//        memset(status_block, 0xFF, sizeof(status_block));
//        memcpy(status_block, &new_status, 8);
//
//        HAL_FLASH_Unlock();
//        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, temp_addr, (uint32_t)&status_block) != HAL_OK) {
//            //....
//        	HAL_UART_Transmit(&huart1, (uint8_t *)" Can not write the run_Ok_flag\r\n", 32, HAL_MAX_DELAY);
//        	return;
//        }
//        HAL_UART_Transmit(&huart1, (uint8_t *)" The firmware works OK\r\n", 32, HAL_MAX_DELAY);
//        HAL_FLASH_Lock();
//    }
//}
void set_run_ok_flag(void) {
    uint32_t temp_addr = 0x080C0000;
    FirmwareUserStatus status_read;
    del_mem(FLASH_SECTOR_6, 1);

    if (status_read.run_ok_flag != RUN_OK_VALUE) {
        FirmwareUserStatus new_status;
        new_status.run_ok_flag = RUN_OK_VALUE;
        new_status.retry_counter = 0;

        uint64_t status_block[8] __attribute__((aligned(32)));
        memset(status_block, 0xFF, sizeof(status_block));
        memcpy(status_block, &new_status, 8);

        HAL_FLASH_Unlock();
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, temp_addr, (uint32_t)&status_block) != HAL_OK) {
            HAL_UART_Transmit(&huart1, (uint8_t *)" Can not write the run_Ok_flag\r\n", 32, HAL_MAX_DELAY);
            HAL_FLASH_Lock();
            return;
        }
        HAL_FLASH_Lock();

        HAL_UART_Transmit(&huart1, (uint8_t *)" The firmware works OK\r\n", 32, HAL_MAX_DELAY);
    }
}

void set_user_to_bl_flag(void) {


    uint32_t temp_addr = 0x080A0000;
    uint32_t user_to_bl_value = USER_TO_BL_VALUE;

    uint64_t status_block[8] __attribute__((aligned(32)));
    memset(status_block, 0xFF, sizeof(status_block));
    memcpy(status_block, &user_to_bl_value, sizeof(user_to_bl_value));

    HAL_FLASH_Unlock();
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, temp_addr, (uint32_t)&status_block) != HAL_OK) {
        HAL_UART_Transmit(&huart1, (uint8_t *)"Failed to write user_to_bl_flag\r\n", 36, HAL_MAX_DELAY);
        HAL_FLASH_Lock();
        return;
    }

    HAL_FLASH_Lock();
    HAL_UART_Transmit(&huart1, (uint8_t *)"✅ Good\r\n", 9, HAL_MAX_DELAY);
}


void user_code_go_back_to_bootloader(uint8_t* buffer){
	uint32_t len = buffer[0] + 1;
	uint32_t crc_host = crc32(buffer, len - 4);
	uint32_t crc_recv = 0;
	memcpy(&crc_recv, &buffer[len - 4], 4);
	if (crc_host == crc_recv) {
		const char *msg = "Preparing to go back to bootloader\r\n";
		HAL_UART_Transmit(&huart1, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
		set_user_to_bl_flag();
		jump_to_bl_code_uart();
	} else {
		const char *msg = "CRC failed\r\n";
		HAL_UART_Transmit(&huart1, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
	}
}
void processing_uart_command(){
	uint8_t rcv_len = 0;
	while(1){
	memset(user_buffer, 0, 300);
	HAL_UART_Receive(&huart1, user_buffer, 1, HAL_MAX_DELAY);
	rcv_len = user_buffer[0];
	HAL_UART_Receive(&huart1, &user_buffer[1], rcv_len, HAL_MAX_DELAY);
	switch(user_buffer[1]){
	case USER_GOBACK_BL:
		user_code_go_back_to_bootloader(user_buffer);
		break;
	default:
		break;
	}
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

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* Enable the CPU Cache */

  /* Enable I-Cache---------------------------------------------------------*/
  //SCB_EnableICache();

  /* Enable D-Cache---------------------------------------------------------*/
  //SCB_EnableDCache();

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
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  char *command = "This is the user applicaiton 1\n";
  HAL_UART_Transmit(&huart1, command, strlen(command), HAL_MAX_DELAY);
  HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_15);
  set_run_ok_flag();
  processing_uart_command();

  while (1)
  {
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

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = 64;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 34;
  RCC_OscInitStruct.PLL.PLLP = 1;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 3072;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
 void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
 void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET);

  /*Configure GPIO pin : PA15 */
  GPIO_InitStruct.Pin = GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x0;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

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
#ifdef USE_FULL_ASSERT
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
