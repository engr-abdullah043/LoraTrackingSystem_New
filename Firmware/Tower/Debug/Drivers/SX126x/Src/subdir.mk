################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/SX126x/Src/sx126x.c 

OBJS += \
./Drivers/SX126x/Src/sx126x.o 

C_DEPS += \
./Drivers/SX126x/Src/sx126x.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/SX126x/Src/%.o Drivers/SX126x/Src/%.su Drivers/SX126x/Src/%.cyclo: ../Drivers/SX126x/Src/%.c Drivers/SX126x/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32U585xx -c -I../Core/Inc -I../Drivers/SX126x/Inc -I../Drivers/STM32U5xx_HAL_Driver/Inc -I../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-SX126x-2f-Src

clean-Drivers-2f-SX126x-2f-Src:
	-$(RM) ./Drivers/SX126x/Src/sx126x.cyclo ./Drivers/SX126x/Src/sx126x.d ./Drivers/SX126x/Src/sx126x.o ./Drivers/SX126x/Src/sx126x.su

.PHONY: clean-Drivers-2f-SX126x-2f-Src

