/*
 * Define.h
 *
 *  Created on: Apr 12, 2025
 *      Author: letie
 */

#ifndef INC_DEFINE_H_
#define INC_DEFINE_H_

#include "main.h"

#define MAX_TX_SIZE 128
#define MAX_RX_SIZE 2048
#define MAX_MOTORS 23

//Motor calibration limited table
#define Motor1_min  -0.5
#define Motor1_max   0.5

#define Motor2_min   -0.5
#define Motor2_max   0.2

#define Motor3_min   -0.5
#define Motor3_max   0.5

#define Motor4_min  0.0
#define Motor4_max   1.0

#define Motor5_min   -0.5
#define Motor5_max   0.3

#define Motor6_min   -0.3
#define Motor6_max   0.5

#define Motor7_min  -0.5
#define Motor7_max   0.5

#define Motor8_min  -0.2
#define Motor8_max   0.5

#define Motor9_min  -0.5
#define Motor9_max  0.5

#define Motor10_min  -1.0
#define Motor10_max  0.0

#define Motor11_min  -0.3
#define Motor11_max   0.5

#define Motor12_min  -0.5
#define Motor12_max  0.4
//-----------------------------Safety Tay-----------------------------
#define Motor13_min  -0.3
#define Motor13_max  0.3

#define Motor14_min  -0.4
#define Motor14_max  0.4

#define Motor15_min  0.0
#define Motor15_max  0.5

#define Motor16_min  -0.8
#define Motor16_max  0.8

#define Motor17_min  -1.6
#define Motor17_max  0.2

#define Motor18_min  -0.8
#define Motor18_max  0.8

#define Motor19_min  -0.4
#define Motor19_max  0.4

#define Motor20_min  -0.5
#define Motor20_max  0.0

#define Motor21_min  -0.8
#define Motor21_max  0.8

#define Motor22_min  -0.2
#define Motor22_max  1.6

#define Motor23_min  -0.8
#define Motor23_max  0.8

//---------------------------------------------------------------------
#define P_MIN -12.57f
#define P_MAX 12.57f

#define V_MIN_02 -44.0f
#define V_MAX_02 44.0f
#define KP_MIN_02 0.0f
#define KP_MAX_02 500.0f
#define KD_MIN_02 0.0f
#define KD_MAX_02 5.0f
#define T_MIN_02 -17.0f
#define T_MAX_02 17.0f

#define V_MIN_03 -20.0f
#define V_MAX_03 20.0f
#define KP_MIN_03 0.0f
#define KP_MAX_03 5000.0f
#define KD_MIN_03 0.0f
#define KD_MAX_03 100.0f
#define T_MIN_03 -60.0f
#define T_MAX_03 60.0f

#define V_MIN_04 -15.0f
#define V_MAX_04 15.0f
#define KP_MIN_04 0.0f
#define KP_MAX_04 5000.0f
#define KD_MIN_04 0.0f
#define KD_MAX_04 100.0f
#define T_MIN_04 -120.0f
#define T_MAX_04 120.0f

#define POLYNOMIAL 0x04C11DB7
#define INITIAL_CRC 0xFFFFFFFF

typedef struct {
	float q;
	float dq;
	float tau_est;
	float temprature;
	uint32_t reserve[2];
} MotorStatePacket;

typedef struct {
	float quaternion[4];
	float gyroscope[3];
	float accelerometer[3];
} IMUStatePacket;

typedef struct {
	MotorStatePacket motorStates[23];
	IMUStatePacket imu;
	uint32_t reserve;
	uint32_t crc;
} LowStatePacket;
//-----------------------------------------------------
typedef struct {
	float q;
	float dq;
	float tau;
	float kp;
	float kd;
	uint32_t reserve[3];
} MotorCmdPacket;

typedef struct {
	MotorCmdPacket cmds[23];
	uint32_t reserve;
	uint32_t crc;
} LowCmdPacket;




typedef struct {
	uint32_t last_rx_time;
	uint8_t is_connected;
} MotorStatus;

uint32_t crc32(const void *data, size_t size);
//uint32_t crc32(const uint8_t *data, size_t length);

#endif /* INC_DEFINE_H_ */
