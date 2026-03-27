################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../RTOS/src/osmcal.c 

OBJS += \
./RTOS/src/osmcal.o 

C_DEPS += \
./RTOS/src/osmcal.d 


# Each subdirectory must supply rules for building sources it contributes
RTOS/src/%.o RTOS/src/%.su RTOS/src/%.cyclo: ../RTOS/src/%.c RTOS/src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F411xE -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I"/home/fyd/STM32CubeIDE/workspace_1.19.0/schedzo/RTOS/inc" -I"/home/fyd/STM32CubeIDE/workspace_1.19.0/schedzo/Drivers/CMSIS/Device/ST/STM32F4xx/Include" -I"/home/fyd/STM32CubeIDE/workspace_1.19.0/schedzo/Drivers/CMSIS/Include" -I"/home/fyd/STM32CubeIDE/workspace_1.19.0/schedzo/Drivers/STM32F4xx_HAL_Driver/Inc" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-RTOS-2f-src

clean-RTOS-2f-src:
	-$(RM) ./RTOS/src/osmcal.cyclo ./RTOS/src/osmcal.d ./RTOS/src/osmcal.o ./RTOS/src/osmcal.su

.PHONY: clean-RTOS-2f-src

