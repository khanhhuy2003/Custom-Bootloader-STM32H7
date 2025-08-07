/*
 * bl_command_processing.h
 *
 *  Created on: Aug 7, 2025
 *      Author: ASUS
 */

#ifndef INC_BL_COMMAND_PROCESSING_H_
#define INC_BL_COMMAND_PROCESSING_H_

#include "main.h"
#include "mbedtls.h"
#include "rng.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include "mbedtls/ecdsa.h"
#include "mbedtls/sha256.h"
#include "mbedtls/ecp.h"
#include "mbedtls/platform_util.h"  // Cho mbedtls_mpi_lset()
#include "mbedtls/aes.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdbool.h>
#include "Define.h"
#include "stdio.h"
#include "string.h"

void bl_check_connect_uart(uint8_t* buffer);
void bl_write_mem_uart(uint8_t* buffer);
void bl_check_mem_uart(uint8_t* buffer);
void bl_del_mem(uint8_t* buffer);
void bl_verify_signature(uint8_t* buffer);
void bl_check_version(uint8_t* buffer);



#endif /* INC_BL_COMMAND_PROCESSING_H_ */
