################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../MBEDTLS/App/mbedtls.c 

OBJS += \
./MBEDTLS/App/mbedtls.o 

C_DEPS += \
./MBEDTLS/App/mbedtls.d 


# Each subdirectory must supply rules for building sources it contributes
MBEDTLS/App/%.o MBEDTLS/App/%.su MBEDTLS/App/%.cyclo: ../MBEDTLS/App/%.c MBEDTLS/App/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_PWR_LDO_SUPPLY -DUSE_HAL_DRIVER -DSTM32H723xx '-DMBEDTLS_CONFIG_FILE=<mbedtls_config.h>' -c -I../Core/Inc -I"C:/Users/ASUS/Downloads/24062025ok/24062025ok/Firmware1/Core/Inc" -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I"D:/VINMOTION DATA/0. Lowlevel Firmware/MotonB Firmware/Firmware1/BMI270_SensorAPI" -I../MBEDTLS/App -I../Middlewares/Third_Party/mbedTLS/include/mbedtls -I../Middlewares/Third_Party/mbedTLS/include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-MBEDTLS-2f-App

clean-MBEDTLS-2f-App:
	-$(RM) ./MBEDTLS/App/mbedtls.cyclo ./MBEDTLS/App/mbedtls.d ./MBEDTLS/App/mbedtls.o ./MBEDTLS/App/mbedtls.su

.PHONY: clean-MBEDTLS-2f-App

