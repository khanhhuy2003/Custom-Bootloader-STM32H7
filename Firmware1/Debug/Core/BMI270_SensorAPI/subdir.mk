################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/BMI270_SensorAPI/MadgwickAHRS.c \
../Core/BMI270_SensorAPI/bmi2.c \
../Core/BMI270_SensorAPI/bmi270.c \
../Core/BMI270_SensorAPI/common.c 

OBJS += \
./Core/BMI270_SensorAPI/MadgwickAHRS.o \
./Core/BMI270_SensorAPI/bmi2.o \
./Core/BMI270_SensorAPI/bmi270.o \
./Core/BMI270_SensorAPI/common.o 

C_DEPS += \
./Core/BMI270_SensorAPI/MadgwickAHRS.d \
./Core/BMI270_SensorAPI/bmi2.d \
./Core/BMI270_SensorAPI/bmi270.d \
./Core/BMI270_SensorAPI/common.d 


# Each subdirectory must supply rules for building sources it contributes
Core/BMI270_SensorAPI/%.o Core/BMI270_SensorAPI/%.su Core/BMI270_SensorAPI/%.cyclo: ../Core/BMI270_SensorAPI/%.c Core/BMI270_SensorAPI/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_PWR_LDO_SUPPLY -DUSE_HAL_DRIVER -DSTM32H723xx -c -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../LWIP/App -I../LWIP/Target -I../Middlewares/Third_Party/LwIP/src/include -I../Middlewares/Third_Party/LwIP/system -I../Drivers/BSP/Components/lan8742 -I../Middlewares/Third_Party/LwIP/src/include/netif/ppp -I../Middlewares/Third_Party/LwIP/src/include/lwip -I../Middlewares/Third_Party/LwIP/src/include/lwip/apps -I../Middlewares/Third_Party/LwIP/src/include/lwip/priv -I../Middlewares/Third_Party/LwIP/src/include/lwip/prot -I../Middlewares/Third_Party/LwIP/src/include/netif -I../Middlewares/Third_Party/LwIP/src/include/compat/posix -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/arpa -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/net -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/sys -I../Middlewares/Third_Party/LwIP/src/include/compat/stdc -I../Middlewares/Third_Party/LwIP/system/arch -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-BMI270_SensorAPI

clean-Core-2f-BMI270_SensorAPI:
	-$(RM) ./Core/BMI270_SensorAPI/MadgwickAHRS.cyclo ./Core/BMI270_SensorAPI/MadgwickAHRS.d ./Core/BMI270_SensorAPI/MadgwickAHRS.o ./Core/BMI270_SensorAPI/MadgwickAHRS.su ./Core/BMI270_SensorAPI/bmi2.cyclo ./Core/BMI270_SensorAPI/bmi2.d ./Core/BMI270_SensorAPI/bmi2.o ./Core/BMI270_SensorAPI/bmi2.su ./Core/BMI270_SensorAPI/bmi270.cyclo ./Core/BMI270_SensorAPI/bmi270.d ./Core/BMI270_SensorAPI/bmi270.o ./Core/BMI270_SensorAPI/bmi270.su ./Core/BMI270_SensorAPI/common.cyclo ./Core/BMI270_SensorAPI/common.d ./Core/BMI270_SensorAPI/common.o ./Core/BMI270_SensorAPI/common.su

.PHONY: clean-Core-2f-BMI270_SensorAPI

