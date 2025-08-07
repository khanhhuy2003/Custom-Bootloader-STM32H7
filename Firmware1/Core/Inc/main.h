/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
typedef struct{
	uint32_t firmware_size;
	uint32_t firmware_crc;
}Firmware_Info;
/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/

/* USER CODE BEGIN Private defines */
/*
 * COMMAND CODE FOR BOOTLOADER
 */
#define BL_CHECK_CONNECTION 0x50
#define BL_WRITE_MEM 0x51
#define BL_WRITE_MEM_2 0x59
#define BL_JUMP      0x52
#define BL_CHECK_MEM 0x53
#define BL_DEL_MEM   0x54
#define BL_JUMP_TO_USER_CODE 0x55

/*
 * For test security
 */
#define BL_SEND_SIGNATURE	0x57
#define BL_VERIFY_SIGNATURE	0x58
#define BL_CHECK_PREVIOUS_VERSION 0x60
/*
 * FLASH address
 */
#define FLASH_BASE_ADDR      0x08020000  // ví dụ bắt đầu sau bootloader
#define FLASH_END_ADDR       0x080FFFFF  // tuỳ thuộc vào cấu hình linker
#define APP_START_ADDR       0x08020000
#define GO_TO_ADDR           0x08020000
#define BACK_UP_ADDR         0x08060000
#define FLAG                 0x080E0000  // Check if the user want to go back bootloader
#define FIRMWARE_INFO        0x080E0020  //The place where the size and crc are saved4
/*
 * FLASH SECTOR
 */
#define SECTOR_0      1
#define SECTOR_1      2
#define SECTOR_2      3
#define SECTOR_3      4
#define SECTOR_4      5
#define SECTOR_5      6
#define SECTOR_6      7
#define SECTOR_7      8

#define APP1_ADDR_START 0x08020000
#define APP1_ADDR_END   0x0805FFFF
#define APP2_ADDR_START 0x08060000
#define APP2_ADDR_END   0x080A0000

#define SIGNATURE_FLAG_APP1      0x080E0000
#define SIGNATURE_FLAG_APP2      0x080E0020
#define STATUS_FLAG_APP1         0x080C0000
#define STATUS_FLAG_APP2         0x080C0020


#define SIGNATURE_FLAG_APP1_P      ((uint64_t*)0x080E0000)
#define SIGNATURE_FLAG_APP2_P      ((uint64_t*)0x080E0020)
#define STATUS_FLAG_APP1_P         ((uint64_t*)0x080C0000)
#define STATUS_FLAG_APP2_P         ((uint64_t*)0x080C0020)
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
