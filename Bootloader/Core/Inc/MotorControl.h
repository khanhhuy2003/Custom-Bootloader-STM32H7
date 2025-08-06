/*
 * MotorControl.h
 *
 *  Created on: Apr 19, 2025
 *      Author: letie
 */

#ifndef INC_MOTORCONTROL_H_
#define INC_MOTORCONTROL_H_

#include <main.h>
#include <stdbool.h>


typedef struct {
	uint32_t targetMotorCanId :8;		//bit 7 ~ bit 0
	uint32_t data :16;					//bit 23 ~ bit 8
	uint32_t mode :5;					//bit 28 ~ bit 24
	uint32_t reserved :3;
} CanId;								//Extended ID - 29 bits

uint32_t encode_can_id(CanId id);
int float_to_uint(float x, float x_min, float x_max, int bits);

void ReceivedCAN3();
void ReceivedCAN4();
void ReceivedCAN5();
void ReceivedCAN6();
void ReceivedCAN7();
void ReceivedCAN8();
void EnableMotor(uint16_t motor_id);

void EnableALLMotor();
void Motor2Control(uint16_t motor_id, float torque, float position,
		float speed, float kp, float kd, int port);
void Motor3Control(uint16_t motor_id, float torque, float position,
		float speed, float kp, float kd, int port);
void Motor4Control(uint16_t motor_id, float torque, float position,
		float speed, float kp, float kd, int port);

void Decode_Motor2_Feedback(uint8_t RxData[8], int ID);
void Decode_Motor3_Feedback(uint8_t RxData[8], int ID);
void Decode_Motor4_Feedback(uint8_t RxData[8], int ID);

#endif /* INC_MOTORCONTROL_H_ */
