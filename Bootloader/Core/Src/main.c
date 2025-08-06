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
#include "mbedtls.h"
#include "memorymap.h"
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
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#define MAGIC_ADDR ((uint64_t*)0x080E0000)
#define FIRMWARE_ADDR ((uint64_t*)0x08020000)
////////////////
#define VERSION_OK_FLAG 0x080A0000
#define VERSION_OK_FLAG_ADDR_START 0x080C0020
#define VERSION_OK_FLAG_ADDR_END   0X080CFFFF
#define RUN_OK_VALUE 0xDEADBEEF
///////////////////////////
#define MAX_FAILED_ALLOWED 1
/////////////////////////////////////
#define FLASH_LOG_START 0x080C0000
///////////////////////////////////////
#define USER_TO_BL_VALUE 0xDEADBEEF
#define USER_TO_BL_ADDR_START 0x080D0020
#define USER_TO_BT_ADD_END    0x080DFFFF

/*
 * Flag check run ok
 */
#define STATUS_ADDR_APP1_FLAG 0x080C0000
#define STATUS_ADDR_APP2_FLAG 0x080C0020
int flag_to_jump = 0;
uint8_t rx_buffer[300];
uint8_t global_flag = 0;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
const uint8_t public_key[64] = {
    // X
    0xfc, 0x3d, 0x37, 0xdb, 0x8f, 0x6a, 0x1d, 0xe9,
    0xbd, 0x6d, 0xb7, 0x9f, 0xa0, 0x11, 0x4a, 0x49,
    0xea, 0xa7, 0x44, 0x94, 0xdb, 0x4e, 0xf0, 0x04,
    0x50, 0x5c, 0xc6, 0x47, 0xc5, 0xf6, 0x47, 0x3e,

    // Y
    0x96, 0x71, 0x56, 0x73, 0x02, 0x85, 0xf2, 0xc1,
    0x53, 0xab, 0x07, 0xdf, 0x48, 0x5b, 0x9c, 0xcc,
    0x5b, 0xa2, 0xc4, 0x27, 0xf1, 0x2d, 0x84, 0x9a,
    0x5c, 0xf7, 0xcf, 0xf4, 0xf1, 0x94, 0x73, 0x30
};
const uint8_t aes_key[16] = {
    0x2B, 0x7E, 0x15, 0x16,
    0x28, 0xAE, 0xD2, 0xA6,
    0xAB, 0xF7, 0x15, 0x88,
    0x09, 0xCF, 0x4F, 0x3C
};

uint8_t iv[16] = {
    0x00, 0x01, 0x02, 0x03,
    0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B,
    0x0C, 0x0D, 0x0E, 0x0F
};

bool is_valid_msp(uint32_t msp)
	{
	    return (
	        ((msp >= 0x20000000) && (msp <= 0x2001FFFF)) || // DTCMRAM
	        ((msp >= 0x24000000) && (msp <= 0x2405FFFF)) || // RAM_D1
	        ((msp >= 0x30000000) && (msp <= 0x30007FFF)) || // RAM_D2
	        ((msp >= 0x38000000) && (msp <= 0x38003FFF))    // RAM_D3
	    );
	}
uint64_t expected_magic_number[4] = {
	    0xDEADBEEFDEADBEEF,
	    0xDEADBEEFDEADBEEF,
	    0xDEADBEEFDEADBEEF,
	    0xDEADBEEFDEADBEEF
};
bool check_magic_number(){
    for(int i = 0; i < 4; i++){
    	if(MAGIC_ADDR[i] != expected_magic_number[i]){
    		return false;
    	}
    }
    return true;
}// User app want to jump to bootloader
bool check_signature_magic_number_app_1(){
    for(int i = 0; i < 4; i++){
    	if(SIGNATURE_FLAG_APP1_P[i] != expected_magic_number[i]){
    		return false;
    	}
    }
    return true;
}// Check signature
bool check_signature_magic_number_app_2(){
    for(int i = 0; i < 4; i++){
    	if(SIGNATURE_FLAG_APP2_P[i] != expected_magic_number[i]){
    		return false;
    	}
    }
    return true;
}//
uint32_t cal_flash_used_app_1(void) {
    uint32_t count;
    __asm__ volatile (
        "mov r0, #0\n"               // count = 0
        "ldr r1, =0x08020000\n"      // ptr = start
        "ldr r2, =0x0805FFFF\n"      // end = 0x08060000

        "loop_start:\n"
        "cmp r1, r2\n"
        "bge done\n"

        "ldr r3, [r1], #4\n"         // load word1, ptr += 4
        "ldr r4, [r1], #4\n"         // load word2
        "ldr r5, [r1], #4\n"         // load word3
        "ldr r6, [r1], #4\n"         // load word4

        "cmp r3, #0xFFFFFFFF\n"
        "bne inc\n"
        "cmp r4, #0xFFFFFFFF\n"
        "bne inc\n"
        "cmp r5, #0xFFFFFFFF\n"
        "bne inc\n"
        "cmp r6, #0xFFFFFFFF\n"
        "bne inc\n"
        "b loop_start\n"

        "inc:\n"
        "add r0, r0, #1\n"
        "b loop_start\n"

        "done:\n"
        "mov %[out], r0\n"
        : [out] "=r" (count)
        :
        : "r0", "r1", "r2", "r3", "r4", "r5", "r6"
    );
    return count;
}

uint32_t cal_flash_used_app_2(void) {
    uint32_t count;
    __asm__ volatile (
        "mov r0, #0\n"
        "ldr r1, =0x08060000\n"
        "ldr r2, =0x0809FFFF\n"

        "flash1_loop_start:\n"
        "cmp r1, r2\n"
        "bge flash1_done\n"

        "ldr r3, [r1], #4\n"
        "ldr r4, [r1], #4\n"
        "ldr r5, [r1], #4\n"
        "ldr r6, [r1], #4\n"

        "cmp r3, #0xFFFFFFFF\n"
        "bne flash1_inc\n"
        "cmp r4, #0xFFFFFFFF\n"
        "bne flash1_inc\n"
        "cmp r5, #0xFFFFFFFF\n"
        "bne flash1_inc\n"
        "cmp r6, #0xFFFFFFFF\n"
        "bne flash1_inc\n"
        "b flash1_loop_start\n"

        "flash1_inc:\n"
        "add r0, r0, #1\n"
        "b flash1_loop_start\n"

        "flash1_done:\n"
        "mov %[out], r0\n"
        : [out] "=r" (count)
        :
        : "r0", "r1", "r2", "r3", "r4", "r5", "r6"
    );

    return count;
}
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

void bl_jump_to_code_uart(uint8_t *buffer){
	uint32_t len = buffer[0] + 1;
	uint32_t crc_host = crc32(buffer, len - 4);
	uint32_t crc_recv = 0;
	memcpy(&crc_recv, &buffer[len - 4], 4);

	if (crc_host == crc_recv) {
//		if(check_signature_magic_number()){
//			const char *msg = "Preparing to jump to user code\r\n";
//			HAL_UART_Transmit(&huart1, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
//			jump_to_user_code_uart();
//		}
//		else{
//			const char *msg = "Signature failed \r\n";
//			HAL_UART_Transmit(&huart1, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
//			return;
//		}
	} else {
		const char *msg = "CRC failed\r\n";
		HAL_UART_Transmit(&huart1, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
	}

}

void bl_check_connect_uart(uint8_t *buffer) {
	uint32_t len = buffer[0] + 1;
	uint32_t crc_host = crc32(buffer, len - 4);
	uint32_t crc_recv = 0;
	memcpy(&crc_recv, &buffer[len - 4], 4);

	if (crc_host == crc_recv) {
		const char *msg = "CONNECTED\r\n";
		HAL_UART_Transmit(&huart1, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
	} else {
		const char *msg = "CRC failed\r\n";
		HAL_UART_Transmit(&huart1, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
	}
}
static uint32_t current_verified_app_base = 0;
void bl_reset_metadata_state(void) {
    current_verified_app_base = 0;
}
typedef struct{
	uint32_t version;
	uint32_t address;
}__attribute__((packed, aligned(32))) FirmwareMetadata;
typedef struct{
	uint32_t run_ok_flag;
	uint32_t retry_counter;
}__attribute__((packed, aligned(32))) FirmwareUserStatus;
void bl_write_mem_uart(uint8_t *buffer) {
	FirmwareMetadata F;
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_dec(&aes, aes_key, 128);

    uint32_t command_len = buffer[0] + 1;
    uint32_t crc_host = crc32(buffer, command_len - 4);
    uint32_t crc_recv;
    memcpy(&crc_recv, &buffer[command_len - 4], 4);

    if (crc_host != crc_recv) {
        HAL_UART_Transmit(&huart1, (uint8_t *)"CRC failed\r\n", 12, HAL_MAX_DELAY);
        return;
    }

    uint32_t address;
    uint8_t size_firmware;
    memcpy(&address, &buffer[2], 4);
    memcpy(&size_firmware, &buffer[6], 1);

    uint8_t *firmware = &buffer[7];

    if (address == APP1_ADDR_START || address == APP2_ADDR_START) {//First chunk, check metedata
        uint32_t version;
        uint32_t total_firmware_length;
        memcpy(&version, &firmware[0], 4);
        memcpy(&total_firmware_length, &firmware[4], 4);

        if (total_firmware_length > 256000) {
            HAL_UART_Transmit(&huart1, (uint8_t *)"❌ FIRMWARE TOO LARGE\r\n", 24, HAL_MAX_DELAY);
            bl_reset_metadata_state();
            return;
        }

        // 👉 Lấy ECC signature
        uint8_t *sig = &firmware[8];  // 64 bytes //ECC
        uint8_t version_data[8];
        memcpy(version_data, &firmware[0], 8);
        uint32_t version_ex;
        memcpy(&version_ex, &firmware[0], 4);

        F.address = address + 224;
        F.version = version_ex;

        uint64_t *flag_ptr = (uint64_t *)0X08017680;
        while (*flag_ptr != 0xFFFFFFFFFFFFFFFFULL) {
            uint32_t stored_version = *(uint32_t *)flag_ptr;
            if (version_ex <= stored_version) {
                HAL_UART_Transmit(&huart1, (uint8_t *)"❌ Version is too old\r\n", 24, HAL_MAX_DELAY);
                bl_reset_metadata_state();
                return;
            }
            flag_ptr += 4;
        }
        uint8_t hash[32];
        mbedtls_sha256(version_data, 8, hash, 0); // 0 = SHA256

        mbedtls_ecdsa_context ctx;
        mbedtls_ecdsa_init(&ctx);
        mbedtls_ecp_group_load(&ctx.grp, MBEDTLS_ECP_DP_SECP256R1);
        mbedtls_mpi_read_binary(&ctx.Q.X, public_key, 32);
        mbedtls_mpi_read_binary(&ctx.Q.Y, public_key + 32, 32);
        mbedtls_mpi_lset(&ctx.Q.Z, 1);

        mbedtls_mpi r, s;
        mbedtls_mpi_init(&r); mbedtls_mpi_init(&s);
        mbedtls_mpi_read_binary(&r, sig, 32);
        mbedtls_mpi_read_binary(&s, sig + 32, 32);

        int ret = mbedtls_ecdsa_verify(&ctx.grp, hash, 32, &ctx.Q, &r, &s);

        mbedtls_mpi_free(&r); mbedtls_mpi_free(&s);
        mbedtls_ecdsa_free(&ctx);

        if (ret != 0) {
            HAL_UART_Transmit(&huart1, (uint8_t *)"❌ Metadata Signature FAIL\r\n", 28, HAL_MAX_DELAY);
            bl_reset_metadata_state();
            return;
        }
        uint64_t version_block[8] __attribute__((aligned(32))); // 64 bytes
        memset(version_block, 0xFF, sizeof(version_block));
        memcpy(version_block, &F, 8); // chỉ lưu version

        HAL_FLASH_Unlock();
        uint32_t dummy_flag = 0X08017680; //0x08020000
        uint64_t temp;
        while (1) {
            memcpy(&temp, dummy_flag, sizeof(temp));
            if (temp == 0xFFFFFFFFFFFFFFFFULL) break;
            dummy_flag += 32;
        }
        if (((uint32_t)dummy_flag % 32) != 0) {
            HAL_UART_Transmit(&huart1, (uint8_t *)"❌ Addr not aligned\r\n", 22, HAL_MAX_DELAY);
            return;
        }

		if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD , dummy_flag , (uint32_t)&version_block) != HAL_OK) {
			HAL_FLASH_Lock();
			HAL_UART_Transmit(&huart1, (uint8_t *)"WRITE ERROR\r\n", 14, HAL_MAX_DELAY);
			bl_reset_metadata_state();
			return;
		}
	    HAL_FLASH_Lock();
	    current_verified_app_base = 1;
        HAL_UART_Transmit(&huart1, (uint8_t *)"✅ Metadata Signature OK\r\n", 26, HAL_MAX_DELAY);

        return;
    }

    if(current_verified_app_base != 1){
    	HAL_UART_Transmit(&huart1, (uint8_t *)"ECC of version is FAILED\r\n", 30, HAL_MAX_DELAY);
    	return;
    }
	if (address < 0x08000000 || address + size_firmware > 0x08200000 || address % 32 != 0) {
		HAL_UART_Transmit(&huart1, (uint8_t *)"INVALID ADDRESS\r\n", 17, HAL_MAX_DELAY);
		return;
	}

	uint8_t padded_input[224];
	uint8_t decrypted_firmware[224];
	memset(padded_input, 0xFF, sizeof(padded_input));
	memcpy(padded_input, firmware, size_firmware);

	if (mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_DECRYPT, 224, iv, padded_input, decrypted_firmware)) {
		HAL_UART_Transmit(&huart1, (uint8_t *)"DECRYPT FAILED\r\n", 17, HAL_MAX_DELAY);
		return;
	}
	/*
	 * decrypt
	 */
	SCB_DisableICache();
	// SCB_DisableDCache();

	if (HAL_FLASH_Unlock() != HAL_OK) {
		HAL_UART_Transmit(&huart1, (uint8_t *)"UNLOCK ERROR\r\n", 15, HAL_MAX_DELAY);
		return;
	}

	uint64_t val[28] __attribute__((aligned(32)));
	memset(val, 0xFF, sizeof(val));
	memcpy(val, decrypted_firmware, size_firmware);

	for (uint32_t offset = 0; offset < size_firmware; offset += 32) {
		if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, address + offset, (uint32_t)&val[offset / 8]) != HAL_OK) {
			HAL_FLASH_Lock();
			HAL_UART_Transmit(&huart1, (uint8_t *)"WRITE ERROR\r\n", 14, HAL_MAX_DELAY);
			return;
		}
	}

	HAL_FLASH_Lock();

	HAL_UART_Transmit(&huart1, (uint8_t *)"WRITE OK\r\n", 10, HAL_MAX_DELAY);

}
void bl_del_mem(uint8_t* buffer){
    uint32_t command_len = buffer[0] + 1;
    uint32_t host_crc = crc32(buffer, command_len - 4);
    uint32_t crc_recv = 0;
    memcpy(&crc_recv, &buffer[command_len - 4], 4);
    if(host_crc == crc_recv){
    	uint32_t addr = 0;
    	memcpy(&addr, &buffer[2], 4);

        if (addr != APP1_ADDR_START && addr != APP2_ADDR_START) {
            HAL_UART_Transmit(&huart1, (uint8_t *)"INVALID ERASE ADDR\r\n", 21, HAL_MAX_DELAY);
            return;
        }
        else if(addr == APP1_ADDR_START){
        	if(del_mem(FLASH_SECTOR_1, 2) == 1 && del_mem(FLASH_SECTOR_7, 1) == 1 ){
            	const char *msg = "DELETE APP 1 SUCCESSFULLY\r\n";
            	HAL_UART_Transmit(&huart1, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
            	return;
        	}
        	else{
            	const char *msg = "DELETE APP 1 FAILED\r\n";
            	HAL_UART_Transmit(&huart1, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
            	return;
        	}
        }
        else if(addr == APP2_ADDR_START){
        	if(del_mem(FLASH_SECTOR_3, 2) == 1 && del_mem(FLASH_SECTOR_7, 1) == 1){
            	const char *msg = "DELETE APP 2 SUCCESSFULLY\r\n";
            	HAL_UART_Transmit(&huart1, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
            	return;
        	}
        	else{
            	const char *msg = "DELETE DELETE APP2 FAILED\r\n";
            	HAL_UART_Transmit(&huart1, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
            	return;
        	}

        }
    }
    else{
		const char *msg = "CRC failed\r\n";
		HAL_UART_Transmit(&huart1, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
		return;
    }
}
void bl_check_mem_uart(uint8_t *buffer) {
	uint32_t command_len = buffer[0] + 1;
	uint32_t crc_host = crc32(buffer, command_len - 4);
	uint32_t crc_recv;
	memcpy(&crc_recv, &buffer[command_len - 4], 4);
	if (crc_host != crc_recv) {
		const char *msg = "CRC failed\r\n";
		HAL_UART_Transmit(&huart1, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
		return ;
	}
	char dummy[150];
	uint32_t result1 = cal_flash_used_app_1();
	uint32_t result2 = cal_flash_used_app_2();
	sprintf(dummy, "Segment 1 of flash use %d / 256000 bytes and Segment 2 of flash use %d /256000 bytes. \n", result1 * 16, result2 * 16);
	HAL_UART_Transmit(&huart1, (uint8_t *)dummy, strlen(dummy), HAL_MAX_DELAY);
}

void bl_verify_signature(uint8_t *buffer) {
    uint32_t command_len = buffer[0] + 1;
    uint32_t crc_recv = 0;
    memcpy(&crc_recv, &buffer[command_len - 4], 4);
    uint32_t host_crc = crc32(buffer, command_len - 4);

    if (crc_recv != host_crc) {
        HAL_UART_Transmit(&huart1, (uint8_t *)"CRC mismatch\r\n", 14, HAL_MAX_DELAY);
        return;
    }

    uint8_t *payload = &buffer[2];  // Skip LTF
    uint32_t user_app_address = 0;
    memcpy(&user_app_address, &payload[0], 4);

    uint32_t firmware_len = 0;
    memcpy(&firmware_len, &payload[4], 4);

    uint8_t *host_hash = &payload[8];          // SHA256 (32 bytes)
    uint8_t *sig = &payload[8 + 32];           // ECC Signature (64 bytes)
    uint8_t calc_hash[32];
    mbedtls_sha256_context sha_ctx;
    mbedtls_sha256_init(&sha_ctx);
    mbedtls_sha256_starts_ret(&sha_ctx, 0); // 0 = SHA256
    user_app_address += 224;

    for (uint32_t i = 0; i < firmware_len; i++) {
        uint8_t byte = *(uint8_t *)(user_app_address + i);
        mbedtls_sha256_update_ret(&sha_ctx, &byte, 1);
    }
    mbedtls_sha256_finish_ret(&sha_ctx, calc_hash);
    mbedtls_sha256_free(&sha_ctx);

    if (memcmp(host_hash, calc_hash, 32) != 0) {
        HAL_UART_Transmit(&huart1, (uint8_t *)"❌ SHA256 mismatch\r\n", 21, HAL_MAX_DELAY);
        return;
    }
    mbedtls_ecdsa_context ctx;
    mbedtls_ecdsa_init(&ctx);
    mbedtls_ecp_group_load(&ctx.grp, MBEDTLS_ECP_DP_SECP256R1);
    mbedtls_mpi_read_binary(&ctx.Q.X, public_key, 32);
    mbedtls_mpi_read_binary(&ctx.Q.Y, public_key + 32, 32);
    mbedtls_mpi_lset(&ctx.Q.Z, 1);

    mbedtls_mpi r, s;
    mbedtls_mpi_init(&r); mbedtls_mpi_init(&s);
    mbedtls_mpi_read_binary(&r, sig, 32);
    mbedtls_mpi_read_binary(&s, sig + 32, 32);

    int ret = mbedtls_ecdsa_verify(&ctx.grp, calc_hash, 32, &ctx.Q, &r, &s);

    mbedtls_mpi_free(&r); mbedtls_mpi_free(&s);
    mbedtls_ecdsa_free(&ctx);

    if (ret != 0) {
        HAL_UART_Transmit(&huart1, (uint8_t *)"❌ Signature and Integrity FAIL\r\n", 33, HAL_MAX_DELAY);
        return;
    }

    HAL_FLASH_Unlock();

    uint32_t sig_flag_addr = 0;
    uint32_t status_flag_addr = 0;

    if (user_app_address == APP1_ADDR_START + 224) {
        sig_flag_addr = SIGNATURE_FLAG_APP1;
        status_flag_addr = STATUS_FLAG_APP1;
    } else if (user_app_address == APP2_ADDR_START + 224) {
        sig_flag_addr = SIGNATURE_FLAG_APP2;
        status_flag_addr = STATUS_FLAG_APP2;
    } else {
        HAL_UART_Transmit(&huart1, (uint8_t *)"❌ Unknown APP address\r\n", 24, HAL_MAX_DELAY);
        HAL_FLASH_Lock();
        return;
    }

    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, sig_flag_addr, (uint32_t)expected_magic_number) != HAL_OK) {
        HAL_UART_Transmit(&huart1, (uint8_t *)"❌ Failed to write flags\r\n", 26, HAL_MAX_DELAY);
        HAL_FLASH_Lock();
        return;
    }

    HAL_FLASH_Lock();

    HAL_UART_Transmit(&huart1, (uint8_t *)"✅ Signature and Integrity OK\r\n", 31, HAL_MAX_DELAY);
}
void bl_check_version(uint8_t* buffer){
    uint32_t len = buffer[0] + 1;
    uint32_t crc_host = crc32(buffer, len - 4);
    uint32_t crc_recv = 0;
    memcpy(&crc_recv, &buffer[len - 4], 4);

    if (crc_host == crc_recv) {
        uint32_t dummy_flag = 0x08017680;
        uint64_t temp;
        char msg[128];
        uint8_t first = 1;

        HAL_UART_Transmit(&huart1, (uint8_t *)"{ \"versions\": [", 15, HAL_MAX_DELAY);

        while (1) {
            memcpy(&temp, (void*)dummy_flag, sizeof(temp));
            if (temp == 0xFFFFFFFFFFFFFFFFULL) break;

            uint32_t version;
            memcpy(&version, &temp, 4);

            if (!first) HAL_UART_Transmit(&huart1, (uint8_t *)", ", 2, HAL_MAX_DELAY);
            sprintf(msg, "\"0x%08lX\"", version);
            HAL_UART_Transmit(&huart1, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);

            first = 0;
            dummy_flag += 32;
        }

        HAL_UART_Transmit(&huart1, (uint8_t *)"] }\r\n", 5, HAL_MAX_DELAY);

    } else {
        const char *msg = "❌ CRC failed\r\n";
        HAL_UART_Transmit(&huart1, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
    }
}
void jump_to_user_code_uart(uint32_t addr){
	__disable_irq();
	uint32_t app_msp = *(volatile uint32_t* )addr;
	uint32_t app_reset_handler = *(volatile uint32_t* )(addr + 4);
   __set_MSP(app_msp);
   SCB->VTOR = addr;
   void (*Jump_To_APP)(void) = (void (*)(void))app_reset_handler;

   Jump_To_APP();

}
int check_ECC_Flag(uint32_t addr){
	if(addr == 0x080200E0){
		if(check_signature_magic_number_app_1()){
			return 1;
		}
		else{
			return 0;
		}
	}
	else if(addr == 0x080600E0){
		if(check_signature_magic_number_app_2()){
			return 1;
		}
		else{
			return 0;
		}
	}
	return 0;
}
int is_fail_counter_exceeded(uint32_t status_addr) {
	FirmwareUserStatus *status = (FirmwareUserStatus *)status_addr;
    return (status->retry_counter >= MAX_FAILED_ALLOWED);
}
int is_run_ok_flag_set(uint32_t status_addr) {
	FirmwareUserStatus *status = (FirmwareUserStatus *)status_addr;
    return (status->run_ok_flag == RUN_OK_VALUE);
}

uint8_t get_fail_counter(uint32_t status_addr) {
    FirmwareUserStatus *status = (FirmwareUserStatus *)status_addr;
    // Nếu địa chỉ hiện tại trống
    if (status->retry_counter == 0xFFFFFFFF) {
        uint32_t dummy_addr = status_addr;
        FirmwareUserStatus new_status = {
            .run_ok_flag = 0xFFFFFFFF,
            .retry_counter = 0
        };

        uint64_t status_block[8] __attribute__((aligned(32))); // 64 bytes
        memset(status_block, 0xFF, sizeof(status_block));
        memcpy(status_block, &new_status, sizeof(FirmwareUserStatus));

        HAL_FLASH_Unlock();
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, dummy_addr, (uint32_t)&status_block) != HAL_OK) {
            HAL_UART_Transmit(&huart1, (uint8_t *)"❌ Failed to write status flags get\r\n", 40, HAL_MAX_DELAY);
        }
        HAL_FLASH_Lock();

        // ✅ đọc lại từ địa chỉ vừa ghi
        status = (FirmwareUserStatus *)dummy_addr;
    }
    return status->retry_counter;
}

void increase_fail_counter(uint32_t status_addr){

	if(status_addr == 0xFFFFFFFF){
		HAL_UART_Transmit(&huart1, (uint8_t*)"Address of run_ok_flag is invalid", 35, HAL_MAX_DELAY);
		return 0;
	}
	FirmwareUserStatus *status = (FirmwareUserStatus *)status_addr;
	FirmwareUserStatus new_status;
	del_mem(FLASH_SECTOR_6, 1);
	new_status.run_ok_flag = 0xFFFFFFFF;
	new_status.retry_counter = 1;
	uint64_t status_block[8] __attribute__((aligned(32))); // 64 bytes
	memset(status_block, 0xFF, sizeof(status_block));
	memcpy(status_block, &new_status, sizeof(FirmwareUserStatus)); // chỉ lưu version
	HAL_FLASH_Unlock();
	if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, status_addr, (uint32_t)&status_block) != HAL_OK) {
		HAL_UART_Transmit(&huart1, (uint8_t *)"❌ Failed to write status flags\r\n", 26, HAL_MAX_DELAY);

	}
	HAL_FLASH_Lock();

}
//void increase_fail_counter(uint32_t status_addr) {
//    if (status_addr == 0xFFFFFFFF) {
//        HAL_UART_Transmit(&huart1, (uint8_t*)"Invalid status address\r\n", 25, HAL_MAX_DELAY);
//        return;
//    }
//
//    FirmwareUserStatus *old = (FirmwareUserStatus *)status_addr;
//    uint8_t retry = old->retry_counter;
//    FirmwareUserStatus new_status = {
//        .run_ok_flag = 0xFFFFFFFF,
//        .retry_counter = 1
//    };
//
//    uint32_t new_addr = status_addr;
//    if(*(uint32_t*)new_addr != 0xFFFFFFFF){
//    	HAL_UART_Transmit(&huart1, (uint8_t*)"Have data\r\n", 25, HAL_MAX_DELAY);
//
//    }
//
//    uint64_t status_block[8] __attribute__((aligned(32)));
//    memset(status_block, 0xFF, sizeof(status_block));
//    memcpy(status_block, &new_status, sizeof(FirmwareUserStatus));
//
//    HAL_FLASH_Unlock();
//    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, new_addr, (uint32_t)&status_block) != HAL_OK) {
//        HAL_UART_Transmit(&huart1, (uint8_t *)"❌ Failed to write status flags in\r\n", 40, HAL_MAX_DELAY);
//    }
//    HAL_FLASH_Lock();
//}

//void check_condition_jump_to_code_uart() {
//    FirmwareMetadata *F = (FirmwareMetadata*)VERSION_OK_FLAG;
////    FirmwareUserStatus *new_status = (FirmwareUserStatus *)temp_addr;
////    new_status->run_ok_flag = ...;
//    while (F->version != 0xFFFFFFFF && F->address != 0xFFFFFFFF) {
//        F++;
//    }
//    F--;
//
//    if (F->version == 0xFFFFFFFF || F->address == 0xFFFFFFFF) {
//        HAL_UART_Transmit(&huart1, (uint8_t *)"No valid firmware found\r\n", 28, HAL_MAX_DELAY);
//        return;
//    }
//
//    uint32_t addr = F->address;
////    uint32_t status_addr = (addr == (APP1_ADDR_START + 224)) ? STATUS_ADDR_APP1_FLAG : STATUS_ADDR_APP2_FLAG;
//    uint32_t status_addr = STATUS_ADDR_APP1_FLAG ;
//
//    uint32_t stay_in_bl_app =  *(uint32_t *)0x080C0000;
//
//    if(stay_in_bl_app == 0xDEADBEEF){
//    	return;
//    }
//    if (!check_ECC_Flag(addr)) {
//        HAL_UART_Transmit(&huart1, (uint8_t *)"ECC Signature invalid\r\n", 26, HAL_MAX_DELAY);
//        return;
//    }
//    uint8_t fail_counter = get_fail_counter(status_addr);
//
//    if(fail_counter > 0 && !is_run_ok_flag_set(status_addr)){
//    	HAL_UART_Transmit(&huart1, (uint8_t *)"Firmware doesn't work OK\r\n", 26, HAL_MAX_DELAY);
//    	return;
//    }
//    if(fail_counter == 0){ //The first time
//        HAL_UART_Transmit(&huart1, (uint8_t *)"First time run - increasing fail counter\r\n", 45, HAL_MAX_DELAY);
//        increase_fail_counter(status_addr);
//    }
//    HAL_UART_Transmit(&huart1, (uint8_t *)"Jumping to user code...\r\n", 35, HAL_MAX_DELAY);
//    jump_to_user_code_uart(addr);
//}
int count_version_in_flash(){
    int count = 0;
    uint32_t *temp_addr_flash = (uint32_t *)0x08017680;

    while (*temp_addr_flash != 0xFFFFFFFF) {
        count++;
        temp_addr_flash += 8;  // Tăng theo đơn vị 4 bytes
    }
    return count;
}
void check_condition_jump_to_code_uart() {
    FirmwareMetadata *F = (FirmwareMetadata*)0x08017680;
    int count_max = count_version_in_flash();
    FirmwareMetadata valid_firmware[count_max];
    int count  = 0;
    // Collect valid firmware
    while (F->version != 0xFFFFFFFF && F->address != 0xFFFFFFFF && count < count_max) {
        valid_firmware[count++] = *F++;
    }
    if (count == 0) {
        HAL_UART_Transmit(&huart1, (uint8_t *)"❌ No valid firmware found\r\n", 28, HAL_MAX_DELAY);
        return;
    }
    uint32_t user_to_bl_value = 0x080A0000;
    if (*(uint32_t*)user_to_bl_value == 0xDEADBEEF) {
        return;
    }
    for (int i = count - 1; i >= 0; i--) {
        uint32_t addr = valid_firmware[i].address;
        uint32_t status_addr = 0x080C0000;
        uint8_t fail_counter = get_fail_counter(status_addr);
        if (!check_ECC_Flag(addr)) {
        	HAL_UART_Transmit(&huart1, (uint8_t *)"ECC of firmware at is failed\r\n", 41, HAL_MAX_DELAY);
            continue;
        }
        if (fail_counter == 0) {
            // Lần chạy đầu tiên
            HAL_UART_Transmit(&huart1, (uint8_t *)"First run - increasing fail-counter\r\n", 41, HAL_MAX_DELAY);
            increase_fail_counter(status_addr);
            jump_to_user_code_uart(addr);
        } else if (is_run_ok_flag_set(status_addr)) {
            jump_to_user_code_uart(addr);
            return;
        } else {
            continue;
        }
    }

    HAL_UART_Transmit(&huart1, (uint8_t *)"All firmwares failed, staying in bootloader\r\n", 48, HAL_MAX_DELAY);
}

void process_uart_command() {
	uint8_t rcv_len = 0;
	while(1){
	memset(rx_buffer, 0, 300);
	HAL_UART_Receive(&huart1, rx_buffer, 1, HAL_MAX_DELAY);
	rcv_len = rx_buffer[0];
	HAL_UART_Receive(&huart1, &rx_buffer[1], rcv_len, HAL_MAX_DELAY);
	switch(rx_buffer[1]){
	case BL_CHECK_CONNECTION:
		bl_check_connect_uart(rx_buffer);
		break;
	case BL_WRITE_MEM:
		bl_write_mem_uart(rx_buffer);
		break;
	case BL_CHECK_MEM:
		bl_check_mem_uart(rx_buffer);
		break;
	case BL_DEL_MEM:
		bl_del_mem(rx_buffer);
		break;
	case BL_JUMP_TO_USER_CODE:
		bl_jump_to_code_uart(rx_buffer);
		break;
	case BL_VERIFY_SIGNATURE:
		bl_verify_signature(rx_buffer);
		break;
	case BL_CHECK_PREVIOUS_VERSION:
		bl_check_version(rx_buffer);
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
  MX_TIM6_Init();
  MX_USART1_UART_Init();
  //MX_RNG_Init();
  MX_MBEDTLS_Init();
  /* USER CODE BEGIN 2 */

  check_condition_jump_to_code_uart();
  //clear_flag();
  del_mem(FLASH_SECTOR_5, 1);


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  process_uart_command();
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 68;
  RCC_OscInitStruct.PLL.PLLP = 1;
  RCC_OscInitStruct.PLL.PLLQ = 11;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 6144;
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
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Number = MPU_REGION_NUMBER1;
  MPU_InitStruct.BaseAddress = 0x30000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_1KB;
  MPU_InitStruct.SubRegionDisable = 0x0;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Number = MPU_REGION_NUMBER2;
  MPU_InitStruct.BaseAddress = 0x30004000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_16KB;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL1;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM1 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */
////
  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM1)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */
////
  /* USER CODE END Callback 1 */
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
	while (1) {
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
